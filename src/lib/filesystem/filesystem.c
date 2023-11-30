/*
MIT License

Copyright (c) 2023 Maximilian Rudolph

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "./filesystem.h"

file_system* fs_create(uint32_t size) {
	file_system* new_fs = calloc(1, sizeof(file_system));
	if (new_fs == NULL) exit(1);

	new_fs->s_block = calloc(1, sizeof(superblock));
	if(new_fs->s_block == NULL) exit(1);

	new_fs->s_block->num_blocks = size;
	new_fs->s_block->free_blocks = size;
	
	// Create free list and set every entry to 1 (= free block)
	new_fs->free_list = calloc(1, size);
	if (new_fs->free_list == NULL) exit(1);

	for (int i=0; i<size; i++) new_fs->free_list[i] = 1;

	// Create Inodes and initialize them
	new_fs->inodes = calloc(size, sizeof(inode));
	if(new_fs->inodes == NULL) exit(1);

	for (int i=0; i<size; i++) inode_init(&(new_fs->inodes[i]));
	
	// First inode = Root ('/')
	new_fs->inodes[0].n_type = dir;
	strncpy(new_fs->inodes[0].name,"/",NAME_MAX_LENGTH);
	new_fs->root_node = 0;

	new_fs->data_blocks = calloc(size,sizeof(data_block));
	if(new_fs->data_blocks == NULL) exit(1);

	return new_fs;
}

void inode_init(inode *i) {
	i->n_type=free_block;
	i->size=0;
	memset(i->name,0,NAME_MAX_LENGTH);
	memset(i->direct_blocks, -1, DIRECT_BLOCKS_COUNT*sizeof(int));
	i->parent = -1; //meaning it has no parent
}

int find_free_inode(file_system *fs) {
	for (int i=0; i<fs->s_block->num_blocks; i++) {
		if(fs->inodes[i].n_type==free_block) return i;
	}

	return -1;
}

void fs_free(file_system *fs){
	free(fs->s_block);
	free(fs->inodes);
	free(fs->free_list);
	free(fs->data_blocks);
	free(fs);
}
