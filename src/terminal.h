#pragma once

#define CURSOR_ORIGIN_CMD       "\x1b[H"
#define CURSOR_ORIGIN_LEN       3
#define CURSOR_HIDE_CMD         "\x1b[?25l"
#define CURSOR_HIDE_LEN         6
#define CURSOR_SHOW_CMD         "\x1b[?25h"
#define CURSOR_SHOW_LEN         6
#define DISPLAY_ERASE_ALL_CMD   "\x1b[2J"
#define DISPLAY_ERASE_ALL_LEN   4
#define DISPLAY_ERASE_LINE_CMD  "\x1b[K"
#define DISPLAY_ERASE_LINE_LEN  3


int termSetupSignals();
int termEnableRawMode();
int termDisableRawMode();
int termGetWindowSize(int *rows, int *cols);
char termReadKey();
