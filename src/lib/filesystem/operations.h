#ifndef OPERATIONS_H

#include <stdint.h>
#include <string.h>

#include "filesystem.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct _target_node {
    char *target_name;
    int target_index; // -1 == not in fs yet, ergo: no index yet
    char *parent_name;
    int parent_index;
} target_node;

// TODO: Doc this
target_node *fs_parse_path(file_system *fs, char *path, enum node_type n_type);

// TODO: Doc this
target_node * fs_find_target(file_system * fs, char* path);

/**
 * Creates a new directory under the given path
 *
 * @Returns: 0 on success, else -1
 */
int fs_mkdir(file_system *fs, char *path);

/**
 * Creates a new empty file under given path_name
 *
 * @Returns:
 * 0 on success
 * -1 if the path is invalid
 * -2 if the file already exists
 */
int fs_mkfile(file_system *fs, char *path_and_name);

/**
 * Lists all directories and files in the directory pointed to by path
 * @Returns:
 * A string in the following format:
 *	- for every directory a line `DIR <directoryname>`. Where `DIR` should
 *	literally be there and `<directoryname>` is substituted with the actual name
 *	of that directory.
 *	- for every directory a line `FILE <filename>`. Where `FILE` should
 *	literally be there and `filename` is substituted with the actual name of
 *	that file.
 * The lines are sorted by inode-index.
 *
 *
 * NOTE: Don't forget to allocate the memory for the string with malloc.
 *
 * In case the given path was not found return NULL
 */
char *fs_list(file_system *fs, char *path);

/**
 * Write (append, not overwrite) @param text to a file pointed to by @param
 * filename The file must exist before it can be written to
 *
 * @Returns:
 * number of written chars on success
 * -1 if the file is not available
 *  -2 if the file is full
 */
int fs_writef(file_system *fs, char *filename, char *text);

/**
 * Reads a file and allocates memory for a uint8_t buffer (array). Reads this
 * file into the buffer writes the file_size into the memory pointed to by int*
 * file_size
 *
 * @Returns the buffer or NULL if the file does not exist
 */
uint8_t *fs_readf(file_system *fs, char *filename, int *file_size);

/**
 * Deletes a file or a directory recursively.
 *
 * @Returns:
 * 0 on success
 * -1 if the file or directory was not found
 */
int fs_rm(file_system *fs, char *path);

/**
 * Imports the file and saves it in the current filesystem under the path
 * pointed to by the second parameter
 *
 * @Param: char* int_path path where the imported file should be saved in the
 * internal file system
 * @Param: char* ext_path path of the file to be imported
 *
 * @Returns:
 * 0 on success
 * -1 if the file or directory wasn't found
 */
int fs_import(file_system *fs, char *int_path, char *ext_path);

/**
 * Exports the file and saves it in the external filesystem under the path pointed to by the second parameter
 * @Param: char* int_path path where the exported file lives
 * @Param: char* ext_path path where the file should be saved in the external filesystem
 *
 * @Returns:
 * 0 on success
 * -1 if the file or directory wasn't found
 */
int fs_export(file_system *fs, char *int_path, char *ext_path);

#define OPERATIONS_H
#endif /* OPERATIONS_H */
