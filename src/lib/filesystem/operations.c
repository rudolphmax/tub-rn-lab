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

#include "./operations.h"
#include "../utils.h"

/**
 * Frees an instance of target_node
 * @param tnode target_node instance
 */
void fs_free_target_node(target_node *tnode) {
  free(tnode->target_name);
  free(tnode->parent_name);
  free(tnode);
}

/**
 * Validates a given path by traversing and searching inodes
 * with corresponding names in the current inode's direct_nodes
 * and returns the target_inode (index & name) and it's parent (index & name).
 * Also checks for: leading slash (required)
 * @param fs pointer to a valid file_system
 * @param path pointer to a path-string
 * @returns target_node (tnode) with the target's & it's parent's inode index &
 * name
 */
target_node *fs_parse_path(file_system *fs, char *path, enum node_type n_type) {
  // duplicating input-path for use with strtok
  char *p_validate = strdup(path);
  if (!p_validate)
    return NULL;

  // paths have to be absolute (starting with '/')
  if (strncmp(p_validate, "/", 1) != 0) {
    debug_print("ERR: Path is invalid. Must be absolute. (no-leading-slash)");
    free(p_validate);
    return NULL;
  }

  // initialising target node
  target_node *tnode = (target_node *)calloc(1, sizeof(target_node));
  tnode->target_index = -1;
  tnode->target_name = (char *)calloc(NAME_MAX_LENGTH, sizeof(char));
  tnode->parent_name = (char *)calloc(NAME_MAX_LENGTH, sizeof(char));
  tnode->parent_index = fs->root_node;

  // "empty" path (only '/')
  if (strcmp(p_validate, "/") == 0) {
    tnode->target_index = 0;
    tnode->parent_index = fs->root_node;
    *(tnode->target_name) = '/';
    *(tnode->parent_name) = '/';

    free(p_validate);
    return tnode;
  }

  // validating the given path
  char *tok = strtok(p_validate, "/");
  while (tok) {
    char *next = strtok(NULL, "/");

    // finding the inode where name == token
    int index = -1;
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
      index = fs->inodes[tnode->parent_index].direct_blocks[i];
      if (index == -1)
        continue;
      
      if (strcmp(fs->inodes[index].name, tok) == 0) {
        if (next) {
	  if (fs->inodes[index].n_type == dir) break;
	  else continue;
	} else if (fs->inodes[index].n_type == n_type) break;
      }
    }

    if (next) { // path has not been fully traversed -> inode has to be a dir
      if (index == -1) {
	    debug_print("ERR: Path is invalid. (path-invalid)");
        fs_free_target_node(tnode);
        free(p_validate);
        return NULL;
      }
      tnode->parent_index = index;
    } else {
      tnode->target_index = index;
      strncpy(tnode->target_name, tok, strlen(tok));
    }

    tok = next;
  }

  free(p_validate);

  strncpy(tnode->parent_name, fs->inodes[tnode->parent_index].name, NAME_MAX_LENGTH);

  return tnode;
}

/**
 * Creates a new node (file or dir) in the file system
 * - finds free inode
 * - sets reference to it in the parent (given by tnode)
 * @param fs Pointer to a file_system object
 * @param tnode Pointer to a target_node object with target_name & parent_index
 * set
 * @param n_type node_type value for the node that is to be created (fil or
 * dir)
 * @returns 0 on success, -1 on failure
 */
int fs_new_node(file_system *fs, target_node *tnode, enum node_type n_type) {
  // finding a free inode for the new dir
  int dir_inode_index = find_free_inode(fs);
  if (dir_inode_index == -1) {
    debug_print("ERR: No inode Available. (exhausted-inodes)");
    return -1;
  }

  // Initializing dir-inode
  inode *dir_inode = &(fs->inodes[dir_inode_index]);
  strncpy(dir_inode->name, tnode->target_name, NAME_MAX_LENGTH);
  dir_inode->n_type = n_type;
  dir_inode->parent = tnode->parent_index;

  // finding free direct-block on parent
  int index = -1;
  for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
    if (fs->inodes[tnode->parent_index].direct_blocks[i] == -1) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    debug_print("ERR: No more space in target dir. (exhausted-direct-blocks)");
    return -1;
  }

  // setting required values on new_dir parent
  fs->inodes[tnode->parent_index].direct_blocks[index] = dir_inode_index;
  fs_free_target_node(tnode);
  return 0;
}

int fs_mkdir(file_system *fs, char *path) {
  // validating path & getting target_name and parent_index
  target_node *tnode = fs_parse_path(fs, path, dir);
  if (tnode == NULL) return -1;
  
  if (tnode->target_index != -1) {
    debug_print("ERR: Directory already exists.");
    fs_free_target_node(tnode);
    return -1;
  }

  return fs_new_node(fs, tnode, dir);
}

int fs_mkfile(file_system *fs, char *path_and_name) {
  // validating path & getting target_name and parent_index
  target_node *tnode = fs_parse_path(fs, path_and_name, fil);
  if (tnode == NULL)
    return -1;
  
  if (tnode->target_index != -1) {
    debug_print("ERR: File already exists.");
    fs_free_target_node(tnode);
    return -2;
  }

  return fs_new_node(fs, tnode, fil);
}

int cpmint(const void *a, const void *b) { return (*(int *)a) - (*(int *)b); }

char *fs_list(file_system *fs, char *path) {
  target_node *tnode = fs_parse_path(fs, path, dir);
  if (tnode == NULL)
    return NULL;
 
  if (tnode->target_index == -1) {
    debug_print("ERR: Directory not found.");
    return NULL;
  }

  // assembling array of the inode-indices of the target's children
  int child_inodes[DIRECT_BLOCKS_COUNT];
  int child_count = 0;

  for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
    int index = fs->inodes[tnode->target_index].direct_blocks[i];
    if (index == -1 || fs->inodes[index].n_type == free_block)
      continue;

    child_inodes[child_count++] = index;
  }

  // sorting child-inode-indices
  qsort(child_inodes, child_count, sizeof(int), cpmint);

  // assembling output string
  int bufsize = 0;
  char *string = (char *)calloc(bufsize, sizeof(char));

  for (int i = 0; i < child_count; i++) {
    int index = child_inodes[i];
    // line_len = name_len + (3 chars (DIR or FIL) + space + newline = 6)
    int line_len = strlen(fs->inodes[index].name) + 6;

    bufsize += line_len;
    string = realloc(string, bufsize);
    if (!string) {
      fs_free_target_node(tnode);
      return NULL;
    }

    snprintf(string + strlen(string), line_len, "%s %s\n",
             (fs->inodes[index].n_type == dir) ? "DIR" : "FIL",
             fs->inodes[index].name);
  }

  fs_free_target_node(tnode);
  return string;
}

/**
 * Finds the first free data-block in the file-system.
 * @param fs Pointer to a file_system object
 * @returns index of the data-block for reference in fs->data_blocks 
 */
int fs_find_block(file_system *fs) {
  int block_index = -1;
  for (int i = 0; i < (int)fs->s_block->num_blocks; i++) {
    block_index = i;
    if (fs->free_list[block_index] == 1)
      break;
  }

  return block_index;
}

int fs_writef(file_system *fs, char *filename, char *text) {
  // empty data; write nothing; return instantly...
  if (strlen(text) == 0)
    return 0;

  // validating path & getting target_name and parent_index
  target_node *tnode = fs_parse_path(fs, filename, fil);
  if (tnode == NULL)
    return -1;
  if (tnode->target_index == -1) {
    debug_print("ERR: File not found.");
    fs_free_target_node(tnode);
    return -1;
  }

  int data_size = strlen(text);

  // 2D Array of the structure { {block_id, available_space}, ... }
  int blocks[DIRECT_BLOCKS_COUNT][2];
  // initializing block_id with -1
  for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++)
    blocks[i][0] = -1;

  // Gathering blocks according to the data_size
  // If file has blocks assigned and they have space: use them
  // if not: find a new, free block
  int leftovers = data_size;
  int i = 0;
  while (leftovers > 0 && i < DIRECT_BLOCKS_COUNT) {
    int index = fs->inodes[tnode->target_index].direct_blocks[i];

    if (index == -1)
      index = fs_find_block(fs);

    if (index == -1) {
      debug_print("ERR: No free data-block available.");
      fs_free_target_node(tnode);
      return -1;
    }

    int space = BLOCK_SIZE - fs->data_blocks[index].size;
    if (space > 0) {
      blocks[i][0] = index;
      blocks[i][1] = MIN(leftovers, space);
      leftovers -= blocks[i][1];

      if (fs->free_list[index] != 0) {
        fs->s_block->free_blocks--;
        fs->free_list[index] = 0;
      }
    }

    i++;
  }

  // Write the corresponding part of the data to the blocks
  // Data can be split between blocks
  // Assuming there is only ever one block containing data that still has space
  int written = 0;
  for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
    if (blocks[i][0] == -1)
      continue;

    data_block *dblock = &(fs->data_blocks[blocks[i][0]]);
    // setting remainings of block to 0
    memset(dblock->block + dblock->size, 0, BLOCK_SIZE - dblock->size);
    // copying data
    memcpy(dblock->block + dblock->size, text + written, blocks[i][1]);

    dblock->size += blocks[i][1];
    fs->inodes[tnode->target_index].direct_blocks[i] = blocks[i][0];
    written += blocks[i][1];
  }
  fs->inodes[tnode->target_index].size = written;

  if (written < data_size) {
    debug_print("ERR: Not all bytes could be written.");
    fs_free_target_node(tnode);
    return -1;
  }

  fs_free_target_node(tnode);
  return written;
}

uint8_t *fs_readf(file_system *fs, char *filename, int *file_size) {
  // validating path
  target_node *tnode = fs_parse_path(fs, filename, fil);
  if (tnode == NULL)
    return NULL;
  if (tnode->target_index == -1) {
    debug_print("ERR: File not found.");
    fs_free_target_node(tnode);
    return NULL;
  }

  // initialising buffer array
  size_t bufsize = fs->inodes[tnode->target_index].size + 1;
  uint8_t *buf = (uint8_t *) calloc(bufsize, sizeof(uint8_t));

  for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
    int index = fs->inodes[tnode->target_index].direct_blocks[i];
    if (index == -1)
      continue;

    data_block *dblock = &(fs->data_blocks[index]);

    // copying bytes from dblock to buffer array (bytewise)
    for (int ci = 0; ci < (int)dblock->size; ci++) {
      memcpy(buf + *file_size + ci * sizeof(uint8_t),
	     dblock->block + ci * sizeof(uint8_t), 1);
    }
    *file_size += dblock->size;
  }

  if (*file_size == 0) {
    fs_free_target_node(tnode);
    return NULL;
  }

  fs_free_target_node(tnode);
  return buf;
}

target_node * fs_find_target(file_system * fs, char* path) {
  target_node * tnode;
  tnode = fs_parse_path(fs, path, dir);
  if (tnode != NULL && tnode->target_name != NULL && tnode->target_index != -1) {
    return tnode;
  }

  if (tnode) fs_free_target_node(tnode);

  tnode = fs_parse_path(fs, path, fil);
  if (tnode != NULL && tnode->target_name != NULL && tnode->target_index != -1) {
    return tnode;
  }

  debug_print("ERR: File or dir not found.");
  if (tnode) fs_free_target_node(tnode);
  return NULL;
}

int fs_rm(file_system *fs, char *path) {
  // validating path
  target_node *tnode = fs_find_target(fs, path);
  if (tnode == NULL) return -1;

  inode* target_inode = &(fs->inodes[tnode->target_index]);
  inode* parent_inode = &(fs->inodes[tnode->parent_index]);

  if (target_inode->n_type == fil) {
    // resetting data-blocks
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
      int index = target_inode->direct_blocks[i];
      if (index == -1)
        continue;

      fs->free_list[index] = 1;
      fs->free_list[index] = 1;
      fs->s_block->free_blocks++;
      data_block *dblock = &(fs->data_blocks[index]);
      memset(dblock->block, 0, dblock->size);
    }

  } else if (target_inode->n_type == dir) {
    // removing children (recursive)
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
      int index = target_inode->direct_blocks[i];
      if (index == -1)
        continue;

      // assembling child's path in fs
      char *childPath = (char *) calloc(strlen(path) + NAME_MAX_LENGTH + 1, sizeof(char));
      strncat(childPath, path, strlen(path));
      strncat(childPath, "/", 1);
      strncat(childPath, fs->inodes[index].name, strlen(fs->inodes[index].name));

      fs_rm(fs, childPath);
      free(childPath);
    }
  }

  // resetting inode
  inode_init(target_inode);

  // removing ref in parent
  for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
    if (parent_inode->direct_blocks[i] == tnode->target_index) {
      parent_inode->direct_blocks[i] = -1;
    }
  }

  fs_free_target_node(tnode);
  return 0;
}

int fs_import(file_system *fs, char *int_path, char *ext_path) {
  FILE *fp = fopen(ext_path, "r");
  if (!fp) {
    debug_print("ERR: Problem opening external file. It probably doesn't exist.");
    return -1;
  }

  fseek(fp, 0, SEEK_END);
  int len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  int retval = -1;

  if (len) {
    char *file_contents = (char *)calloc(len + 1, sizeof(char));

    int read_len = fread(file_contents, 1, len, fp);
    if (read_len == len)
      retval = fs_writef(fs, int_path, file_contents);

    free(file_contents);
  }

  fclose(fp);

  if (retval < 0) return -1;
  
  return 0;
}

int fs_export(file_system *fs, char *int_path, char *ext_path) {
  int file_size = 0;
  uint8_t *file_contents = fs_readf(fs, int_path, &file_size);
  if (file_contents == NULL)
    return -1;

  FILE *fp = fopen(ext_path, "w");
  if (!fp) {
    debug_print("ERR: Problem opening external file. It probably doesn't exist.");
    return -1;
  }

  size_t written_len = fwrite(file_contents, sizeof(uint8_t), file_size, fp);

  fclose(fp);
  free(file_contents);

  if (written_len != file_size)
    return -1;

  return 0;
}
