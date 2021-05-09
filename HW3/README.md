For this project extended our library for threads. The previous project implemented pthread_create(), pthread_exit(), pthread_self(), schedule(), and scheduler_init() functions. This project implemented pthread_mutex_init(), pthread_mutex_lock(), pthread_mutex_unlock_(), pthread_mutex_destroy(), pthread_barrier_init(), pthread_barrier_wait(), and pthread_barrier_destroy() functions.

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

pthread_mutex_init(): This function mallocs space for the struct 'waiting_threads_mutex' and assigns it to the mutex argument. This struct will contain a linked list of the threads waiting to acquire the mutex, the current status of the mutex (locked or unlocked), and the ID of the thread that currently holds the mutex.

pthread_mutex_lock(): This function lets the calling thread acquire the mutex if the mutex is unlocked. If the mutex is already locked, then the thread gets blocked and its ID is added to the linked list that contains the IDs of the waiting threads. 

pthread_mutex_unlock(): This function unlocks the mutex if the calling thread has the same ID as the current mutex holder. If the IDs do not match, then an error code is returned.

pthread_mutex_destroy(): This function frees the current mutex by freeing each member of its linked list then freeing the mutex itself.

pthread_barrier_init(): This function mallocs space for the struct 'barrier_threads_mutex' and assigns it to the barrier argument. This struct will contain a linked list of the threads waiting in the barrier, the count value that must be reached for the threads in the barrier to be released, and the current number of threads in the barrier.

pthread_barrier_wait(): This function blocks the calling thread and adds it to the barrier (by storing its ID in the linked list). If the number of threads in the barrier reaches 'count' then the threads' statuses are all set from TS_BLOCKED to TS_READY. After the barrier is released, the IDs in the linked list are all set to -1 to invalidate those nodes and prepare the linked list for the next barrier. This function returns PTHREAD_BARRIER_SERIAL_THREAD when the barrier is released ('count' is reached) and 0 otherwise.

pthread_barrier_destroy(): This function frees the current barrier by freeing each member of its linked list then freeing the barrier itself.