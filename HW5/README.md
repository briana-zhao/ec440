For this project we implemented a file system on top of a virtual disk. This included implementing some basic file system calls like fs_read and fs_write. The file system is implemented using inode data structures. The disk has a total of 8,192 blocks, and each block is 4KB in size. The disk can also store a maximum of 64 files.

For this homework assignment I did not use any external resources, but I went to office hours multiple times for help.

____________________________________________________________________________________
make_fs: This function creates a new disk and writes all necessary metadata to the disk. The metadata includes the superblock (which contains information on where inode, directory, and data content can be located on the disk), the directory, and the table of inodes. make_fs also initializes the bitmap that determines which of the 8,192 blocks are free/used.

mount_fs: This function opens a disk then reads in the metadata from disk and stores it in the appropriate global variables. It also initializes the array of file descriptors. 

umount_fs: This function writes all the metadata to their corresponding block locations on disk, then it closes the disk.

fs_open: This function loops through all of the files in directory and finds the one corresponding to the 'name' argument. Once the file is found, it finds an unused file descriptor and assigns it to the file. This new file descriptor is returned from the function.

fs_close: This function loops through all of the files in directory and finds the one corresponding to the 'fildes' argument. If the file is found, then the file descriptor's variables are reset, allowing the file descriptor to be used again in the future. 

fs_create: This function first loops through the directory and finds an open spot to place the new file. This loop also checks if a file of the same name already exists. Then it loops through the table of inodes to find an open spot to place the new inode (for the file that is to be created). If both spots can be found, then the file is created and the inode is assigned to the file.

fs_delete: This function loops through all of the files in directory and finds the one corresponding to the 'name' argument. If the file is still open, then an error is returned. Next, the function loops through the file's block locations and frees them by writing an empty block to disk at each of those locations. The blocks are then marked as 'unused' on the bitmap. Finally, the directory entry is reset and the inode is unassigned.

fs_read: This function loops through all of the files in directory and finds the one corresponding to the 'fildes' argument. Then a buffer is created that is large enough to store all of the file's current data and the file's blocks are read in from disk into this buffer. Starting at the file's offset, nbytes of data are copied from the buffer to 'buf'. Finally, the file's offset is incremented by nbytes.

fs_write: This function loops through all of the files in directory and finds the one corresponding to the 'fildes' argument. It is determined how many extra blocks the file will need to store the nbytes of new data. A loop is used to go through all the data blocks and find the necessary amount of free blocks to then assign to the file. Then a buffer is created that is large enough to store all of the file's current and new data. Next, all of the file's current blocks are read into this buffer from disk and nbytes of data from 'buf' are stored into this buffer starting at the file's offset. Finally, all the blocks are written back onto disk and the file's offset is incremented by nbytes.

fs_get_filesize:This function loops through all of the files in directory and finds the one corresponding to the 'fildes' argument. Then it returns the size of that file.

fs_listfiles: This function loops through all the files in directory, and if the file is being used, then its name is stored into an array of filenames. A null pointer is added after the last filename. 

fs_lseek: This function loops through all of the files in directory and finds the one corresponding to the 'fildes' argument. Then it sets this file's offset to the 'offset' argument. If the offset is less than 0 or greater than the file's size, then an error is returned.

fs_truncate: This function loops through all of the files in directory and finds the one corresponding to the 'fildes' argument. An error is returned if the argument 'length' is greater than the file's size (fs_truncate cannot be used to extend files). It is determined what the file's new number of blocks will be after truncating. Then a loop is used to go through the file's block locations, and for any blocks that need to be removed, an empty block is written to disk at those block locations. The blocks are then marked as 'unused' on the bitmap.
