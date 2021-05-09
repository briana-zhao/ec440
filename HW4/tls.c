#include "tls.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>


/*
 * This is a good place to define any data structures you will use in this file.
 * For example:
 *  - struct TLS: may indicate information about a thread's local storage
 *    (which thread, how much storage, where is the storage in memory)
 *  - struct page: May indicate a shareable unit of memory (we specified in
 *    homework prompt that you don't need to offer fine-grain cloning and CoW,
 *    and that page granularity is sufficient). Relevant information for sharing
 *    could be: where is the shared page's data, and how many threads are sharing it
 *  - Some kind of data structure to help find a TLS, searching by thread ID.
 *    E.g., a list of thread IDs and their related TLS structs, or a hash table.
 */

/*
 * Now that data structures are defined, here's a good place to declare any
 * global variables.
 */

/*
 * With global data declared, this is a good point to start defining your
 * static helper functions.
 */

/*
 * Lastly, here is a good place to add your externally-callable functions.
 */ 

#define MAX_THREAD_COUNT 128

struct TLS
{
	pthread_t tid;			//id of thread that owns this TLS
	unsigned int size;		//size in bytes
	unsigned int page_num;	//number of pages
	struct page **pages;	//array of pointers to pages
};

struct page
{
	unsigned long int address;	//start address of page
	int ref_count;				//counter for shared pages
};

struct tid_tls_pair
{
	pthread_t tid;
	struct TLS *tls;
};

//---------Global Variables---------//
int page_size;
bool initialize = false;

struct tid_tls_pair tid_tls_pairs[MAX_THREAD_COUNT]; //global variable to keep track of all thread IDs and the corresponding TLS


//---------Helper Functions---------//
void tls_protect(struct page* p)
{
	if (mprotect((void *) p->address, page_size, PROT_NONE))
	{
		fprintf(stderr, "tls_protect: could not protect page\n");
		exit(1);
	}
}

void tls_unprotect(struct page* p)
{
	if (mprotect((void *) p->address, page_size, PROT_READ | PROT_WRITE))
	{
		fprintf(stderr, "tls_unprotect: could not unprotect page\n");
		exit(1);
	}
}

void tls_handle_page_fault(int sig, siginfo_t *si, void *context)
{
	unsigned long int p_fault = ((unsigned long int) si->si_addr) & ~(page_size - 1); //start address of the page that caused the seg fault

	for (int i = 0; i < MAX_THREAD_COUNT; i++) //loop through array of TIDs and TLSs
	{
		for (int j = 0; j < tid_tls_pairs[j].tls->page_num; j++) //loop through all pages of each TLS
		{
			if (tid_tls_pairs[i].tls->pages[j]->address == p_fault) //does the page's address match the page fault address?
			{
				printf("Seg fault caused by thread... exiting thread...\n");
				pthread_exit(NULL);
			}
		}
	}

	printf("Regular seg fault... re-raising signal...\n");
	signal(SIGSEGV, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	raise(sig);
}

void tls_init()
{
	page_size = getpagesize();

	struct sigaction sigact;

	/* Handle page faults (SIGSEGV, SIGBUS) */
	sigemptyset(&sigact.sa_mask);

	/* Give context to handler */
	sigact.sa_flags = SA_SIGINFO;
	sigact.sa_sigaction = tls_handle_page_fault;
	sigaction(SIGBUS, &sigact, NULL);
	sigaction(SIGSEGV, &sigact, NULL);

	for (int i = 0; i < MAX_THREAD_COUNT; i++)
	{
		tid_tls_pairs[i].tid = -1; //For now, set all thread IDs to -1
	}

	initialize = true;

}


int tls_create(unsigned int size)
{
	if (initialize == false) //for first time tls_create is called
	{
		tls_init();
	}

	//Error checking----

	if (size < 0)
	{
		return -1;
	}

	bool current_has_tls = false;
	for(int i = 0; i < MAX_THREAD_COUNT; i++) //check if the thread already has a LSA
	{
		if (tid_tls_pairs[i].tid == pthread_self())
		{
			current_has_tls = true;
		}
	}

	if (current_has_tls == true)
	{
		return -1;
	}
	
	//-------------------

	int new_tls_index = -1;
	struct TLS* new_tls = calloc(1, sizeof(struct TLS)); //calloc size for new TLS

	for (int i = 0; i < MAX_THREAD_COUNT; i++) //need to find next open spot to store TID and TLS pair
	{
		if (tid_tls_pairs[i].tid == -1) //then the spot is available
		{
			new_tls_index = i;
			break;
		}
	}

	//assign the correct properties for the new TLS
	new_tls->size = size; 
	new_tls->page_num = ((size - 1) / (page_size)) + 1; // - 1 ??
	new_tls->pages = (struct page**)calloc(new_tls->page_num, sizeof(struct page *)); //calloc the array of pages
	new_tls->tid = pthread_self();

	for (int i = 0; i < new_tls->page_num; i++)
	{
		struct page *pg = (struct page *)calloc(1, sizeof(struct page *)); //calloc the page
		pg->address = (unsigned long int)mmap(0, page_size, PROT_NONE, MAP_ANON | MAP_PRIVATE, 0, 0); //obtain memory for the page
		pg->ref_count = 1; //set ref_count to 1 (only this thread is currently using the page)
		new_tls->pages[i] = pg; //place the page in the array of pages
	}	

	tid_tls_pairs[new_tls_index].tls = new_tls;
	tid_tls_pairs[new_tls_index].tid = pthread_self();

	return 0;
}

int tls_destroy()
{

	//Error checking-----------------
	bool current_has_tls = false;
	int current_tls_index = -1;

	for (int i = 0; i < MAX_THREAD_COUNT; i++) //check that the current thread has a TLS to destroy
	{
		if (tid_tls_pairs[i].tid == pthread_self())
		{
			current_has_tls = true;
			current_tls_index = i;
			break;
		}
	}

	if (current_has_tls == false)
	{
		return -1;
	}

	//-------------------------------

	//reset some properties back to -1
	tid_tls_pairs[current_tls_index].tid = -1;
	tid_tls_pairs[current_tls_index].tls->tid = -1;
	tid_tls_pairs[current_tls_index].tls->size = -1;

	for (int i = 0; i < tid_tls_pairs[current_tls_index].tls->page_num; i++) //loop through all the pages
	{
		if (tid_tls_pairs[current_tls_index].tls->pages[i]->ref_count > 1) //if page is being shared
		{
			tid_tls_pairs[current_tls_index].tls->pages[i]->ref_count--; //then decrement ref_count
		}
		else //if page is not being shared (ref_count == 1)
		{
			munmap((void *)tid_tls_pairs[current_tls_index].tls->pages[i]->address, page_size); //then munmap the memory
		}
	}

	tid_tls_pairs[current_tls_index].tls->page_num = -1;
	free(tid_tls_pairs[current_tls_index].tls->pages);	

	return 0;
}

int tls_read(unsigned int offset, unsigned int length, char *buffer)
{

	//Error checking--------------------

	bool current_has_tls = false;
	int current_tls_index = -1;

	for (int i = 0; i < MAX_THREAD_COUNT; i++) //check that current thread has LSA
	{
		if (tid_tls_pairs[i].tid == pthread_self())
		{
			current_has_tls = true;
			current_tls_index = i;
			break;
		}
	}

	if (current_has_tls == false)
	{
		return -1;
	}

	if ((offset + length) > (tid_tls_pairs[current_tls_index].tls->page_num * page_size)) //check if offset + length > size of TLS
	{
		return -1;
	}

	//---------------------------------

	for (int i = 0; i < tid_tls_pairs[current_tls_index].tls->page_num; i++) //unprotect all of the TLS's pages
	{
		tls_unprotect(tid_tls_pairs[current_tls_index].tls->pages[i]);
	}


	int idx, cnt;
	for(cnt = 0, idx = offset; idx < (offset + length); ++cnt, ++idx)
	{
		//calculate page number and page offset within the current page
		unsigned int page_number, page_offset;
		page_number = idx / page_size;
		page_offset = idx % page_size;

		struct page *pg;
		pg = tid_tls_pairs[current_tls_index].tls->pages[page_number]; //pg points to page indicated by page_number

		char *read_location = ((char *) pg->address) + page_offset; //points to where data to be read starts
		buffer[cnt] = *read_location;	//store into buffer
	}

	for (int i = 0; i < tid_tls_pairs[current_tls_index].tls->page_num; i++) //reprotect all of the TLS's pages again
	{
		tls_protect(tid_tls_pairs[current_tls_index].tls->pages[i]);
	}

	return 0;
}

int tls_write(unsigned int offset, unsigned int length, const char *buffer)
{
	//Error checking----------------

	bool current_has_tls = false;
	int current_tls_index = -1;

	for (int i = 0; i < MAX_THREAD_COUNT; i++) //check if current thread has TLS
	{
		if (tid_tls_pairs[i].tid == pthread_self())
		{
			current_has_tls = true;
			current_tls_index = i;
			break;
		}
	}

	if (current_has_tls == false)
	{
		return -1;
	}

	if ((offset + length) > (tid_tls_pairs[current_tls_index].tls->page_num * page_size)) //check if offset + length > size of TLS
	{
		return -1;
	}

	//----------------------------------

	for (int i = 0; i < tid_tls_pairs[current_tls_index].tls->page_num; i++) //unprotect all the TLS's pages
	{
		tls_unprotect(tid_tls_pairs[current_tls_index].tls->pages[i]);
	}

	int idx, cnt;
	for(cnt = 0, idx = offset; idx < (offset + length); ++cnt, ++idx)
	{
		//calculate page number and page offset within the current page
		int page_number, page_offset;
		page_number = idx / page_size;
		page_offset = idx % page_size;

		struct page *pg;
		pg = tid_tls_pairs[current_tls_index].tls->pages[page_number]; //pg points to page indicated by page_number

		if (pg->ref_count > 1) //Page is shared so need to do CoW!
		{
			pg->ref_count--;

			unsigned long int page_copy;
			page_copy = (unsigned long int)mmap(0, page_size, PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0); //obtain memory for the new page copy, allow writing to this memory

			memcpy((void *)page_copy, (void *)pg->address, page_size); //copy the orignal page's contents into the new page's memory space

			tls_protect(pg); //protect the old page because we only want to write to the copy

			tid_tls_pairs[current_tls_index].tls->pages[page_number] = (struct page *)calloc(1, sizeof(struct page)); //calloc space for the page
			tid_tls_pairs[current_tls_index].tls->pages[page_number]->address = page_copy; //put the mmapped memory in 'address'
			tid_tls_pairs[current_tls_index].tls->pages[page_number]->ref_count = 1; 
			
			pg = tid_tls_pairs[current_tls_index].tls->pages[page_number]; //point pg to the newly copied page
			pg->address = page_copy; //redirect pg->address to page_copy
		}

		char *write_location = ((char *) pg->address) + page_offset; //points to the starting position to be written to
		*write_location = buffer[cnt]; //store next char from buffer
	}

	for (int i = 0; i < tid_tls_pairs[current_tls_index].tls->page_num; i++) //reprotect all of the TLS's pages again
	{
		tls_protect(tid_tls_pairs[current_tls_index].tls->pages[i]);
	}

	return 0;
}

int tls_clone(pthread_t tid)
{
	//Error checking---------------------------------------

	bool current_has_tls = false;
	int target_tls_index = -1;

	for (int i = 0; i < MAX_THREAD_COUNT; i++) //check if current thread has TLS and find the index of the target thread
	{
		if (tid_tls_pairs[i].tid == pthread_self())
		{
			current_has_tls = true;
		}
		else if (tid_tls_pairs[i].tid == tid)
		{
			target_tls_index = i;
		}
	}

	if (target_tls_index == -1 || current_has_tls == true)
	{
		if (target_tls_index == -1)
		{
			printf("target thread has no tls!\n");
		}
		if (current_has_tls == true)
		{
			printf("current thread already has tls!\n");
		}
		return -1;
	}

	int open_spot = -1;
	for (int i = 0; i < MAX_THREAD_COUNT; i++) //need to find next open spot to store TID and (cloned) TLS pair
	{
		if (tid_tls_pairs[i].tid == -1) //then the spot is available
		{
			open_spot = i;
			break;
		}
	}

	if (open_spot == -1)
	{
		printf("no open spot!\n");
		return -1;
	}

	//-------------------------------------------

	struct TLS* new_tls = (struct TLS*)calloc(1, sizeof(struct TLS)); //calloc space for new TLS

	new_tls->page_num = tid_tls_pairs[target_tls_index].tls->page_num; //match number of pages to target thread's
	new_tls->pages = (struct page**)calloc(new_tls->page_num, sizeof(struct page*)); //calloc space for an array of pages
	new_tls->size = tid_tls_pairs[target_tls_index].tls->size; //match size to target thread's
	new_tls->tid = pthread_self(); //current thread's ID

	for (int i = 0; i < new_tls->page_num; i++)
	{
		new_tls->pages[i] = tid_tls_pairs[target_tls_index].tls->pages[i]; //point each page ptr to target thread's corresponding page
		new_tls->pages[i]->ref_count++; //page is now being shared so increment ref_count
	}

	//store the new TID/TLS pair
	tid_tls_pairs[open_spot].tid = pthread_self();
	tid_tls_pairs[open_spot].tls = new_tls;

	return 0;
}


//Used for debugging
int print_tls()
{
	for (int i = 0; i < MAX_THREAD_COUNT; i++)
	{
		printf("Global TLS index %d tid: %ld\n", i, tid_tls_pairs[i].tid);
	}

	return 0;
}

