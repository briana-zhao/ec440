#include "ec440threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* You can support more threads. At least support this many. */
#define MAX_THREADS 128

/* Your stack should be this many bytes in size */
#define THREAD_STACK_SIZE 32767

/* Number of microseconds between scheduling events */
#define SCHEDULER_INTERVAL_USECS (50 * 1000)

/* Extracted from private libc headers. These are not part of the public
 * interface for jmp_buf.
 */
#define JB_RBX 0
#define JB_RBP 1
#define JB_R12 2
#define JB_R13 3
#define JB_R14 4
#define JB_R15 5
#define JB_RSP 6
#define JB_PC 7


enum thread_status
{
	TS_EXITED,
	TS_RUNNING,
	TS_READY,
	TS_STANDBY,
	TS_BLOCKED
};

/* The thread control block stores information about a thread. You will
 * need one of this per thread.
 */
struct thread_control_block {
	/* TODO: add a thread ID */
	pthread_t id;
	/* TODO: add information about its stack */
	unsigned long int *rsp;
	/* TODO: add information about its registers */
	jmp_buf registers;
	/* TODO: add information about the status (e.g., use enum thread_status) */
	enum thread_status status;
	/* Add other information you need to manage this thread */
};



struct Node
{
	int data_id;
	struct Node* next;
};

struct waiting_threads_mutex
{
	int list_length;
	int mutex_status; //might not need
	int lock_holder; //might not need
	struct Node* first;
	struct Node* last;
};

struct waiting_threads_barrier
{
	int count;
	int num_boundary; //number of threads currently in boundary
	int barrier_full;
	struct Node* first;
	struct Node* last;
};

//Create an array of threads (an array of the struct thread_control_block)
struct thread_control_block allthreads[MAX_THREADS];

int thread_current = 0;	//Keeps track of the current running thread
int num_running_threads = 0; //Keeps track of the current number of running (active) threads
int allthreads_index = 0;
static bool is_first_call = true;

static void lock()
{
	sigset_t sset;
	sigemptyset(&sset);
	sigaddset(&sset, SIGALRM);
	sigprocmask(SIG_BLOCK, &sset, NULL);
}

static void unlock()
{
	sigset_t sset;
	sigemptyset(&sset);
	sigaddset(&sset, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &sset, NULL);
}




static void schedule()
{
	lock();

	int n = 0;
	
	if(setjmp(allthreads[thread_current].registers) == 0)	//Use setjump to save context of current thread
	{

		//Use Round Robin to find the next ready thread to run
		thread_current++;

		while(allthreads[thread_current].status != TS_READY)
		{
			thread_current++;

			if (thread_current > num_running_threads - 1)
			{
				thread_current = 0;

				n++;
			}

			if (n >= 2 * num_running_threads)
			{
				if (allthreads[thread_current].status == TS_BLOCKED)
				{
					allthreads[thread_current].status = TS_READY;
				}
				thread_current++;
				break;
			}
		}
		
		unlock();
		//Use longjmp to context switch to the next thread
		longjmp(allthreads[thread_current].registers, 1);
	}
	else	//longjmp called
	{		
		//Next scheduled thread is now resuming execution...
	}
}

static void scheduler_init()
{
	
	//Set all thread's status to TS_STANDBY for now since they're created but are not yet READY
	for (int t = 0; t < MAX_THREADS; t++)
	{
		allthreads[t].status = TS_STANDBY;
	}


	//Before starting the timer, must make a TCB for 'main' which is the first thread
	thread_current = 0;
	allthreads[0].id = 0;
	allthreads[0].rsp = NULL;
	allthreads[0].status = TS_READY;


	setjmp(allthreads[0].registers); //Save registers with setjmp

	num_running_threads++;	//'main' is the first running thread

	//Set up a SIGALRM that goes off every 50ms (this is the quantum)
	struct sigaction alarm = {{0}};

	alarm.sa_handler = schedule;	//Whenever alarm goes off, schedule() handles it
	sigemptyset(&alarm.sa_mask);
	alarm.sa_flags = SA_NODEFER;	//Allows the signal to be received inside the signal handler (which is schedule())
	sigaction(SIGALRM, &alarm, NULL);

	ualarm(SCHEDULER_INTERVAL_USECS, SCHEDULER_INTERVAL_USECS);	//Call ualarm to set the timer
}

int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg)
{
	
	if (is_first_call)
	{
		is_first_call = false;
		scheduler_init();
	}

	if (num_running_threads < MAX_THREADS)	//Max number of threads is 128
	{
		
		for(int i = 0; i < MAX_THREADS; i++)	//Find the next available ID that can be used
		{
			if (allthreads[i].status != TS_READY && allthreads[i].status != TS_EXITED)
			{
				allthreads_index = i;
				break;
			}
		}
		
		//Create the TCB for the new thread
		unsigned long int* stack_pointer;
		allthreads[num_running_threads].id = allthreads_index;				//Set thread's ID
		*thread = allthreads_index;
		allthreads[num_running_threads].rsp = (unsigned long int *)malloc(THREAD_STACK_SIZE);		//Create thread's stack
		stack_pointer = allthreads[num_running_threads].rsp;
		stack_pointer = (stack_pointer + (THREAD_STACK_SIZE / sizeof(unsigned long int)) - 1);		//Move to top of stack, leaving space for address of pthread_exit to be stored
		*(stack_pointer) = (unsigned long int)pthread_exit;					//Place address of pthread_exit at top of stack
	
		
		allthreads[num_running_threads].status = TS_READY;
		

		setjmp(allthreads[num_running_threads].registers); //save the registers with setjmp

		//Launch start_routine and store the stack pointer
		allthreads[num_running_threads].registers[0].__jmpbuf[JB_PC] = ptr_mangle((unsigned long int)start_thunk);
		allthreads[num_running_threads].registers[0].__jmpbuf[JB_R12] = (unsigned long int)start_routine;
		allthreads[num_running_threads].registers[0].__jmpbuf[JB_R13] = (unsigned long int)arg;
		allthreads[num_running_threads].registers[0].__jmpbuf[JB_RSP] = ptr_mangle((unsigned long int)stack_pointer);


		num_running_threads++;

		//Immediately schedule the new thread we just created
		schedule();
	}

	return 0;
}

void pthread_exit(void *value_ptr)
{
	allthreads[thread_current].status = TS_EXITED; 	//Update thread's status to EXITED

	num_running_threads--;

	if (num_running_threads >= 1)
	{
		schedule();		//schedule() the exited threads
	}
	else
	{
		exit(0);		//Exit if there are no more threads running
	}

	__builtin_unreachable();	
}

pthread_t pthread_self(void)
{
	return allthreads[thread_current].id;		//Just return the curren thread's ID!
}



int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr)
{
	lock();
	struct waiting_threads_mutex *mutexlist = (struct waiting_threads_mutex*)malloc(sizeof(struct waiting_threads_mutex));
	mutexlist->first = NULL;
	mutexlist->last = NULL;
	mutexlist->list_length = 0;
	mutexlist->mutex_status = 0; //0 for unlocked, 1 for locked
	mutexlist->lock_holder = -1;

	mutex->__align = (long int)(mutexlist); //?? __align
	unlock();
	return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	lock();
	struct waiting_threads_mutex *mutexlist = (struct waiting_threads_mutex *)mutex->__align;

	if (mutexlist->mutex_status == 0) //mutex is unlocked so the thread can acquire it
	{
		mutexlist->mutex_status = 1;
		mutexlist->lock_holder = thread_current;
		allthreads[thread_current].status = TS_READY;
	}
	else //mutex is locked so the thread gets blocked
	{
		allthreads[thread_current].status = TS_BLOCKED;

		struct Node *n = (struct Node*)malloc(sizeof(struct Node));
		n->data_id = thread_current;
		n->next = NULL;

		if (mutexlist->list_length == 0)
		{
			mutexlist->first = n;
			mutexlist->last = n;
		}
		else
		{
			mutexlist->last->next = n;
			mutexlist->last = n;
		}

		mutexlist->list_length++;
	}

	unlock();
	if (is_first_call == false)
	{
		schedule();
	}
	
	//unlock();

	return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	lock();
	struct waiting_threads_mutex *mutexlist = (struct waiting_threads_mutex *)mutex->__align;
	struct Node *node_traverse;
	if (mutexlist->mutex_status == 1 && (thread_current == mutexlist->lock_holder)) //if the mutex is currently locked and the lock owner is trying to unlock
	{
		mutexlist->mutex_status = 0; //unlock mutex

		if (mutexlist->list_length > 0)
		{
			node_traverse = mutexlist->first;
			while (node_traverse != NULL)
			{
				if (node_traverse->data_id != -1 && allthreads[node_traverse->data_id].status == TS_BLOCKED) //next thread found
				{
					allthreads[node_traverse->data_id].status = TS_READY;
					mutexlist->mutex_status = 1; //thread acquires lock
					mutexlist->lock_holder = node_traverse->data_id;

					node_traverse->data_id = -1;

					mutexlist->list_length--;
					break;
				}
				node_traverse = node_traverse->next;
			}
		}

		unlock();
		return 0;
	}
	else
	{
		unlock();
		return -1; //return error code
	}
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	lock();
	struct waiting_threads_mutex *mutexlist = (struct waiting_threads_mutex *)mutex->__align;
	struct Node *node_current;
	struct Node *node_next;

	node_current = mutexlist->first;
	node_next = NULL;
	while (node_current != NULL) //free the linked list of waiting threads
	{
		node_next = node_current->next;
		free(node_current);
		node_current = node_next;
	}

	free(mutexlist); //free the mutex
	unlock();

	return 0;
}


int pthread_barrier_init(pthread_barrier_t *restrict barrier, const pthread_barrierattr_t *restrict attr, unsigned count)
{
	lock();
	if (count > 0)
	{
		struct waiting_threads_barrier *barrierlist = (struct waiting_threads_barrier*)malloc(sizeof(struct waiting_threads_barrier));
		barrierlist->first = NULL;
		barrierlist->last = NULL;
		barrierlist->count = count;
		barrierlist->num_boundary = 0;
		barrierlist->barrier_full = 0;

		barrier->__align = (long int)barrierlist;

		unlock();
		return 0;
	}
	else
	{
		unlock();
		return EINVAL;
	}
}

int pthread_barrier_wait(pthread_barrier_t *barrier)
{
	lock();
	struct waiting_threads_barrier *barrierlist = (struct waiting_threads_barrier*)barrier->__align; //Cast to our struct type
	struct Node *node_traverse;
	struct Node *node_current;
	struct Node *node_next;

	if (barrierlist->num_boundary < barrierlist->count) //threads in barrier not reached count
	{
		allthreads[thread_current].status = TS_BLOCKED;
		
		struct Node *n = (struct Node*)malloc(sizeof(struct Node));
		n->data_id = thread_current;
		n->next = NULL;

		if (barrierlist->num_boundary == 0)
		{
			barrierlist->first = n;
			barrierlist->last = n;
		}
		else
		{
			barrierlist->last->next = n;
			barrierlist->last = n;
		}

		barrierlist->num_boundary++;


		unlock();
		if (is_first_call == false)
		{
			schedule(); 
		}
		
		unlock();

		return 0;
	}
	else //threads in barrier reached count
	{		
		node_traverse = barrierlist->first;
		
		while (node_traverse != NULL)
		{
			if (node_traverse->data_id != -1 && allthreads[node_traverse->data_id].status == TS_BLOCKED) //release all threads in barrier
			{
				allthreads[node_traverse->data_id].status = TS_READY;
				barrierlist->num_boundary--;
			}
			node_traverse = node_traverse->next;
		}

		node_current = barrierlist->first;
		node_next = NULL;
		while (node_current != NULL) //'free' the linked list to reset barrier
		{
			node_next = node_current->next;
			node_current->data_id = -1;
			node_current = node_next;
		}

		barrierlist->num_boundary = 0;
		
		unlock();

		barrierlist->barrier_full = 1;

		return PTHREAD_BARRIER_SERIAL_THREAD;
	}
}

int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
	lock();
	struct waiting_threads_barrier *barrierlist = (struct waiting_threads_barrier*)barrier->__align;
	struct Node *node_current;
	struct Node *node_next;

	node_current = barrierlist->first;
	node_next = NULL;

	while (node_current != NULL) //free the linked list of waiting threads
	{
		node_next = node_current->next;
		free(node_current);
		node_current = node_next;
	}
 
	free(barrierlist); //free the barrier
	unlock();
	
	return 0;
}

