#include "types.h"
#include "stat.h"
#include "user.h"

void spawn_process(int nice_value, int id) {
    int pid = fork();
    volatile int i;
    if (pid == 0) {
        nice(getpid(), nice_value);  // Set the process nice value
        for (i = 0; i < 5000000; i++); // Workload loop
        printf(1, "Process %d with nice=%d completed\n", id, nice_value);
        exit();
    }
}

int main(void) {
    int i;
    printf(1, "Starting test for competition between high and low priority processes.\n");
    
    // Spawn multiple low-priority processes (nice = 5)
    for (i = 0; i < 10; i++) {
        spawn_process(5, i);  // Each with nice=5
    }

    // Spawn one high-priority process (nice = 1)
      // Process ID 99 with nice=1
    spawn_process(1, 99);
    // Wait for all child processes to complete
    for (i = 0; i < 11; i++) wait();

    printf(1, "All processes completed.\n");
    exit();
}
