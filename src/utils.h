#pragma once

#define UNUSED(x) (void) (x)

#define errExit(...) err_exit(__FILE__, __LINE__, __VA_ARGS__);

void err_exit(const char *file, int line, const char *fmt, ...);
