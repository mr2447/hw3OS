
#include "types.h"
#include "user.h"

void priority_test() {
    int i;
    int pid = fork();

    if (pid == 0) {
        // Child process with low priority
        nice(getpid(), 5);  // Set highest nice value (lowest priority)
        for (i = 0; i < 5; i++) {
            printf(1, "Child process (low priority) running\n");
            sleep(5);
        }
        exit();
    } else {
        // Parent process with high priority
        nice(getpid(), 1);  // Set lowest nice value (highest priority)
        for (i = 0; i < 5; i++) {
            printf(1, "Parent process (high priority) running\n");
            sleep(5);
        }
        wait();
    }
}

int
main(void)
{
    // Other user tests...
    printf(1, "Running priority test:\n");
    priority_test();

    printf(1, "All tests passed!\n");
    exit();
}
