#ifndef RN_PRAXIS_UTILS_H
#define RN_PRAXIS_UTILS_H

#include <inttypes.h>
#include <sys/errno.h>
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))


/**
 * Prints a given debug message to the console.
 * @param message the message to be printed
 */
void debug_print(char *message);

/**
 * Prints a given debug message together with an value to the console.
 * @param message the message to be printed
 */
void debug_printv(char *message, char *value);

/**
 * Determines whether a given string could be interpreted as an uint16.
 * Attempts a conversion from char* to uint16_t to see if it fails. The result isn't saved.
 * @param str the string to be checked
 * @return 1 if the string could be a uint16 if converted, 0 if not.
 */
int str_is_uint16(const char *str);

/**
 * Checks if a given string-buffer end with a CRLF ('\r\n\r\n')
 * @param str input string to check
 * @return 1 if it ends with CRLF, 0 if it doesn't, -1 on error
 */
int string_ends_with_empty_line(char* str);

#endif //RN_PRAXIS_UTILS_H
