* xv6 folder will be submitted with CPUS set to 1 but you can change it to 2 and run these tests by using the adjustments mentioned in each test description. At 2 cpus, the tests require more output than can be screenshot, so they are demonstrated here on 1 cpu.

CL Demonstration: 
0. Set Priority_SCHEDULER to 0 in proc.c. 0 means the program is using Round Robin
1. run make qemu-nox
2. nice 4
3. nice 4 5
5. nice 3 2
6. nice 2

Test File Demonstration (nice):
0. Set Priority_SCHEDULER to 0 in proc.c. 0 means the program is using Round Robin
1. run make qemu-nox
2. test6

Test 1 Demonstration: 
0. Set Priority_SCHEDULER to 1 in proc.c. 1 means the program is using Priority Scheduler
1. run make qemu-nox
2. test1

Test 2 Demonstration: 
0. Set Priority_SCHEDULER to 1 in proc.c. 1 means the program is using Priority Scheduler
1. run make qemu-nox
2. test2

Test 3 Demonstration: 
0. Set Priority_SCHEDULER to 1 in proc.c. 1 means the program is using Priority Scheduler
1. run make qemu-nox
2. test3

