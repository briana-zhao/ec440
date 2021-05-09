For this project we implemented a library that provides local storage for threads. Each thread has its own TLS that it can read from/write to. Data can also be shared among multiple threads with the help of Copy-on-Write. To implement this, we created the functions tls_create(), tls_write(), tls_read(), tls_destroy(), and tls_clone(), as well as some helper functions.

External Resources I used:
https://www.tutorialspoint.com/c_standard_library/c_function_memcpy.htm
https://stackoverflow.com/questions/1585186/array-of-pointers-initialization
https://linuxhint.com/using_mmap_function_linux/
https://stackoverflow.com/questions/12076308/why-does-munmap-needs-a-length-as-parameter

____________________________________________________________________________________

tls_protect: This function uses mprotect to protect a page's memory space from being modified.

tls_unprotect: This function uses mprotect to unprotect a page's memory space and allow reading and writing.

tls_handle_page_fault: This function determines if a segmentation fault is caused by a thread trying to access another thread's TLS or if it is caused by a "regular" issue (typical seg fault). It first determines the start address of the page that caused the seg fault, then it loops through all pages to check if a page address matches the page fault address. If so, pthread_exit is called. If not, the seg fault signal is re-raised.

tls_init: This function is called the first time tls_create is called. It initializes the global variable that stores thread ID and TLS pairs. It also sets up the necessary signal handlers.

tls_create: This function creates a TLS for a thread (that doesn't already have a TLS). Calloc is used to allocate space for the TLS's array of page pointers, then it uses mmap to obtain memory for each page. Other attributes such as 'size' and 'page_num' are also set.

tls_destroy: This function destroys the TLS for a thread. It resets the TLS's members to -1, then it loops through all the TLS's pages. If the page is being shared (ref_count is greater than 1) then the page cannot be freed because it is still being used by other threads. Therefore, the page is only freed with munmap if it is not being shared. If it is being shared, then the page's ref_count is simply decremented by 1.

tls_read: This function reads a specified amount of data from a thread's TLS starting at an offset into a buffer. First, every page of the TLS must be unprotected using tls_unprotect. Then a loop is used to calculate a page number and page offset, access that page, then read in a character starting at the offset into a buffer. After the read operation is complete, all pages are reprotected using tls_protect.

tls_write: This function writes a specified amount of data from a buffer into a thread's TLS starting at an offset. First, every page of the TLS must be unprotected using tls_unprotect. Then a loop is used to calculate a page number and page offset then write a character from the buffer into the page starting at the page offset. For each page in the loop, it is checked to see if ref_count is greater than one to determine if the page is being shared among threads. If it is being shared, then a Copy-on-Write is performed on the page. To do this, a new page is created using mmap, then memcpy is used to copy the contents from the original page into the new page. This new page belongs to the current thread only and can now be written to.

tls_clone: This function clones a target thread's TLS and assigns it as the TLS to the current thread (the thread that calls tls_clone). It does error checking to make sure that the current thread doesn't already have a TLS. If it doesn't already have a TLS, then a new TLS is allocated along with an array of page pointers. A loop is used to point each of these page pointers to the target thread's pages. ref_count for each page is incremented because the page is now being shared with the current thread.