#ifndef RN_PRAXIS_UTILS_H
#define RN_PRAXIS_UTILS_H

#include <inttypes.h>
#include <sys/errno.h>
#include <string.h>

int str_is_uint16(const char *str);

/**
 * Checks if a given string-buffer end with a CRLF ('\r\n\r\n')
 * @param str input string to check
 * @return 0 if it ends with CRLF, 1 if it doesn't, -1 on error
 */
int string_ends_with_emptyline(char* str);

#endif //RN_PRAXIS_UTILS_H
