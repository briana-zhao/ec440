For this project we created a library for threads. This included implementing a pthread_create(), pthread_exit(), pthread_self(), schedule(), and scheduler_init() functions.
The threads are scheduled using a Round-Robin scheduler, which selects a new thread to run every 50 ms.

I used the following resources to help understand/implement the timer in my program. 
https://www.lemoda.net/c/ualarm-setjmp/
https://www.gnu.org/software/libc/manual/html_node/Setting-an-Alarm.html
https://www.gnu.org/software/libc/manual/html_node/Time-Types.html
https://www.gnu.org/software/libc/manual/html_node/Signal-Sets.html

____________________________________________________________________________________
pthread_create: This function first calls scheduler_init() if it is the first time calling pthread_create. Then it finds a unique thread ID by searching through the TCB array (allthreads) for a spot that is not yet populated by a thread. The function then sets the ID and allocates memory for the thread's stack. The address of pthread_exit is stored at the top of the stack so that pthread_exit is executed once the thread finishes its task. The newly created thread's status is set to READY, and setjmp is called to save the current state of its registers. Then, the thread's registers are modified in order to start the thread's routine and store its stack. Lastly, pthread_create() calls schedule() to schedule the newly created thread.

pthread_exit(): This function sets the current thread's status to EXITED and calls schedule() if there are still threads running. Otherwise, it calls exit().

pthread_self(): This function returns the ID of the current running thread.

scheduler_init(): This function executes the first time pthread_create() is called. It turns the main function into the first thread, then it sets up an alarm to go off every 50 ms. Every time this alarm goes off, the schedule() function handles it and selects a new thread to run. 

scheduler(): This function first calls setjmp to save the state of the current thread's registers. Then it uses Round-Robin to find the next thread that has READY status. Once this thread is found, longjmp is called to context switch to the previously saved state of that thread.