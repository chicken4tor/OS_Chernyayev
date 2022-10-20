#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdio.h>
#include <errno.h>
#include <spawn.h>
#include <sys/param.h>

#include <compfuncs.h>

#include "manager.h"
#include "shared_data.h"

const int NAMED_PIPE_MODE = S_IFIFO | 0640;
const int READ_BUFF = 1024;

struct _input_value
{
    int value;
    compfunc_status_t g_status;
    compfunc_status_t f_status;
};

typedef struct _input_value input_value_t;

struct _calculated_value
{
    compfunc_status_t status; /// Operation status
    value_t value;
};

typedef struct _calculated_value calculated_value_t;

struct _manager_state
{
    pid_t comp_nodes[NODES_COUNT];                         // Reference to processes for computation
    int comm_fd[NODES_COUNT];                              // File descriptors for communication channels
    int input_fd[NODES_COUNT + 1];                         // File descriptors for results communication channels, extra space for input stream
    int nfds;                                              // Maximal file descriptor + 1 for select call
    int max_count;                                         // Size of communication buffers
    input_value_t *x_values;                               // Input queue
    int x_start_pos;                                       // Index of head element in circular input queue
    int x_free_pos;                                        // Index of free element in circular input queue
    int output_type[NODES_COUNT];                          // Output value type
    calculated_value_t *intermediate_results[NODES_COUNT]; // Output values from computational nodes
    int intermediate_start_pos[NODES_COUNT];               // Index of head element in computational queue
    int intermediate_free_pos[NODES_COUNT];                // Index of free element in computational queue
};

manager_state_t *construct_manager(int input_fd, int buffer_size, const char *f_func, const char *g_func)
{
    manager_state_t *mgr = malloc(sizeof(manager_state_t));

    // Create named pipes
    mkfifo(node_pipe[F_NODE][0], NAMED_PIPE_MODE);
    mkfifo(node_pipe[F_NODE][1], NAMED_PIPE_MODE);
    mkfifo(node_pipe[G_NODE][0], NAMED_PIPE_MODE);
    mkfifo(node_pipe[G_NODE][1], NAMED_PIPE_MODE);

    // Allocate buffers
    mgr->max_count = buffer_size;
    mgr->x_values = malloc(sizeof(input_value_t) * buffer_size);
    mgr->x_start_pos = 0;
    mgr->x_free_pos = 0;

    for (int i = 0; i < NODES_COUNT; i++)
    {
        mgr->intermediate_results[i] = malloc(sizeof(calculated_value_t) * buffer_size);
        mgr->intermediate_start_pos[i] = 0;
        mgr->intermediate_free_pos[i] = 0;
    }

    // Launch computation processes, at first
    int status;
    char *args[] = {
        calc_task,
        "f",
        f_func,
        NULL};

    status = posix_spawn(&mgr->comp_nodes[F_NODE], calc_task, NULL, NULL, args, NULL);
    if (status != 0)
    {
        printf("f node - failed\n");
    }

    args[1] = "g";
    args[2] = g_func;

    status = posix_spawn(&mgr->comp_nodes[G_NODE], calc_task, NULL, NULL, args, NULL);
    if (status != 0)
    {
        printf("g node - failed\n");
    }

    // Open file descriptors
    for (int i = 0; i < NODES_COUNT; i++)
    {
        mgr->comm_fd[i] = open(node_pipe[i][0], O_WRONLY);
        if (mgr->comm_fd[i] == -1)
        {
            fprintf(stderr, "manager: Named pipe open failed %s\n", node_pipe[i][0]);
            /// @todo Cleanup partially constructed object
            return NULL;
        }
    }

    for (int i = 0; i < NODES_COUNT; i++)
    {
        mgr->input_fd[i] = open(node_pipe[i][1], O_RDONLY);
        if (mgr->input_fd[i] == -1)
        {
            fprintf(stderr, "manager: Results named pipe open failed %s\n", node_pipe[i][1]);
            /// @todo Cleanup partially constructed object
            return NULL;
        }
    }

    mgr->input_fd[NODES_COUNT] = input_fd;

    // Find maximal file descriptor
    mgr->nfds = mgr->input_fd[0];

    for (int i = 1; i <= NODES_COUNT; i++)
    {
        mgr->nfds = MAX(mgr->nfds, mgr->input_fd[i]);
    }

    mgr->nfds++;

    if (mgr->nfds > FD_SETSIZE)
    {
        printf("HUGE file descriptor - %d\n", mgr->nfds - 1);
        return NULL;
    }

    return mgr;
}

void destruct_manager(manager_state_t *mgr)
{
    /// @todo Sync computation queues

    // Free buffers
    free(mgr->x_values);

    for (int i = 0; i < NODES_COUNT; i++)
    {
        free(mgr->intermediate_results[i]);
    }

    // Close file descriptors
    for (int i = 0; i < NODES_COUNT; i++)
    {
        close(mgr->comm_fd[i]);
        close(mgr->input_fd[i]);
        remove(node_pipe[i][0]);
        remove(node_pipe[i][1]);
    }

    free(mgr);
}

bool communicate(manager_state_t *mgr)
{
    fd_set data_streams;

    // Input set
    FD_ZERO(&data_streams);

    // X values and results data streams
    for (int i = 0; i <= NODES_COUNT; i++)
    {
        FD_SET(mgr->input_fd[i], &data_streams);
    }

    int sel_result;

    struct timeval io_timeout;

    io_timeout.tv_sec = 30;
    io_timeout.tv_usec = /*5*/00000; // .5 sec interval

    sel_result = select(mgr->nfds, &data_streams, NULL, NULL, &io_timeout);

    if (sel_result == -1)
    {
        /// @todo report error
        return false;
    }
    else if (sel_result == 0)
    {
        // Timeout
        return true;
    }

    printf("manager: select - %d\n", sel_result);

    // Get input value, and send it to sub-processes
    if (FD_ISSET(mgr->input_fd[NODES_COUNT], &data_streams))
    {
        // Assumption: Data is read by lines, one X value per line
        char buff[READ_BUFF + 1]; // Extra char for zero string termination
        ssize_t result = read(mgr->input_fd[NODES_COUNT], buff, READ_BUFF);
        buff[result] = 0;
        long value;
        char *endptr;
        errno = 0;
        value = strtol(buff, &endptr, 10);
        if (errno != 0)
        {
            printf("Failed to parse input (%d) - %ld, %s", errno, result, buff);
        }
        else
        {
            printf("Read %ld - %ld, %s", value, result, buff);

            // Send X to calculators
            input_value_t x;

            x.value = (int)value;
            x.f_status = COMPFUNC_STATUS_MAX;
            x.f_status = COMPFUNC_STATUS_MAX;

            for (int i = 0; i < NODES_COUNT; i++)
            {
                int result = write(mgr->comm_fd[i], &x.value, sizeof(x.value));
                if (result == -1)
                {
                    // write operation failed
                    return false;
                }
            }

            // Add value to queue
            mgr->x_values[mgr->x_free_pos] = x;
            mgr->x_free_pos = (mgr->x_free_pos + 1) % mgr->max_count;
        }
    }

    // Get calculated results
    for (int i = 0; i < NODES_COUNT; i++)
    {
        if (FD_ISSET(mgr->input_fd[i], &data_streams))
        {
            char buff[READ_BUFF];
            ssize_t result = read(mgr->input_fd[i], buff, sizeof(buff));
            printf("read from %d, %ld\n", i, result);
        }
    }

    return true;
}
