#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
    printf(1, "Starting test_nice program to demonstrate `nice` system call with one and two argument cases.\n");

    int pid1, pid2, pid3;

    // Test case where only the new nice value is provided (applies to the current process)
    printf(1, "Parent (PID: %d) - Setting nice value to 1\n", getpid());
    int old_nice_parent = nice(getpid(), 1);  // Applies to the current process (parent)

    printf(1, "Parent (PID: %d), Previous nice: %d, New nice: 1\n", getpid(), old_nice_parent);

    // Fork child process 1 and set nice value with both PID and value arguments
    if ((pid1 = fork()) == 0) {
        // Child process 1
 
        int old_nice = nice(getpid(), 5);  // Passes both PID and value
        wait();

        printf(1, "Child 1 (PID: %d), Previous nice: %d, New nice: 5\n", getpid(), old_nice);

        
        exit();
    }
    else {
        printf(1, "Parent (PID: %d), Previous nice: %d, New nice: 1\n", getpid(), old_nice_parent);
    }

    // Fork child process 2 and set nice value with both PID and value arguments
    if ((pid2 = fork()) == 0) {
        // Child process 2

        int old_nice = nice(getpid(), 3);  // Passes both PID and value
        wait();

        printf(1, "Child 2 (PID: %d), Previous nice: %d, New nice: 3\n", getpid(), old_nice);

        exit();
    }
    else {
        printf(1, "Parent (PID: %d), Previous nice: %d, New nice: 1\n", getpid(), old_nice_parent);
    }

    // Fork child process 3 and set nice value with both PID and value arguments
    if ((pid3 = fork()) == 0) {
        // Child process 3
  
        int old_nice = nice(getpid(), 2);  // Passes both PID and value
        wait();
  
        printf(1, "Child 3 (PID: %d), Previous nice: %d, New nice: 2\n", getpid(), old_nice);

        exit();
    }
    else {
        printf(1, "Parent (PID: %d), Previous nice: %d, New nice: 1\n", getpid(), old_nice_parent);
    }
    // Wait for all child processes to complete
    wait();
    wait();
    wait();

    printf(1, "All child processes have completed. Test of `nice` system call with one and two arguments finished.\n");

    exit();
}
