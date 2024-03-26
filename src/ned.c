
#define _DEFAULT_SOURCE

#include "utils.h"
#include "terminal.h"
#include "astring.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define NED_VERSION "0.1"

#define ESC_KEY '\x1b'
#define CTRL_KEY(k) ((k) & 0x1f)


static bool nedRunning = true;

typedef struct
{
    char *string;
    int size;
    char *renderString;
    int rsize;
} edRow_s;

typedef struct
{
    int winRows;   // window size
    int winCols;
    int cx;     // cursor pos
    int cy;
    edRow_s *row;
    int numRows;
    int rowOffset;
    int colOffset;
} edConfig_s;


static edConfig_s edConfig;

static FILE *logFile = NULL;
#define LOG(format, ...) { fprintf(logFile, format, __VA_ARGS__); fflush(logFile); }

void edMoveCursor(int key)
{
    switch(key)
    {
        case ARROW_UP:
            if (edConfig.cy > 0) edConfig.cy--;
            // if cursor ends up past the line end, snap it to end of line
            if (edConfig.cx >= edConfig.row[edConfig.cy].size) edConfig.cx = edConfig.row[edConfig.cy].size - 1;
            break;
        case ARROW_DOWN:
            if (edConfig.cy < edConfig.numRows - 1) edConfig.cy++;
            // if cursor ends up past the line end, snap it to end of line
            if (edConfig.cx >= edConfig.row[edConfig.cy].size) edConfig.cx = edConfig.row[edConfig.cy].size - 1;
            break;
        case ARROW_RIGHT:
            // get the size of the column at the current row (cy)
            if (edConfig.cx < edConfig.row[edConfig.cy].size - 1) edConfig.cx++;
            break;
        case ARROW_LEFT:
            if (edConfig.cx > 0) edConfig.cx--;
            break;
        case HOME:
            edConfig.cx = 0;
            break;
        case END:
            // set cursor to the smallest of the window size and the current row length
            if (edConfig.winCols < edConfig.row[edConfig.cy].size)
            {
                edConfig.cx = edConfig.winCols - 1;
            }
            else
            {
                edConfig.cx = edConfig.row[edConfig.cy].size - 1;
            }
            break;
    }
}

void edProcessKey()
{
    int key = termReadKey();

    switch (key)
    {
        case ESC_KEY:
            break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
        case HOME:
        case END:
            edMoveCursor(key);
            break;
        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = edConfig.winRows - 1;
                while (times--) edMoveCursor(key == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;
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

void edDrawWelcomeMsg(astring *frame)
{
    if (edConfig.numRows == 0)
    {

        char welcome[128] = { 0 };
        int welcomeLen = snprintf(welcome, sizeof(welcome),
                "ned, the blazingly fast text editor -- version %s", NED_VERSION);
        int padding = (edConfig.winCols - welcomeLen) / 2;

        if (padding)
        {
            astringAppend(frame, "~", 1);
            padding--;
        }

        while (padding--) astringAppend(frame, " ", 1);
        astringAppend(frame, welcome, welcomeLen);
    }
    else
    {
        astringAppend(frame, "~", 1);
    }
}

void edScroll()
{
    if (edConfig.cy >= edConfig.winRows)
    {
        edConfig.rowOffset = edConfig.cy - edConfig.winRows + 1;
        LOG("cy: %d, winrows: %d, offset: %d\n", edConfig.cy, edConfig.winRows, edConfig.rowOffset);
    }
    else
    {
        edConfig.rowOffset = 0;
    }

    LOG("cx: %d\n", edConfig.cx);
    if (edConfig.cx >= edConfig.winCols)
    {
        edConfig.colOffset = edConfig.cx - edConfig.winCols + 1;
        LOG("cx: %d, wincols: %d, offset: %d\n", edConfig.cx, edConfig.winCols, edConfig.colOffset);
    }
    else
    {
        LOG("%s\n", "coloffset = 0");
        edConfig.colOffset = 0;
    }
}

void edDrawRows(astring *frame)
{
    for (int y = 0; y < edConfig.winRows; y++)
    {
        int off = edConfig.rowOffset;
        if (y < (edConfig.numRows - off))
        {
            // limit text size to the window width
            edRow_s currRow = edConfig.row[y + off];
            // do not scroll further than row size. Print at most the NULL char
            int colOffset = (edConfig.colOffset <= currRow.rsize) ? edConfig.colOffset : currRow.rsize;
            int len = (currRow.rsize - colOffset > edConfig.winCols) ? edConfig.winCols : currRow.rsize - colOffset;
            LOG("drawing string: %s\n", &currRow.renderString[colOffset]);
            astringAppend(frame, &currRow.renderString[colOffset], len);
        }
        else if (y == edConfig.winRows/ 3)
        {
            edDrawWelcomeMsg(frame);
        }
        else
        {
            astringAppend(frame, "~", 1);
        }

        // Erase line by line as we print new text
        astringAppend(frame, DISPLAY_ERASE_LINE_CMD, DISPLAY_ERASE_LINE_LEN);
        if (y < edConfig.winRows- 1)
        {
            // Don't print a newline on the last line, otherwise it's empty (no tilde)
            astringAppend(frame, "\r\n", 2);
        }
    }
}

void edRefreshScreen()
{
    edScroll();

    astring *frame = astringNew();

    astringAppend(frame, CURSOR_HIDE_CMD, CURSOR_HIDE_LEN);
    astringAppend(frame, CURSOR_ORIGIN_CMD, CURSOR_ORIGIN_LEN);

    edDrawRows(frame);

    // Set cursor position
    char cursorPos[32];
    // Terminal is 1-indexed, so we need to add 1 to the positions
    int cursorPosLen = snprintf(cursorPos, sizeof(cursorPos), "\x1b[%d;%dH", edConfig.cy  - edConfig.rowOffset + 1, edConfig.cx  - edConfig.colOffset + 1);
    astringAppend(frame, cursorPos, cursorPosLen);

    astringAppend(frame, CURSOR_SHOW_CMD, CURSOR_SHOW_LEN);

    write(STDOUT_FILENO, astringGetString(frame), astringGetLen(frame));
    
    astringFree(&frame);
}

void edRenderRow(edRow_s *row)
{
    // free previously allocated mem
    free(row->renderString);
    row->rsize = 0;
    row->renderString = malloc(9 * row->size + 1);

    int idx = 0;
    while (row->string[idx])
    {
        switch (row->string[idx])
        {
            case '\t':
                {
                    char *tab = "    ";
                    memcpy(&row->renderString[row->rsize], tab, 4);
                    row->rsize += 4;
                }
                break;
            default:
                // just copy the chars as normal
                row->renderString[row->rsize] = row->string[idx];
                row->rsize++;
                break;
        }
        idx++;
    }

    row->renderString[row->rsize] = '\0';
}

void edAppendRow(char *line, size_t lineLen)
{
    // TODO(noxet): reallocate row when we hit limit

    line[lineLen] = '\0';

    LOG("append string: %s\n", line);

    edConfig.row[edConfig.numRows].size = lineLen;
    // TODO(noxet): free this memory later, in edClose?
    edConfig.row[edConfig.numRows].string = strdup(line);

    edConfig.row[edConfig.numRows].renderString = NULL;
    edConfig.row[edConfig.numRows].rsize = 0;

    edRenderRow(&edConfig.row[edConfig.numRows]);

    edConfig.numRows++;
}

void edOpen(const char *filename)
{
    assert(filename != NULL);

    FILE *inputFile = fopen(filename, "r");
    if (!inputFile) errExit("Failed to open file: %s", filename);

    char *line = NULL;
    size_t bufferSize = 0;
    ssize_t lineLen = 0;
    while ((lineLen = getline(&line, &bufferSize, inputFile)) != -1)
    {
        // TODO(noxet): hack to avoid BO, fix later with reallocations
        if (edConfig.numRows >= 100) break;

        // remove newline char(s) if present and terminate string
        while (lineLen > 0 && (line[lineLen - 1] == '\n' || line[lineLen - 1] == '\r')) lineLen--;
        edAppendRow(line, lineLen);
    }

    free(line);
    fclose(inputFile);
}

void edInit()
{
    edConfig.cx = 0;
    edConfig.cy = 0;
    edConfig.row = calloc(100, sizeof(*edConfig.row));
    edConfig.numRows = 0;
    edConfig.rowOffset = 0;
    edConfig.colOffset = 0;

    // TODO(noxet): Handle window resize event
    if (termGetWindowSize(&edConfig.winRows, &edConfig.winCols) == -1) errExit("Failed to get window size");


    printf("window size, rows: %d, cols: %d\n", edConfig.winRows, edConfig.winCols);
}

int main(int argc, char *argv[])
{
    logFile = fopen("ned.log", "w");

    if (termEnableRawMode() == -1) errExit("Failed to set raw mode");
    if (termSetupSignals() == -1) errExit("Failed to set up signal handler");

    edInit();
    if (argc >= 2)
    {
        edOpen(argv[1]);
    }

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
