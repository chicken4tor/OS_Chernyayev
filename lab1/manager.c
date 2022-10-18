#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdio.h>
#include <errno.h>
#include <spawn.h>

#include <compfuncs.h>

#include "manager.h"
#include "shared_data.h"

const int NAMED_PIPE_MODE = 0666;
const int READ_BUFF = 1024;

struct _input_value
{
    long value;
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
    pid_t comp_nodes[NODES_COUNT];                         /// Reference to processes for computation
    int comm_fd[NODES_COUNT + 1];                          /// File descriptors for communication channels, extra space for input stream
    int max_count;                                         /// Size of communication buffers
    input_value_t *x_values;                               /// Input queue
    int x_start_pos;                                       /// Index of head element in circular input queue
    int x_free_pos;                                        /// Index of free element in circular input queue
    int output_type[NODES_COUNT];                          /// Output value type
    calculated_value_t *intermediate_results[NODES_COUNT]; /// Output values from computational nodes
    int intermediate_start_pos[NODES_COUNT];               /// Index of head element in computational queue
    int intermediate_free_pos[NODES_COUNT];                /// Index of free element in computational queue
};

manager_state_t *construct_manager(int input_fd, int buffer_size, const char *f_func, const char *g_func)
{
    manager_state_t *mgr = malloc(sizeof(manager_state_t));

    // Create named pipes
    mkfifo(node_pipe[F_NODE], NAMED_PIPE_MODE);
    mkfifo(node_pipe[G_NODE], NAMED_PIPE_MODE);

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

    // Open file descriptors
    for (int i = 0; i < NODES_COUNT; i++)
    {
        mgr->comm_fd[i] = open(node_pipe[i], O_RDWR);
    }
    mgr->comm_fd[NODES_COUNT] = input_fd;

    // Launch computation processes
    int status;
    char *args[] = {
        calc_task,
        "f",
        f_func,
        NULL};

    status = posix_spawn(&mgr->comp_nodes[F_NODE], calc_task, NULL, NULL, args, NULL);
    if (status != 0)
    {
        printf("Start 1 failed\n");
    }

    args[1] = "g";
    args[2] = g_func;

    status = posix_spawn(&mgr->comp_nodes[G_NODE], calc_task, NULL, NULL, args, NULL);
    if (status != 0)
    {
        printf("Start 2 failed\n");
    }

    return mgr;
}

void destruct_manager(manager_state_t *mgr)
{
    /// @todo Sync computation queues
    ;

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
    }

    free(mgr);
}

bool communicate(manager_state_t *mgr)
{
    fd_set input_stream;
    fd_set results_stream;

    // Input set
    FD_ZERO(&input_stream);
    FD_SET(mgr->comm_fd[NODES_COUNT], &input_stream);

    // Results set
    FD_ZERO(&results_stream);

    for (int i = 0; i < NODES_COUNT; i++)
    {
        FD_SET(mgr->comm_fd[i], &results_stream);
    }

    int sel_result;

    struct timeval io_timeout;

    io_timeout.tv_sec = 30;
    io_timeout.tv_usec = 0; // 500000 == .5 sec interval

    sel_result = select(NODES_COUNT, &input_stream, &results_stream, NULL, &io_timeout);

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

    // Get calculated results
    for (int i = 0; i < NODES_COUNT; i++)
    {
        if (FD_ISSET(mgr->comm_fd[i], &results_stream))
        {
            char buff[READ_BUFF];
            ssize_t result = read(mgr->comm_fd[i], buff, sizeof(buff));
        }
    }

    // Get input value, and send it to sub-processes
    if (FD_ISSET(mgr->comm_fd[NODES_COUNT], &input_stream))
    {
        // Assumption: Data is read by lines, one X value per line
        char buff[READ_BUFF + 1]; // Extra char for zero string termination
        ssize_t result = read(mgr->comm_fd[NODES_COUNT], buff, READ_BUFF);
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

            x.value = value;
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

    return true;
}
