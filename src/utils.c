#include "utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void err_exit(const char *file, int line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("[%s] @ %d - ", file, line);
    vprintf(fmt, args);
    va_end(args);

    exit(EXIT_SUCCESS);
}
