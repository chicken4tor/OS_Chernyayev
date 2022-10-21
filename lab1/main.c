#include <stdio.h>
#include <signal.h>
#include <sys/select.h>
#include <unistd.h>

#include "manager.h"

const int COMM_BUFFER = 10;

volatile sig_atomic_t cultural_canceling = 0;

void handle_interrupt()
{
    cultural_canceling = 1;
}

int main(int argc, char **argv)
{
    printf("OS Lab 1\n");

    signal(SIGINT, handle_interrupt);

    manager_state_t *mgr = construct_manager(STDIN_FILENO, COMM_BUFFER, "imul", "imin");

    if (mgr == NULL)
    {
        fprintf(stderr, "Something goes wrong\n");
        return 1;
    }

    while (1)
    {
        // Handle cancellation signal
        if (cultural_canceling)
        {
            cultural_canceling = false;

            bool canceling_confirmed = false;

            printf("Please confirm that computation should be stopped y(es, stop)/n(ot yet)[n]\n");

            fd_set input_streams;

            FD_ZERO(&input_streams);

            FD_SET(STDIN_FILENO, &input_streams);

            struct timeval io_timeout;

            io_timeout.tv_sec = 5;
            io_timeout.tv_usec = 0;

            int sel_result = select(1, &input_streams, NULL, NULL, &io_timeout);

            if (sel_result == 1)
            {
                char buff[50];

                int retval = read(STDIN_FILENO, buff, sizeof(buff));

                if (retval == 2)
                {
                    if (buff[0] == 'y' || buff[0] == 'Y')
                    {
                        canceling_confirmed = true;
                    }
                }
            }

            if (!canceling_confirmed)
            {
                printf("action is not confirmed within 5 seconds. proceeding...\n");
            }
            else
            {
                /// @todo Drain input queue
                // communicate(mgr, false /* No new input*/)
                break;
            }
        }

        if (!communicate(mgr))
        {
            fprintf(stderr, "mgr: failure in communication\n");
            break;
        }
    }

    // perform cleanup...
    destruct_manager(mgr);

    return 0;
}
