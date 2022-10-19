#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include <trialfuncs.h>

#include "shared_data.h"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stdout, "Usage: %s <f or g> <function>\n", argv[0]);
        return 1;
    }

    computation_node node = NODES_COUNT; // Unknown node type

    if (strcmp("f", argv[1]) == 0)
    {
        node = F_NODE;
    }
    else if (strcmp("g", argv[1]) == 0)
    {
        node = G_NODE;
    }
    else
    {
        fprintf(stderr, "Invalid node type - %s\n", argv[1]);
        return 1;
    }

    trial_function_t tf = function_from_name(argv[2]);

    if (tf == TF_UNKNOWN)
    {
        fprintf(stderr, "Unknown trial function - %s\n", argv[2]);
        return 1;
    }

    // input formats

    int comm_fd = open(node_pipe[node][0], O_CREAT|O_RDWR);
    if (comm_fd == -1)
    {
        fprintf(stderr, "Failed to open input named pipe - %s\n", node_pipe[node][0]);
        return 1;
    }

    int result_fd = open(node_pipe[node][1], O_CREAT|O_RDWR);
    if (result_fd == -1)
    {
        fprintf(stderr, "Failed to open result named pipe - %s\n", node_pipe[node][1]);
        return 1;
    }

    // listen for input
    int buff[256];
    while (1)
    {
        // Blocking Input-Output operation
        int retval = read(comm_fd, buff, sizeof(buff));
        if (retval == -1)
        {
            printf("NODE %d: Data error\n", node);
            return 1;
        }
        else if (retval == 0)
        {
            // Nothing to read, finish process
            return 0;
        }

        printf("NODE %d: Data ready - %d, X - ", node, retval);

        // Process all input values, even if we read more than one integer from named pipe
        for (int i = 0; i < retval / sizeof(int); i++)
        {
            int x = buff[i];

            printf("%d ", x);

            // calculate
            value_t result;

            tf_result_t result_type;

            switch (node)
            {
            case F_NODE:
                switch (tf)
                {
                case TF_IMUL:
                    result.status = trial_f_imul(x, &result.i_val);
                    break;
                case TF_IMIN:
                    result.status = trial_f_imin(x, &result.i_val);
                    break;
                case TF_FMUL:
                    result.status = trial_f_fmul(x, &result.d_val);
                    break;
                case TF_AND:
                    result.status = trial_f_and(x, &result.b_val);
                    break;
                case TF_OR:
                    result.status = trial_f_or(x, &result.b_val);
                    break;
                }
                break;
            case G_NODE:
                switch (tf)
                {
                case TF_IMUL:
                    result.status = trial_g_imul(x, &result.i_val);
                    break;
                case TF_IMIN:
                    result.status = trial_g_imin(x, &result.i_val);
                    break;
                case TF_FMUL:
                    result.status = trial_g_fmul(x, &result.d_val);
                    break;
                case TF_AND:
                    result.status = trial_g_and(x, &result.b_val);
                    break;
                case TF_OR:
                    result.status = trial_g_or(x, &result.b_val);
                    break;
                }
                break;
            }

            // send result
            int w_result = write(result_fd, &result, sizeof(result));
            if (w_result == -1 || w_result != sizeof(result))
            {
                // Error, or data write is incomplete
                fprintf(stderr, "NODE %d: Data write error (%d)\n", node, w_result);
                return 1;
            }
            fsync(result_fd);
        }

        printf("\n");

        sleep(1);
    }

    // for (int i = 0; i < 20; i++)
    // {
    //     PROCESS_FUNC(g, or, i);
    // }

    // compfunc_status_t status;
    // double fresult;
    // int iresult;
    // printf ("f(0) and g(0): \n");
    // status = trial_f_imul(0, &iresult);
    // printf ("f_imul(0): %s\n", symbolic_status(status));
    // if (status == COMPFUNC_SUCCESS)
    //     printf ("f_imul(0): %d\n", iresult);
    // printf ("f(0): %d\n", trial_f_imul(0, &iresult));
    // PROCESS_FUNC(f, imul, 0);
    // printf ("g(0): %d\n", trial_g_imul(0, &iresult));
    // PROCESS_FUNC(g, imul, 0);
    // printf ("g(1): %d\n", trial_g_imul(1, &iresult));
    // PROCESS_FUNC(g, imul, 1);
    // printf ("f(2): %d\n", trial_f_imul(2, &iresult));
    // PROCESS_FUNC(f, imul, 2);
    // printf ("g(3): %d\n", trial_g_imul(3, &iresult));
    // PROCESS_FUNC(g, imul, 3);

    // PROCESS_FUNC(g, or, -1);
    // PROCESS_FUNC(g, or, 0);
    // PROCESS_FUNC(g, imin, 0);

    return 0;
}
