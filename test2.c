#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
    int pid;
    int nice_values[5] = {5, 3, 2, 1, 4}; // Non-sequential nice values
    int i;
    volatile int count;
    nice(getpid(), 1);
    for (i = 0; i < 5; i++) {
        pid = fork();
        if (pid == 0) {
            // Child process: Longer, CPU-bound task
            for (count = 0; count < 100; count++)
                printf(1, "[Child PID %d] running now with nice value %d\n", getpid(), nice_values[i]);
            printf(1, "[Child PID %d] Completed with assigned nice value %d\n", getpid(), nice_values[i]);
            exit();
        } else {
            // Parent process: Set the nice value of the child
            printf(1, "[Parent PTD %d] setting child to %d\n",getpid(), nice_values[i]);
            nice(pid, nice_values[i]);
        }
    }
    
    while (wait() != -1) {}
    exit();
}
