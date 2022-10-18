#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

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

    // input formats

    int input_fd = open(node_pipe[node], O_RDWR);
    if (input_fd == -1)
    {
        fprintf(stderr, "Failed to open named pipe - %s\n", node_pipe[node]);
        return 1;
    }

    // listen for input
    char buff[1024];
    while (1)
    {
        // Blocking Input-Output operation
        int retval = read(input_fd, buff, 1024);
        if (retval == -1)
        {
            printf("NODE %d: Data error\n", node);
            return 1;
        }
        printf("NODE %d: Data ready - %d\n", node, retval);

        // calculate

        // send result
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
