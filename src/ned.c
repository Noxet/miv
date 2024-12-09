
#define _DEFAULT_SOURCE

#include "utils.h"
#include "terminal.h"
#include "astring.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#define NED_VERSION "0.1"

#define NED_TAB_STOP 8
#define NED_QUIT_TIMES 2

//#define ESC_KEY '\x1b'
#define CTRL_KEY(k) ((k) & 0x1f)


static bool nedRunning = true;

typedef struct
{
    char *string;
    int size;
    char *renderString;
    int renderSize;
} edRow_s;

typedef struct
{
    int winRows;    // window size
    int winCols;
    int cx;         // cursor pos
    int cy;
    int rx;         // render pos
    edRow_s *row;
    int numRows;
    int rowOffset;
    int colOffset;
    char *filename;
    char statusMsg[80];
    time_t statusMsgTime;
    bool dirty;
} edConfig_s;


void edInsertChar(int c);
void edDeleteChar();
void edNewLine();
void edSaveFile(const char *filename);
void edSetStatusMessage(const char *fmt, ...);
void edRowDeleteChar(edRow_s *row, int at);


static edConfig_s edConfig;

static FILE *logFile = NULL;
#define LOG(format, ...) { fprintf(logFile, format, __VA_ARGS__); fflush(logFile); }

void edMoveCursor(int key)
{
    switch(key)
    {
        case ARROW_UP:
            if (edConfig.cy > 0) edConfig.cy--;
            break;
        case ARROW_DOWN:
            if (edConfig.cy < edConfig.numRows - 1) edConfig.cy++;
            break;
        case ARROW_RIGHT:
            // get the size of the column at the current row (cy)
            //if (edConfig.cx < edConfig.row[edConfig.cy].renderSize - 1) edConfig.cx++;
            edConfig.cx++;
            break;
        case ARROW_LEFT:
            if (edConfig.cx > 0) edConfig.cx--;
            break;
        case HOME:
            edConfig.cx = 0;
            break;
        case END:
            edConfig.cx = edConfig.row[edConfig.cy].size;
            break;
        default:
            assert(false);
            break;
    }

    // if cursor ends up past the line end, snap it to end of line
    if (edConfig.cx >= edConfig.row[edConfig.cy].size) edConfig.cx = edConfig.row[edConfig.cy].size;
    // limit cx to 0 in case of empty line
    if (edConfig.cx < 0) edConfig.cx = 0;
}

void edProcessKey()
{
    static int quitTimes = NED_QUIT_TIMES;
    int key = termReadKey();

    switch (key)
    {
        case ESC_KEY:
        case CTRL_KEY('l'):     // screen refresh not needed since we do it on every update
            // TODO
            break;
        case CTRL_KEY('h'):
        case BACKSPACE:
        case DELETE:
            if (key == DELETE)
            {
                if (edConfig.cx == edConfig.row[edConfig.cy].size)
                {
                    edMoveCursor(ARROW_DOWN);
                    edMoveCursor(HOME);
                }
                else
                {
                    edMoveCursor(ARROW_RIGHT);
                }
            }
            edDeleteChar();
            break;
        case ENTER_KEY:
            edNewLine();
            break;

        /* ARROW KEYS */
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
            if (edConfig.dirty)
            {
                edSetStatusMessage("File modified, press CTRL-Q again to discard changes and quit");
                quitTimes--;
                if (quitTimes == 0) nedRunning = false;
            }
            else
            {
                nedRunning = false;
            }

            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            // we need to return here, to not reset the quitTime counter at the end
            return;
        case CTRL_KEY('w'):
            edSaveFile(edConfig.filename);
            edSetStatusMessage("File saved successfully!");
            break;
        default:
            edInsertChar(key);
            break;
    }

    quitTimes = NED_QUIT_TIMES;
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

void edDrawStatusBar(astring *frame)
{
    // TODO(noxet): make macros for colors
    astringAppend(frame, "\x1b[7m", 4);
    char status[256];
    char *filename = (edConfig.filename == NULL) ? "No Name" : edConfig.filename;
    char *dirty = (edConfig.dirty) ? "(modified)" : "";
    int statusLen = snprintf(status, sizeof(status), "[%.20s] - %d lines %s", filename, edConfig.numRows, dirty);
    astringAppend(frame, status, statusLen);

    // right-adjusted status bar
    char rstatus[32];
    int rstatusLen = snprintf(rstatus, sizeof(rstatus), "[%d / %d]", edConfig.cy + 1, edConfig.numRows);
    // fill the rest of the status bar with white color
    while (statusLen < edConfig.winCols - rstatusLen)
    {
        astringAppend(frame, " ", 1);
        statusLen++;
    }

    astringAppend(frame, rstatus, rstatusLen);

    astringAppend(frame, "\x1b[m", 3);
    astringAppend(frame, "\r\n", 2);
}


void edDrawMessageBar(astring *frame)
{
    // clear the line
    astringAppend(frame, "\x1b[K", 3);
    size_t msgLen = strlen(edConfig.statusMsg);
    if (msgLen && (time(NULL) - edConfig.statusMsgTime < 5))
    {
        astringAppend(frame, edConfig.statusMsg, msgLen);
    }
}


static inline int edRowCxToRx(edRow_s *row, int cx)
{
    int rx = 0;
    for (int i = 0; i < cx; i++)
    {
        if (row->string[i] == '\t') rx += (NED_TAB_STOP - 1) - (rx % NED_TAB_STOP);
        rx++;
    }

    return rx;
}


void edScroll()
{
    edConfig.rx = 0;

    if (edConfig.cy < edConfig.numRows)
    {
        edConfig.rx = edRowCxToRx(&edConfig.row[edConfig.cy], edConfig.cx);
    }

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
    LOG("rx: %d\n", edConfig.rx);
    if (edConfig.rx >= edConfig.winCols)
    {
        edConfig.colOffset = edConfig.rx - edConfig.winCols + 1;
        LOG("rx: %d, wincols: %d, offset: %d\n", edConfig.rx, edConfig.winCols, edConfig.colOffset);
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
            int colOffset = (edConfig.colOffset <= currRow.renderSize) ? edConfig.colOffset : currRow.renderSize;
            int len = (currRow.renderSize - colOffset > edConfig.winCols) ? edConfig.winCols : currRow.renderSize - colOffset;
            astringAppend(frame, &currRow.renderString[colOffset], len);
        }
        else if (y == edConfig.winRows / 3)
        {
            edDrawWelcomeMsg(frame);
        }
        else
        {
            astringAppend(frame, "~", 1);
        }

        // Erase line by line as we print new text
        astringAppend(frame, DISPLAY_ERASE_LINE_CMD, DISPLAY_ERASE_LINE_LEN);
        //if (y < edConfig.winRows - 1)
        astringAppend(frame, "\r\n", 2);
    }
}

void edRefreshScreen()
{
    edScroll();

    astring *frame = astringNew();

    astringAppend(frame, CURSOR_HIDE_CMD, CURSOR_HIDE_LEN);
    astringAppend(frame, CURSOR_ORIGIN_CMD, CURSOR_ORIGIN_LEN);

    edDrawRows(frame);
    edDrawStatusBar(frame);
    edDrawMessageBar(frame);

    // Set cursor position
    char cursorPos[32];
    // Terminal is 1-indexed, so we need to add 1 to the positions
    int cursorPosLen = snprintf(cursorPos, sizeof(cursorPos), "\x1b[%d;%dH", edConfig.cy  - edConfig.rowOffset + 1, edConfig.rx  - edConfig.colOffset + 1);
    astringAppend(frame, cursorPos, cursorPosLen);

    astringAppend(frame, CURSOR_SHOW_CMD, CURSOR_SHOW_LEN);

    write(STDOUT_FILENO, astringGetString(frame), astringGetLen(frame));

    astringFree(&frame);
}

void edSetStatusMessage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(edConfig.statusMsg, sizeof(edConfig.statusMsg), fmt, ap);
    va_end(ap);
    edConfig.statusMsgTime = time(NULL);
}

/**
 * Row operations
 */

void edRenderRow(edRow_s *row)
{
    // free previously allocated mem
    free(row->renderString);
    row->renderSize = 0;
    size_t numTabs = 0;
    for (int i = 0; i < row->size; i++)
    {
        if (row->string[i] == '\t') numTabs++;
    }
    row->renderString = malloc(row->size + numTabs * (NED_TAB_STOP - 1) + 1);

    int idx = 0;
    while (row->string[idx])
    {
        switch (row->string[idx])
        {
            case '\t':
                {
                    row->renderString[row->renderSize++] = ' ';
                    while (row->renderSize % NED_TAB_STOP != 0) row->renderString[row->renderSize++] = ' ';
                }
                break;
            default:
                // just copy the chars as normal
                row->renderString[row->renderSize] = row->string[idx];
                row->renderSize++;
                break;
        }
        idx++;
    }

    row->renderString[row->renderSize] = '\0';
}

void edInsertRow(int at, char *line, size_t lineLen)
{
    // TODO(noxet): reallocate row when we hit limit
    memmove(&edConfig.row[at + 1], &edConfig.row[at], sizeof(edRow_s) * (edConfig.numRows - at));

    edConfig.row[at].size = lineLen;
    // TODO(noxet): free this memory later, in edClose?
    edConfig.row[at].string = malloc(lineLen + 1);
    memcpy(edConfig.row[at].string, line, lineLen);
    edConfig.row[at].string[lineLen] = '\0';

    edConfig.row[at].renderString = NULL;
    edConfig.row[at].renderSize = 0;

    edRenderRow(&edConfig.row[at]);

    edConfig.numRows++;

    edConfig.dirty = true;
}

void edRowInsertChar(edRow_s *row, int at, int c)
{
    if (at < 0 || at > row->size) at = row->size;
    row->string = realloc(row->string, row->size + 2); // +2 for new char and NULL-byte at the end
    memmove(&row->string[at + 1], &row->string[at], row->size - at + 1);
    row->size++;
    row->string[at] = c;
    row->string[row->size] = '\0';
    edRenderRow(row);
}

void edRowDeleteChar(edRow_s *row, int at)
{
    if (at < 0) return;
    memmove(&row->string[at], &row->string[at + 1], row->size - at);
    row->size--;
    //row->string = realloc(row->string, row->size - 1);
    edRenderRow(row);
}

void edRowAppendString(edRow_s *row, char *str)
{
    size_t strLen = strlen(str);

    row->string = realloc(row->string, row->size + strLen + 1);
    strcat(row->string, str);
    row->size += strLen;

    edRenderRow(row);
    edConfig.dirty = true;
}

/*
 * Inserts a character into the text under the cursor
 */
void edInsertChar(int c)
{
    if (edConfig.cy == edConfig.numRows)
    {
        edInsertRow(edConfig.numRows, "", 0);
    }

    edRowInsertChar(&edConfig.row[edConfig.cy], edConfig.cx, c);
    edConfig.cx++;

    edConfig.dirty = true;
}

void edFreeRow(edRow_s *row)
{
    free(row->string);
    free(row->renderString);
}

void edDeleteRow(int atY)
{
    if (atY <= 0 || atY > edConfig.numRows) return;
    edRowAppendString(&edConfig.row[atY - 1], edConfig.row[atY].string);
    edFreeRow(&edConfig.row[atY]);
    memmove(&edConfig.row[atY], &edConfig.row[atY + 1], sizeof(edRow_s) * (edConfig.numRows - atY - 1));
    edConfig.numRows--;
    edConfig.dirty = true;
}

void edDeleteChar()
{
    if (edConfig.cy == edConfig.numRows) return;
    if (edConfig.cx == 0 && edConfig.cy == 0) return;

    if (edConfig.cx <= 0)
    {
        edConfig.cx = edConfig.row[edConfig.cy - 1].size;
        edDeleteRow(edConfig.cy);
        edConfig.cy--;
    }
    else
    {
        edRow_s *row = &edConfig.row[edConfig.cy];
        edRowDeleteChar(row, edConfig.cx - 1);
        edConfig.cx--;
    }

    edConfig.dirty = true;
}

void edNewLine()
{
    edRow_s *row = &edConfig.row[edConfig.cy];

    char *s = &row->string[edConfig.cx];
    int sSize = row->size - edConfig.cx;
    // insert row will strdup the string so we have a real copy of it
    edInsertRow(edConfig.cy + 1, s, sSize);

    // TODO(noxet): cleanup unused mem?
    row->string[edConfig.cx] = '\0';
    row->size -= sSize;
    edRenderRow(row);

    edConfig.cx = 0;
    edConfig.cy++;
}

char *edPrompt(char *prompt)
{
    size_t bufSize = 128;
    char *buf = malloc(bufSize);

    size_t bufLen = 0;
    buf[0] = '\0';

    while (1)
    {
        edSetStatusMessage(prompt, buf);
        edRefreshScreen();

        int c = termReadKey();
        if (c == '\r')
        {
            if (bufLen != 0)
            {
                edSetStatusMessage("");
                return buf;
            }
        }
    }
}

/*
 * Converts the row struct to a normal, NULL-terminated string to be written to file
 */
char *edRowsToString(int *bufLen)
{
    int totLen = 0;
    for (int i = 0; i < edConfig.numRows; i++)
    {
        totLen += edConfig.row[i].size + 1;
    }
    totLen++; // NULL byte

    *bufLen = totLen;
    LOG("Total string len: %d", totLen);

    char *buf = malloc(totLen);
    assert(buf != NULL);

    char *p = buf;
    for (int i = 0; i < edConfig.numRows; i++)
    {
        memcpy(p, edConfig.row[i].string, edConfig.row[i].size);
        p += edConfig.row[i].size;
        *p = '\n';
        p++;
    }

    *p = '\0';

    return buf;
}

void edOpen(const char *filename)
{
    assert(filename != NULL);

    FILE *inputFile = fopen(filename, "r");
    if (!inputFile) errExit("Failed to open file: %s", filename);
    free(edConfig.filename);
    edConfig.filename = strdup(filename);

    char *line = NULL;
    size_t bufferSize = 0;
    ssize_t lineLen = 0;
    while ((lineLen = getline(&line, &bufferSize, inputFile)) != -1)
    {
        // TODO(noxet): hack to avoid BO, fix later with reallocations
        if (edConfig.numRows >= 1000) break;

        // remove newline char(s) if present and terminate string
        while (lineLen > 0 && (line[lineLen - 1] == '\n' || line[lineLen - 1] == '\r')) lineLen--;
        edInsertRow(edConfig.numRows, line, lineLen);
    }

    free(line);
    fclose(inputFile);

    // since we call appendRow here, it will be falsely set to dirty
    edConfig.dirty = false;
}

void edSaveFile(const char *filename)
{
    // TODO(noxet): handle this case later
    if (filename == NULL) return;

    int bufLen = 0;
    char *content = edRowsToString(&bufLen);

    FILE *fd = fopen(filename, "w");
    if (!fd) errExit("Failed to open file: %s", filename);
    int ret = fwrite(content, 1, bufLen, fd);
    if (ret != bufLen) errExit("Failed to save all bytes to file: %s", filename);
    fclose(fd);
    free(content);
    edConfig.dirty = false;
}

void edInit()
{
    edConfig.cx = 0;
    edConfig.cy = 0;
    edConfig.rx = 0;
    edConfig.row = calloc(1000, sizeof(*edConfig.row));
    edConfig.numRows = 0;
    edConfig.rowOffset = 0;
    edConfig.colOffset = 0;
    edConfig.filename = NULL;
    edConfig.statusMsg[0] = '\0';
    edConfig.statusMsgTime = 0;
    edConfig.dirty = false;

    // TODO(noxet): Handle window resize event
    if (termGetWindowSize(&edConfig.winRows, &edConfig.winCols) == -1) errExit("Failed to get window size");
    // make room for the status bar and messages at the end
    // TODO(noxet): Fix this later by using "pane" size or similar, which is independent of window size
    edConfig.winRows -= 2;


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

    edSetStatusMessage("HELP: CTRL-S to save | CTRL-Q to quit");

    while (nedRunning)
    {
        edRefreshScreen();
        edProcessKey();
    }

    if (termDisableRawMode() == -1) errExit("Restoring userTerm failed");



    return 0;
}
