#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    int pid, new_value, old_value;

    if (argc == 3) {
        // Example: nice <pid> <value>
        pid = atoi(argv[1]);
        new_value = atoi(argv[2]);
    } else if (argc == 2) {
        // Example: nice <value> (applies to the current process)
        pid = getpid();
        new_value = atoi(argv[1]);
    } else {
        printf(2, "Usage: nice <pid> <value> or nice <value>\n");
        exit();
    }

    // Call the `nice` system call
    old_value = nice(pid, new_value);
    if (old_value < 0) {
        printf(2, "Error: Unable to set nice value for pid %d\n", pid);
    } else {
        printf(1, "%d %d\n", pid, old_value);  // print pid and old nice value
    }

    exit();
}