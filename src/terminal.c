#include "utils.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/errno.h>


static struct termios userTerm;


static void sigHandler(int sig)
{
    UNUSED(sig);

    // restore starting terminal state
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &userTerm) == -1)
    {
        printf("[%s] - Failed to set tcsetattr\r\n", __func__);
    }

    _exit(EXIT_SUCCESS);
}

int termSetupSignals()
{
    struct sigaction sa, prev;
    sa.sa_handler = sigHandler;
    if (sigaction(SIGTERM, &sa, NULL) == -1) return -1;
    return 0;
}

void enableRawMode()
{
    struct termios raw;

    tcgetattr(STDIN_FILENO, &raw);

    printf("termios fields:\n");
    printf("\tiflag: 0x%04lx\n", raw.c_iflag);
    printf("\toflag: 0x%04lx\n", raw.c_oflag);
    printf("\tcflag: 0x%04lx\n", raw.c_cflag);
    printf("\tlflag: 0x%04lx\n", raw.c_lflag);

    //raw.c_cc[VINTR] = 12;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

}

void setEcho(bool state)
{
    struct termios tp;

    tcgetattr(STDIN_FILENO, &tp);
    
    if (state == true)
    {
        tp.c_lflag |= ECHO;
    }
    else
    {
        tp.c_lflag &= ~ECHO;
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tp);
}

void toggleEcho()
{
    struct termios tp;

    tcgetattr(STDIN_FILENO, &tp);
    
    tp.c_lflag ^= ECHO;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tp);

}

int termEnableRawMode()
{
    struct termios term;

    if (tcgetattr(STDIN_FILENO, &term) == -1) return -1;

    // copy the startup state of the terminal so we can restore later
    userTerm = term;

    term.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
    term.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | 
            INPCK | ISTRIP | IXON | PARMRK);
    term.c_oflag &= ~OPOST;

    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1) return -1;

    return 0;
}

int termDisableRawMode()
{
    // restore the saved config
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &userTerm) == -1) return -1;
    return 0;
}

char termReadKey()
{
    char ch = '\0';
    ssize_t nread;
    while ((nread = read(STDIN_FILENO, &ch, 1)) != 1)
    {
        if (nread == -1 && errno != EAGAIN) errExit("Failed to read input key");
    }

    return ch;
}

