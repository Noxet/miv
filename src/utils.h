#pragma once

#define UNUSED(x) (void) (x)

#define errExit(...) {termDisableRawMode(); err_exit(__FILE__, __func__, __LINE__, __VA_ARGS__);}

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

void err_exit(const char *file, const char *func, int line, const char *fmt, ...);
