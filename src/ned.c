#include "utils.h"
#include "terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>


#define CTRL_KEY(k) ((k) & 0x1f)

#define SET_CURSOR_ORIGIN() write(STDOUT_FILENO, "\x1b[H", 3);


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

void edDrawRows()
{
    for (int y = 0; y < edConfig.rows; y++)
    {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void edRefreshScreen()
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    SET_CURSOR_ORIGIN();

    edDrawRows();

    SET_CURSOR_ORIGIN();
}

void edInit()
{
    if (termGetWindowSize(&edConfig.rows, &edConfig.cols) == -1) errExit("Failed to get window size");
}

int main()
{
    if (termEnableRawMode() == -1) errExit("Failed to set raw mode");
    if (termSetupSignals() == -1) errExit("Failed to set up signal handler");

    edInit();

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
