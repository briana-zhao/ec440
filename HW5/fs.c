#include "disk.h"
#include "fs.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_FILE_DESCRIPTORS 32
#define MAX_FILE_NAME 16
#define BLOCK_SIZE 4096
#define MAX_FILES 64
#define MAX_FILE_SIZE 1024 * 1024   //1 Mebibyte
#define TOTAL_BLOCKS 8192

struct super_block {
    int inode_offset;
    int inode_length;
    int directory_offset;
    int directory_length;
    int data_offset;
};

struct inode {
    bool is_used;
    char name[MAX_FILE_NAME];
    int size;
    int offset;
    int block_locations[256];
};

struct dir_entry {
    bool is_used;
    int inode_number;
    char name[MAX_FILE_NAME];
};

struct file_descriptor {
    bool is_used;
    int inode_number;
    int offset;
};


int block_bitmap[TOTAL_BLOCKS];
struct super_block sblock;
struct dir_entry directory[MAX_FILES];
struct file_descriptor filedesc[MAX_FILE_DESCRIPTORS];
struct inode inode_table[MAX_FILES];


int make_fs(const char *disk_name)
{
    if (make_disk(disk_name) != 0)
    {
        return -1;
    }

    if (open_disk(disk_name) != 0)
    {
        return -1;
    }

    //Create the superblock
    sblock.inode_offset = 1;
    sblock.inode_length = MAX_FILES;
    sblock.directory_offset = sblock.inode_offset + sblock.inode_length;
    sblock.directory_length = (sizeof(struct dir_entry) * MAX_FILES) / BLOCK_SIZE;
    sblock.data_offset = sblock.directory_offset + sblock.directory_length + 1;


    char *block_holder = calloc(1, BLOCK_SIZE);     //a buffer that is the size of a block

    memcpy((void *) block_holder, (void *) &sblock, sizeof(struct super_block));    
    block_write(0, block_holder);   //write the superblock to disk


    memcpy((void *) block_holder, (void *) &directory, sizeof(struct dir_entry) * MAX_FILES); 
    block_write(sblock.directory_offset, block_holder); //write the directory to disk


    for (int i = 0; i < sblock.inode_length; i++)
    {
        memcpy((void *) block_holder, (void *) &inode_table[i], sizeof(struct inode));
        block_write(sblock.inode_offset + i, block_holder); //write each index of inode_table to disk (each gets own block)
    }


    for (int i = 0; i < 8192; i++) //Prepare the bitmap. Initially all data blocks are free
    {
        if (i < sblock.data_offset)
        {
            block_bitmap[i] = 1; //used
        }
        else
        {
            block_bitmap[i] = 0; //free
        }
    }

    if (close_disk(disk_name) != 0)
    {
        return -1;
    }

    return 0;
}

int mount_fs(const char *disk_name)
{
    if (open_disk(disk_name) != 0)
    {
        return -1;
    }

    char *block_holder = calloc(1, BLOCK_SIZE);
    
    block_read(0, block_holder);    //Read in the block containing the superblock from disk
    memcpy((void *) &sblock, (void *) block_holder, sizeof(struct super_block));    //store into sblock global variable
    
    block_read(sblock.directory_offset, block_holder);      //Read in the block containing the directory from disk
    memcpy((void *) &directory, (void *) block_holder, sizeof(struct dir_entry) * MAX_FILES);       //store into directory global variable
    
    for (int i = 0; i < sblock.inode_length; i++)
    {
        block_read(sblock.inode_offset + i, block_holder);
        memcpy((void *) &inode_table[i], (void *) block_holder, sizeof(struct inode)); //Read in each block of the inode_table and store in the appropriate index
    }

    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) //Prepare the file descriptors
    {
        filedesc[i].is_used = false;
        filedesc[i].inode_number = -1;
        filedesc[i].offset = 0;
    }

    for (int i = 0; i < MAX_FILES; i++) //Prepare each inode's block locations (each has no blocks initially)
    {
        for (int j = 0; j < 256; j++)
        {
            inode_table[i].block_locations[j] = -1;
        }
    }

    free(block_holder);
    return 0;
}

int umount_fs(const char *disk_name)
{
    char *block_holder = calloc(1, BLOCK_SIZE);

    memcpy((void *) block_holder, (void *) &sblock, sizeof(struct super_block));
    block_write(0, block_holder);   //write the superblock to disk

    memcpy((void *) block_holder, (void *) &directory, sizeof(struct dir_entry) * MAX_FILES);
    block_write(sblock.directory_offset, block_holder); //write the directory to disk

    for (int i = 0; i < sblock.inode_length; i++)
    {
        memcpy((void *) block_holder, (void *) &inode_table[i], sizeof(struct inode));
        block_write(sblock.inode_offset + i, block_holder); //write each index of inode_table to disk
    }

    if (close_disk(disk_name) != 0)
    {
        return -1;
    }

    return 0;
}

int fs_open(const char *name)
{
    for (int i = 0; i < MAX_FILES; i++) //Loop through all files...
    {
        if (strcmp(directory[i].name, name) == 0) //correct file found, now find an open file descriptor
        {
            for (int j = 0; j < MAX_FILE_DESCRIPTORS; j++) //Loop through all file descriptors
            {
                if (filedesc[j].is_used == false)
                {
                    filedesc[j].is_used = true;
                    filedesc[j].inode_number = directory[i].inode_number;
                    filedesc[j].offset = 0;

                    return j; //j is the new file descriptor
                }
            }
        }
    }

    return -1; 
}

int fs_close(int fildes)
{
    if (fildes < 0 || filedesc[fildes].is_used == false || fildes > MAX_FILE_DESCRIPTORS)
    {
        return -1;
    }

    for (int i = 0; i < MAX_FILES; i++) //Loop through all files...
    {
        if (directory[i].inode_number == filedesc[fildes].inode_number) //Find the file that corresponds to fildes
        {
            filedesc[fildes].is_used = false;
            filedesc[fildes].inode_number = -1;
            filedesc[fildes].offset = 0;
            return 0;
        }
    }
    
    return -1;
}

int fs_create(const char *name)
{
    if (strlen(name) > MAX_FILE_NAME - 1) //file name is too long
    {
        return -1;
    }

    int open_spot = -1;
    bool found_open = false;

    for (int i = 0; i < MAX_FILES; i++) //Loop through directory
    {
        if (found_open == false && directory[i].is_used == false) //Find spot to place the new file
        {
            found_open = true;
            open_spot = i;
        }

        if (strcmp(directory[i].name, name) == 0) //Error if the file already exists
        {
            return -1;
        }
    }

    if (found_open == false) //No more space to place a new file
    {
        return -1;
    }

    int open_inode_spot = -1;

    for (int i = 0; i < MAX_FILES; i++) //Loop through inode_table, find an open spot
    {
        if (inode_table[i].is_used == false)
        {
            open_inode_spot = i;
            break;
        }
    }

    //Create the file
    directory[open_spot].is_used = true;
    strcpy(directory[open_spot].name, name);
    directory[open_spot].inode_number = open_inode_spot;

    //Assign an inode to the file
    inode_table[open_inode_spot].is_used = true;
    strcpy(inode_table[open_inode_spot].name, name);
    inode_table[open_inode_spot].size = 0;
    inode_table[open_inode_spot].offset = 0;
    return 0;
}

int fs_delete(const char *name)
{
    bool file_exists = false;
    int file_location = -1;

    for (int i = 0; i < MAX_FILES; i++) //Loop through files, find file that matches 'name'
    {
        if (strcmp(directory[i].name, name) == 0)
        {
            file_exists = true;
            file_location = i;

            for (int j = 0; j < MAX_FILE_DESCRIPTORS; j++) //check if the file is open
            {
                if (filedesc[j].inode_number == directory[i].inode_number)
                {
                    return -1; //if file is open, can't delete it
                }
            }
        }
    }

    if (file_exists == false)
    {
        return -1;
    }

    char *empty_block = calloc(1, BLOCK_SIZE);
    int inode_location = directory[file_location].inode_number;

    for (int i = 0; i < 256; i++)   //Loop through the inode's blocks and free blocks where data is stored
    {
        if (inode_table[inode_location].block_locations[i] != -1)
        {
            block_write(inode_table[inode_location].block_locations[i], empty_block); //clear the block by writing the empty block to disk
            block_bitmap[inode_table[inode_location].block_locations[i]] = 0; //clear the bit in the bitmap
            inode_table[inode_location].block_locations[i] = -1;    //remove the block from block_locations
        }
    }

    //delete the file from directory
    directory[file_location].is_used = false;
    directory[file_location].inode_number = -1;
    strcpy(directory[file_location].name, "");
    
    //unassign the inode
    inode_table[inode_location].is_used = false;
    strcpy(inode_table[inode_location].name, "");
    inode_table[inode_location].size = 0;
    inode_table[inode_location].offset = 0;

    return 0;
}

int fs_read(int fildes, void *buf, size_t nbyte)
{
    if (fildes < 0 || filedesc[fildes].is_used == false || fildes > MAX_FILE_DESCRIPTORS)
    {
        return -1;
    }

    int directory_location = -1;

    for (int i = 0; i < MAX_FILES; i++) //Loop through files, find file corresponding to fildes
    {
        if (directory[i].is_used == true)
        {
            if (directory[i].inode_number == filedesc[fildes].inode_number)
            {
                directory_location = i;
                break;
            }
        }
    }

    int inode_location = directory[directory_location].inode_number;
    char *buffer = calloc(1, BLOCK_SIZE * ((inode_table[inode_location].size - 1) / BLOCK_SIZE + 1)); //buffer will store all the current file's data

    int block = 0;

    for (int i = 0; i < ((inode_table[inode_location].size - 1) / BLOCK_SIZE + 1); i++)
    {
        block_read(inode_table[inode_location].block_locations[block],  buffer + i * BLOCK_SIZE); //reading all of file's original data into buffer, block by block
        block++;
    }

    memcpy(buf, (void *) (buffer + filedesc[fildes].offset), nbyte); //copy nbytes of data from buffer to 'buf, starting at the file's offset

    filedesc[fildes].offset += nbyte;    //adjust the file's offset

    return nbyte;
}

int fs_write(int fildes, void *buf, size_t nbyte)
{
    if (fildes < 0 || filedesc[fildes].is_used == false || fildes > MAX_FILE_DESCRIPTORS)
    {
        return -1;
    }

    int directory_location = -1;

    for (int i = 0; i < MAX_FILES; i++) //Loop through files, find file corresponding to fildes
    {
        if (directory[i].is_used == true)
        {
            if (directory[i].inode_number == filedesc[fildes].inode_number)
            {
                directory_location = i;
                break;
            }
        }
    }

    if (filedesc[fildes].offset + nbyte > MAX_FILE_SIZE) //if offset + nbyte > 1024 * 1024, then 'nbyte' is adjusted to just write until the file is full
    {
        nbyte = MAX_FILE_SIZE - filedesc[fildes].offset;
    }

    int inode_location = directory[directory_location].inode_number;

    int num_new_blocks_needed = nbyte / BLOCK_SIZE + 1; //number of new blocks the file will need to store the new data
    int next_block_location = -1;

    for (int i = 0; i < 256; i++)   //Loop through file's blocks, find next spot to store the next block location
    {
        if (inode_table[inode_location].block_locations[i] == -1)
        {
            next_block_location = i;
            break;
        }
    }

    if (filedesc[fildes].offset + nbyte > inode_table[inode_location].size) //file's size will increase by nbytes
    {
        inode_table[inode_location].size = filedesc[fildes].offset + nbyte;
    }

    char *buffer = calloc(1, BLOCK_SIZE * ((inode_table[inode_location].size - 1) / BLOCK_SIZE + 1)); //buffer will store all the current file's data
    int block = 0;

    for (int i = 0; i < ((inode_table[inode_location].size - 1) / BLOCK_SIZE + 1); i++)
    {
        block_read(inode_table[inode_location].block_locations[block],  buffer + i * BLOCK_SIZE); //reading all of file's original data into buffer, block by block
        block++;
    }


    int next_free_data_block = -1;
    int added_blocks = 0;
    int j = 0;

    for (int i = sblock.data_offset; i < TOTAL_BLOCKS; i++) //Looping through all data blocks
    {
        if (block_bitmap[i] == 0)   //find a data block that is not being used
        {
            next_free_data_block = i;
        }

        inode_table[inode_location].block_locations[next_block_location + j] = next_free_data_block; //"give" the free data block to the file
        block_bitmap[i] = 1;
        j++;
        added_blocks++;

        if (added_blocks == num_new_blocks_needed) //stop when we've added enough blocks
        {
            break;
        }
    }

    memcpy((void *) (buffer + filedesc[fildes].offset), buf, nbyte); //Copy nbytes from 'buf' to 'buffer' starting at the file's offset

    block = 0;
    for (int i = 0; i < ((inode_table[inode_location].size - 1) / BLOCK_SIZE + 1); i++)
    {
        block_write(inode_table[inode_location].block_locations[block],  buffer + i * BLOCK_SIZE); //Write all the blocks back to disk (now containing the newly written nbytes)
        block++;
    }

    filedesc[fildes].offset += nbyte; //adjust file's offset

    return nbyte;
}

int fs_get_filesize(int fildes)
{
    if (filedesc[fildes].is_used == false) //Error if fildes is not being used
    {
        return -1;
    }

    for (int i = 0; i < MAX_FILES; i++) //Loop through files, find file corresponding to fildes
    {
        if (directory[i].inode_number == filedesc[fildes].inode_number)
        {
            return inode_table[directory[i].inode_number].size;
        }
    }

    return -1;
}

int fs_listfiles(char ***files)
{
    char *filenames[65];

    int j = 0;

    for (int i = 0; i < MAX_FILES; i++) //Loop through files
    {
        if (directory[i].is_used == true)   //Store names of the directories being used
        {
            filenames[j] = directory[i].name;
            j++;
        }
    }


    filenames[j] = NULL; //Store null pointer after last file name

    *files = filenames;

    return 0;
}

int fs_lseek(int fildes, off_t offset)
{
    if (fildes < 0 || filedesc[fildes].is_used == false || fildes > MAX_FILE_DESCRIPTORS)
    {
        return -1;
    }

    int location = -1;

    for (int i = 0; i < MAX_FILES; i++) //Loop through files, find file corresponding to fildes
    {
        if (directory[i].inode_number == filedesc[fildes].inode_number)
        {
            location = i;
        }
    }

    if (offset > inode_table[directory[location].inode_number].size || offset < 0) //Offset can't be < 0 or greater than the file's size
    {
        return -1;
    }

    filedesc[fildes].offset = offset; //set file's offset to new 'offset'

    return 0;
}

int fs_truncate(int fildes, off_t length)
{
    if (filedesc[fildes].is_used == false) //Error if fildes is not being used
    {
        return -1;
    }

    int directory_location = -1;

    for (int i = 0; i < MAX_FILES; i++) //Loop through files, find file corresponding to fildes
    {
        if (directory[i].inode_number == filedesc[fildes].inode_number)
        {
            directory_location = i;
            break;
        }
    }

    if (directory_location == -1 || length > inode_table[directory[directory_location].inode_number].size) //Error if file can't be found or if 'length' is greater than file's size (can't extend file with fs_truncate)
    {
        return -1;
    }

    if (inode_table[directory[directory_location].inode_number].size == 0)
    {
        return 0;
    }

    inode_table[directory[directory_location].inode_number].size = length; //Set new size to 'length'
    if (filedesc[fildes].offset > length) //If offset (the file pointer) > 'length'...
    {
        filedesc[fildes].offset = length;   //...set offset to 'length'
    }

    int new_number_of_blocks = (length / BLOCK_SIZE) + 1; //file's new number of blocks after truncating

    char *empty_block = calloc(1, BLOCK_SIZE);
    int inode_location = directory[directory_location].inode_number;

    for (int i = 0; i < 256; i++) //Loop through the file's block_locations
    {
        if (i >= new_number_of_blocks && inode_table[inode_location].block_locations[i] != -1) //Free the excess blocks
        {
            block_write(inode_table[inode_location].block_locations[i], empty_block); //clear the block by writing the empty block to disk
            block_bitmap[inode_table[inode_location].block_locations[i]] = 0; //clear the bit in the bitmap
            inode_table[inode_location].block_locations[i] = -1;   //remove the block from block_locations
        }
    }

    return 0;
}