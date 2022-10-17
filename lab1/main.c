#include <stdio.h>
#include <signal.h>

#include <unistd.h> // @todo Use nanosleep in productin code

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

    manager_state_t *mgr = construct_manager(STDIN_FILENO, COMM_BUFFER);

    // create named pipes
    // spawn processes
    // get x
    // send x to f
    // send x to g
    // get response from f
    // get response from g
    // calculate f*g, send result

    while (1)
    {
        // Handle cancellation signal
        if (cultural_canceling)
        {
            printf("Good bye, cruel world\n");

            // perform cleanup...
            destruct_manager(mgr);

            return 0;
        }

        communicate(mgr);

        sleep(1);
    }

    // Unreachable branch
    return 1;
}
