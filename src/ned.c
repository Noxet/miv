#include "utils.h"
#include "terminal.h"
#include "astring.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define NED_VERSION "0.1"

#define CTRL_KEY(k) ((k) & 0x1f)


static bool nedRunning = true;

typedef struct
{
    int rows;
    int cols;
} edConfig_s;


static edConfig_s edConfig;


void edProcessKey()
{
    char key = termReadKey();

    switch (key)
    {
        case CTRL_KEY('q'):
            nedRunning = false;
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            break;
        case '\r':
            printf("\r\n");
            break;
        default:
            putchar(key);
            break;
    }
}

void edDrawRows(astring *frame)
{
    for (int y = 0; y < edConfig.rows; y++)
    {
        if (y == edConfig.rows / 3)
        {
            char welcome[128] = { 0 };
            int welcomeLen = snprintf(welcome, sizeof(welcome),
                    "ned, the blazingly fast text editor -- version %s", NED_VERSION);
            int padding = (edConfig.cols - welcomeLen) / 2;

            while (padding--) astringAppend(frame, " ", 1);
            astringAppend(frame, welcome, welcomeLen);
        }
        else
        {
            astringAppend(frame, "~", 1);
        }

        // Erase line by line as we print new text
        astringAppend(frame, DISPLAY_ERASE_LINE_CMD, DISPLAY_ERASE_LINE_LEN);
        if (y < edConfig.rows - 1)
        {
            // Don't print a newline on the last line, otherwise it's empty (no tilde)
            astringAppend(frame, "\r\n", 2);
        }
    }
}

void edRefreshScreen()
{
    astring *frame = astringNew();

    astringAppend(frame, CURSOR_HIDE_CMD, CURSOR_HIDE_LEN);
    astringAppend(frame, CURSOR_ORIGIN_CMD, CURSOR_ORIGIN_LEN);

    edDrawRows(frame);

    astringAppend(frame, CURSOR_ORIGIN_CMD, CURSOR_ORIGIN_LEN);
    astringAppend(frame, CURSOR_SHOW_CMD, CURSOR_SHOW_LEN);

    write(STDOUT_FILENO, astringGetString(frame), astringGetLen(frame));
    
    astringFree(&frame);
}

void edInit()
{
    if (termGetWindowSize(&edConfig.rows, &edConfig.cols) == -1) errExit("Failed to get window size");


    printf("window size, rows: %d, cols: %d\n", edConfig.rows, edConfig.cols);
}

int main()
{
    if (termEnableRawMode() == -1) errExit("Failed to set raw mode");
    if (termSetupSignals() == -1) errExit("Failed to set up signal handler");

    edInit();

    astring *astr = astringNew();


    // disable stdout buffering
    setbuf(stdout, NULL);

    while (nedRunning)
    {
        edRefreshScreen();
        edProcessKey();
    }

    if (termDisableRawMode() == -1) errExit("Restoring userTerm failed");


    return 0;
}
