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

/* thread_status identifies the current state of a thread. You can add, rename,
 * or delete these values. This is only a suggestion. */
enum thread_status
{
	TS_EXITED,
	TS_RUNNING,
	TS_READY,
	TS_STANDBY
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

//Create an array of threads (an array of the struct thread_control_block)
struct thread_control_block allthreads[MAX_THREADS];


int thread_current = 0;	//Keeps track of the current running thread
int num_running_threads = 0; //Keeps track of the current number of running (active) threads
int allthreads_index = 0;
static bool is_first_call = true;

static void schedule()//(int signal)
{
	/* TODO: implement your round-robin scheduler 
	 * 1. Use setjmp() to update your currently-active thread's jmp_buf
	 *    You DON'T need to manually modify registers here.
	 * 2. Determine which is the next thread that should run
	 * 3. Switch to the next thread (use longjmp on that thread's jmp_buf)
	 */
	
	
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
			}
		}
		
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
	/* TODO: do everything that is needed to initialize your scheduler. For example:
	 * - Allocate/initialize global threading data structures
	 * - Create a TCB for the main thread. Note: This is less complicated
	 *   than the TCBs you create for all other threads. In this case, your
	 *   current stack and registers are already exactly what they need to be!
	 *   Just make sure they are correctly referenced in your TCB.
	 * - Set up your timers to call schedule() at a 50 ms interval (SCHEDULER_INTERVAL_USECS)
	 */
	
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
	// Create the timer and handler for the scheduler. Create thread 0.
	// if (num_running_threads > 1)
	// {
	// 	is_first_call = false;
	// }

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

	/* TODO: Return 0 on successful thread creation, non-zero for an error.
	 *       Be sure to set *thread on success.
	 * Hints:
	 * The general purpose is to create a TCB:
	 * - Create a stack.
	 * - Assign the stack pointer in the thread's registers. Important: where
	 *   within the stack should the stack pointer be? It may help to draw
	 *   an empty stack diagram to answer that question.
	 * - Assign the program counter in the thread's registers.
	 * - Wait... HOW can you assign registers of that new stack? 
	 *   1. call setjmp() to initialize a jmp_buf with your current thread
	 *   2. modify the internal data in that jmp_buf to create a new thread environment
	 *      env->__jmpbuf[JB_...] = ...
	 *      See the additional note about registers below
	 *   3. Later, when your scheduler runs, it will longjmp using your
	 *      modified thread environment, which will apply all the changes
	 *      you made here.
	 * - Remember to set your new thread as TS_READY, but only  after you
	 *   have initialized everything for the new thread.
	 * - Optionally: run your scheduler immediately (can also wait for the
	 *   next scheduling event).
	 */
	/*
	 * Setting registers for a new thread:
	 * When creating a new thread that will begin in start_routine, we
	 * also need to ensure that `arg` is passed to the start_routine.
	 * We cannot simply store `arg` in a register and set PC=start_routine.
	 * This is because the AMD64 calling convention keeps the first arg in
	 * the EDI register, which is not a register we control in jmp_buf.
	 * We provide a start_thunk function that copies R13 to RDI then jumps
	 * to R12, effectively calling function_at_R12(value_in_R13). So
	 * you can call your start routine with the given argument by setting
	 * your new thread's PC to be ptr_mangle(start_thunk), and properly
	 * assigning R12 and R13.
	 *
	 * Don't forget to assign RSP too! Functions know where to
	 * return after they finish based on the calling convention (AMD64 in
	 * our case). The address to return to after finishing start_routine
	 * should be the first thing you push on your stack.
	 */
	return 0;
}

void pthread_exit(void *value_ptr)
{
	/* TODO: Exit the current thread instead of exiting the entire process.
	 * Hints:
	 * - Release all resources for the current thread. CAREFUL though.
	 *   If you free() the currently-in-use stack then do something like
	 *   call a function or add/remove variables from the stack, bad things
	 *   can happen.
	 * - Update the thread's status to indicate that it has exited
	 */

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
	/* TODO: Return the current thread instead of -1
	 * Hint: this function can be implemented in one line, by returning
	 * a specific variable instead of -1.
	 */
	return allthreads[thread_current].id;		//Just return the curren thread's ID!
}

/* Don't implement main in this file!
 * This is a library of functions, not an executable program. If you
 * want to run the functions in this file, create separate test programs
 * that have their own main functions.
 */
