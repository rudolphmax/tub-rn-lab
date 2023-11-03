#include "utils.h"

int str_is_uint16(const char *str) {
    char *end;
    errno = 0;
    intmax_t val = strtoimax(str, &end, 10);
    if (errno == ERANGE || val < 0 || val > UINT16_MAX || end == str || *end != '\0') {
        return -1;
    }

    return 0;
}
