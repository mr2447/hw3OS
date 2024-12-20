#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define PRIORITY_SCHEDULER 0  // Set to 1 for priority scheduling, 0 for round-robin
#define MAX_PRIORITY 5        // Maximum priority level

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  struct proc *priority_head[MAX_PRIORITY]; // Tail pointers for circular lists
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  #if PRIORITY_SCHEDULER
    int i;
    for (i = 0; i < MAX_PRIORITY; i++) {
        ptable.priority_head[i] = 0;  // Initialize all heads to NULL
    }
    #endif
}

#if PRIORITY_SCHEDULER
void add_to_priority_queue(struct proc *p) {
  int priority_index = p->nice - 1;   // Determine the appropriate priority queue
  struct proc *head = ptable.priority_head[priority_index];

  if (head == 0) {
    // If the queue is empty, create a self-loop
    p->next = p->prev = p;
    ptable.priority_head[priority_index] = p;
  } else {
    // Insert the process after the tail in the circular list
    p->next = head;    // New process points to head
    p->prev = head->prev;   //Prev of p is tail
    head->prev->next = p; //Tail's next points to new process
    head->prev = p;    // Head's prev points to new process
    //P is now tail
  }
}
void remove_from_priority_queue(struct proc *p) {
  if (p->next == 0 && p->prev == 0) {
    //cprintf("Process %d already removed from priority queue\n", p->pid);
    return; // Process is already removed; do nothing
  }
   //cprintf("Removing process %d from priority queue\n", p->pid);
  int priority_index = p->nice - 1;
  struct proc *head = ptable.priority_head[priority_index];

  if (p->next == p) {  // Only one process in the queue
    //cprintf("Only process in priority queue, removing %d\n", p->pid);
    ptable.priority_head[priority_index] = 0;
  } else {
    //cprintf("Detaching process %d from priority queue\n", p->pid);
    p->prev->next = p->next;
    p->next->prev = p->prev;
    if (p == head) {
      ptable.priority_head[priority_index] = p->next; // Update head to next process if p is head
    }
  }

  // Clean up pointers for the removed process
  p->next = 0;
  p->prev = 0;
   //cprintf("Process %d removed successfully\n", p->pid);
}
#endif

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->nice = 3; //Default value for processes = 3

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;
  #if PRIORITY_SCHEDULER
  add_to_priority_queue(p); //Add process to queue
  #endif

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;
  np->nice = proc->nice;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  #if PRIORITY_SCHEDULER
  add_to_priority_queue(np);
  #endif

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
   // Notify that the process is exiting
  //cprintf("Process %d exiting\n", proc->pid);
  wakeup1(proc->parent);
  //cprintf("Process %d woke up parent %d\n", proc->pid, proc->parent->pid);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
        //cprintf("Process %d woke up init parent %d\n", proc->pid, proc->parent->pid);
    }
  }

  // Remove the process from the priority queue
  #if PRIORITY_SCHEDULER
  remove_from_priority_queue(proc);
  #endif
  //cprintf("Process %d removed from priority queue\n", proc->pid);


  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  //cprintf("Process %d set to ZOMBIE\n", proc->pid);
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        //cprintf("Reaping ZOMBIE process %d (parent %d)\n", p->pid, proc->pid);
        //remove_from_priority_queue(p); //redundant check remove if problems
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    //cprintf("Parent %d going to sleep in wait()\n", proc->pid);
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void scheduler(void)
{
  struct proc *p;

  for(;;){
    sti();  // Enable interrupts on this processor.
    acquire(&ptable.lock);

    #if PRIORITY_SCHEDULER  // Priority Scheduling
    int priority;
    int found_runnable = 0;
       // Iterate over each priority level from highest (1) to lowest (MAX_PRIORITY)
    for (priority = 0; priority < MAX_PRIORITY; priority++) {
      struct proc *head = ptable.priority_head[priority];
      
      // Skip to the next priority level if this one is empty
      if (!head) continue;

      p = head;

      do {
        if (p->state == RUNNABLE) {
          // Found a RUNNABLE process; schedule it
          found_runnable = 1;
          ptable.priority_head[priority] = p->next;  // Rotate the queue
          //cprintf("Scheduling process %d with priority %d\n", p->pid, p->nice);

          proc = p;
          switchuvm(p);
          p->state = RUNNING;
          swtch(&cpu->scheduler, p->context);
          switchkvm();

          // Clear `proc` after it runs
          proc = 0;
          break;
        }
        p = p->next;
      } while (p != head);  // Complete a full rotation if needed

      if(found_runnable) break;  // Exit priority loop if a process was scheduled
    }

    // // If no RUNNABLE process was found, put the CPU in an idle state
    // if (!found_runnable) {
    //   //cprintf("No RUNNABLE processes found, CPU going idle\n");
    //   release(&ptable.lock);
    //   asm volatile("hlt");  // Halt CPU until the next interrupt
    //   continue;  // Re-check processes after waking up
    // }


    #else  // Round Robin Scheduling

    // Loop to find the first RUNNABLE process in Round Robin order
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == RUNNABLE){
        proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&cpu->scheduler, p->context);
        switchkvm();
        proc = 0;  // Reset proc after process finishes or yields
        break;  // Only run one process per scheduling cycle
      }
    }

    #endif

    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  //cprintf("Switching out process %d\n", proc->pid);
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    // Print the process ID, state, name, and priority (nice value)
    cprintf("%d %s %s priority:%d", p->pid, state, p->name, p->nice);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// In proc.c
int
set_nice(int pid, int value)
{
  struct proc *p;
  acquire(&ptable.lock); // Lock the process table
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->pid == pid) { // Find process with the matching pid
      //cprintf("set_nice: Found process PID %d with current nice = %d\n", pid, p->nice); // Debug: Check current nice value

      int old_nice = p->nice; // Store the old nice value

      #if PRIORITY_SCHEDULER
      remove_from_priority_queue(p);  // Remove the process from its current priority queue

      p->nice = value; // Set the new nice value
      
      add_to_priority_queue(p); // Reinsert the process into the new priority queue based on updated nice value
      #else
      p->nice = value;
      #endif

      //cprintf("set_nice: PID %d, old nice = %d, new nice = %d\n", pid, old_nice, p->nice); // Debug: Confirm the update

      release(&ptable.lock); // Unlock the process table
      return old_nice; // Return the old nice value
    }
  }
  release(&ptable.lock); // Unlock if pid is not found
  //cprintf("set_nice: Process with PID %d not found\n", pid); // Debug: If process not found
  return -1; // Return -1 if the process was not found
}
