#include "utils.h"
#include "terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define CTRL_KEY(k) ((k) & 0x1f)


void editorProcessKey()
{
    char key = termReadKey();

    switch (key)
    {
        case CTRL_KEY('q'):
            exit(0);
            break;
        case '\r':
            printf("\r\n");
            break;
        default:
            putchar(key);
            break;
    }
}

int main()
{
    if (termEnableRawMode() == -1) errExit("Failed to set raw mode");
    if (termSetupSignals() == -1) errExit("Failed to set up signal handler");

    // disable stdout buffering
    setbuf(stdout, NULL);

    while (true)
    {
        editorProcessKey();
    }

    if (termDisableRawMode() == -1) errExit("Restoring userTerm failed");


    return 0;
}
