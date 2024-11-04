#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
    int pid;
    int nice_values[] = {1, 2, 3, 4, 5}; // Nice values to test
    int num_processes = sizeof(nice_values) / sizeof(nice_values[0]);
    int i;  // Declare the loop variable here for reuse

    printf(1, "Starting nice test\n");
    nice(getpid(), 2);

    for (i = 0; i < num_processes; i++) {
        pid = fork();
        if (pid < 0) {
            printf(1, "Fork failed\n");
            exit();
        }
        if (pid == 0) {
            // Child process: Set its own nice value using its PID
            int old_nice = nice(getpid(), nice_values[i]);
            printf(1, "Child PID %d: Set nice to %d, old nice was %d\n", getpid(), nice_values[i], old_nice);
            sleep(10);  // Slow down to allow print to finish
            exit();
        }
        sleep(10); // Short delay between forks to reduce output overlap
    }

    // Parent waits for all children to finish
    for (i = 0; i < num_processes; i++) {
        wait();
    }

    printf(1, "Nice test completed\n");
    exit();
}
