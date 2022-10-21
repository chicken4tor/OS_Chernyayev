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
#include <memory.h>

#include <compfuncs.h>
#include <trialfuncs.h>

#include "manager.h"
#include "shared_data.h"

const int NAMED_PIPE_MODE = S_IFIFO | 0640;
const int READ_BUFF = 1024;

enum _comm_status
{
    CS_NONE,
    CS_SENT,
    CS_RECEIVED,
};

/// @brief Is data send over named pipe, is response received?
typedef enum _comm_status comm_status_t;

struct _calculated_value
{
    comm_status_t comm;
    value_t value;
};

/// @brief Calculated value and relates states
typedef struct _calculated_value calculated_value_t;

struct _input_value
{
    int value;
    calculated_value_t result[NODES_COUNT];
};

/// @brief Input value and calculated results
typedef struct _input_value input_value_t;

struct _manager_state
{
    pid_t comp_nodes[NODES_COUNT];                // Reference to processes for computation
    int comm_fd[NODES_COUNT];                     // File descriptors for communication channels
    int input_fd[NODES_COUNT + 1];                // File descriptors for results communication channels, extra space for input stream
    int max_count;                                // Size of communication buffers
    input_value_t *x_values;                      // Input and results queue
    int x_start_pos;                              // Index of head element in circular input queue
    int x_free_pos;                               // Index of free element in circular input queue
    trial_function_t trial_function[NODES_COUNT]; // Trial function
    tf_result_t output_type[NODES_COUNT];         // Output value type
};

static char node_name[NODES_COUNT] = {'f', 'g'};

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

    // Find maximal file descriptor, to check select system call requirements
    int nfds = mgr->input_fd[0];

    for (int i = 1; i <= NODES_COUNT; i++)
    {
        nfds = MAX(nfds, mgr->input_fd[i]);
    }

    for (int i = 0; i < NODES_COUNT; i++)
    {
        nfds = MAX(nfds, mgr->comm_fd[i]);
    }

    if (nfds >= FD_SETSIZE)
    {
        printf("HUGE file descriptor - %d, upgrade to poll(2)\n", nfds);
        return NULL;
    }

    // Know your trial function

    mgr->trial_function[F_NODE] = function_from_name(f_func);
    mgr->trial_function[G_NODE] = function_from_name(g_func);

    for (int i = 0; i < NODES_COUNT; i++)
    {
        mgr->output_type[i] = trial_result_type(mgr->trial_function[i]);
    }

    return mgr;
}

void destruct_manager(manager_state_t *mgr)
{
    /// @todo Sync computation queues

    // Close file descriptors
    for (int i = 0; i < NODES_COUNT; i++)
    {
        close(mgr->comm_fd[i]);
        close(mgr->input_fd[i]);
        remove(node_pipe[i][0]);
        remove(node_pipe[i][1]);
    }

    // Free buffers
    free(mgr->x_values);

    free(mgr);
}

bool communicate(manager_state_t *mgr)
{
    fd_set data_streams;
    fd_set out_streams;

    int nfds = -1; // Maximal file descriptor

    // Input set
    FD_ZERO(&data_streams);

    // X values and results data streams
    for (int i = 0; i <= NODES_COUNT; i++)
    {
        FD_SET(mgr->input_fd[i], &data_streams);
        nfds = MAX(nfds, mgr->input_fd[i]);
    }

    // Send params to workers streams
    FD_ZERO(&out_streams);

    for (int i = 0; i < NODES_COUNT; i++)
    {
        // Is write operation expected
        if (mgr->x_start_pos != mgr->x_free_pos && mgr->x_values[mgr->x_start_pos].result[i].comm == CS_NONE)
        {
            FD_SET(mgr->comm_fd[i], &out_streams);
            nfds = MAX(nfds, mgr->comm_fd[i]);
        }
    }

    int sel_result;

    struct timeval io_timeout;

    io_timeout.tv_sec = 30;
    io_timeout.tv_usec = 0;

    sel_result = select(nfds + 1, &data_streams, &out_streams, NULL, &io_timeout);

    if (sel_result == -1)
    {
        int err = errno;

        if (err == EINTR)
        {
            // Interrupted by user
            return true;
        }
        else
        {
            /// @todo report error
            return false;
        }
    }
    else if (sel_result == 0)
    {
        // Timeout
        return true;
    }

    // printf("manager: select - %d\n", sel_result);

    // Get input value, add it to queue
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
            // printf("Read %ld - %ld, %s", value, result, buff);

            // Add value to queue
            memset(&mgr->x_values[mgr->x_free_pos], 0, sizeof(mgr->x_values[0]));
            mgr->x_values[mgr->x_free_pos].value = (int)value;
            mgr->x_free_pos = (mgr->x_free_pos + 1) % mgr->max_count;
        }
    }

    // Get calculated results
    for (int i = 0; i < NODES_COUNT; i++)
    {
        if (FD_ISSET(mgr->input_fd[i], &data_streams))
        {
            value_t val[10];
            ssize_t result = read(mgr->input_fd[i], val, sizeof(val));

            // Current implementation is limited to 1 result value from named pipe
            if (result <= 0 || result > sizeof(val))
            {
                fprintf(stderr, "COMM failed %d: %lu\n", i, result);
                return false;
            }

            for (int j = 0; j < result / sizeof(value_t); j++)
            {
                input_value_t *current = &mgr->x_values[mgr->x_start_pos];
                calculated_value_t *res_val = &current->result[i];
                res_val->comm = CS_RECEIVED;
                res_val->value = val[j];
                printf("trial_%c_%s(%d) %s", node_name[i], tf_name(mgr->output_type[i]), current->value, symbolic_status(res_val->value.status));
                if (res_val->value.status == COMPFUNC_SUCCESS)
                {
                    printf("<");
                    // Print actual value
                    switch (mgr->output_type[i])
                    {
                    case TFR_INT:
                        print_int_value(res_val->value.i_val);
                        break;
                    case TFR_UINT:
                        print_unsigned_int_value(res_val->value.ui_val);
                        break;
                    case TFR_FLOAT:
                        print_double_value(res_val->value.d_val);
                        break;
                    case TFR_BOOL:
                        print__Bool_value(res_val->value.b_val);
                        break;
                    }
                    printf(">");
                }
                printf("\n");
            }
        }
    }

    // Send X to calculators
    for (int i = 0; i < NODES_COUNT; i++)
    {
        if (FD_ISSET(mgr->comm_fd[i], &out_streams))
        {
            int result = write(mgr->comm_fd[i], &mgr->x_values[mgr->x_start_pos].value, sizeof(mgr->x_values[mgr->x_start_pos].value));
            if (result == -1)
            {
                // write operation failed
                return false;
            }
            mgr->x_values[mgr->x_start_pos].result[i].comm = CS_SENT;
        }
    }

    return true;
}
