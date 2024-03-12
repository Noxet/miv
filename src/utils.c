#include "utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void err_exit(const char *file, const char *func, int line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("[%s] in [%s() @ %d] - ", file, func, line);
    vprintf(fmt, args);
    printf("\r\n");
    va_end(args);

    perror("errno");

    exit(EXIT_SUCCESS);
}
