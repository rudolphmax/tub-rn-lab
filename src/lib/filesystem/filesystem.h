#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define BLOCK_SIZE 1024
#define NAME_MAX_LENGTH 32
#define DIRECT_BLOCKS_COUNT 12

enum node_type{
	reg_file=1,
	directory=2,
	free_block=3
};

typedef struct _data_block{
	size_t size;
	uint8_t block[BLOCK_SIZE];
} data_block;

/*
 * The direct_blocks can either point to other inode, in case this inode is a directory
 * or to data_blocks, in case this is a regular file
 */
typedef struct _inode {
	enum node_type n_type;
	uint16_t size;
	char name[NAME_MAX_LENGTH];
	int direct_blocks[DIRECT_BLOCKS_COUNT]; //Block numbers. -1 if there is no block
	int parent; //inode number of parent
} inode;

typedef struct _superblock{
	uint32_t num_blocks;
	uint32_t free_blocks;
} superblock;

typedef struct _fs{
	superblock* s_block;
	uint8_t * free_list; //free == 1
	inode * inodes;	
	data_block* data_blocks;
	int root_node; //inode-number of root node
}file_system ;

/**
	* Allocates memory for a filesystem and loads an existing filesystem from a .fs-file.
	* @param const char* path to the fs-file
	* @return pointer to a fs-struct 
**/
file_system* fs_load(const char* fs_file_path);


/**
	* creates a new file system file
	* including Superblock, free list, space for inodes etc
	* @param const char* fs_file_path path and name to file
	* @param uint32_t size Amount of 1024-Byte-Blocks in the filesystem
	* @return pointer to fs struct
**/
file_system *fs_create(uint32_t size);

/*
 * dumps the filesystem to harddrive
 * @param file_system* fs the filesystem to dump
 * @param const char* file_path where to put the file on the harddrive
 * @return 0 on success, -1 else
 */
int fs_dump(file_system* fs, const char* file_path);


/*
	* Initialize an empty inode
*/
void inode_init(inode* i);
/*
	* find free inode and return its number or -1 if there is no free inode
*/
int find_free_inode(file_system* fs);

/*
	* frees up memory
*/
void fs_free(file_system* fs);
#ifdef DEBUG
	#define LOG_INODE(i) fprintf(stderr,"INODE\nType: %d\nName: %s\n",i.n_type,i.name);
#else
	#define LOG_INODE(i)	;

#endif

#endif //FILESYSTEM_H
