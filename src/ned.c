#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>

#define UNUSED(x) (void) (x)

static struct termios userTerm;

void enableRawMode()
{
    struct termios raw;

    tcgetattr(STDIN_FILENO, &raw);

    printf("termios fields:\n");
    printf("\tiflag: 0x%04x\n", raw.c_iflag);
    printf("\toflag: 0x%04x\n", raw.c_oflag);
    printf("\tcflag: 0x%04x\n", raw.c_cflag);
    printf("\tlflag: 0x%04x\n", raw.c_lflag);

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

int ttySetRawMode(struct termios *prevTerm)
{
    struct termios term;

    if (tcgetattr(STDIN_FILENO, &term) == -1) return -1;

    // copy the startup state of the terminal so we can restore later
    if (prevTerm != NULL)
    {
        *prevTerm = term;
    }

    term.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
    term.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | 
            INPCK | ISTRIP | IXON | PARMRK);
    term.c_oflag &= ~OPOST;

    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1) return -1;

    return 0;
}

static void handler(int sig)
{
    UNUSED(sig);

    // restore starting terminal state
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &userTerm) == -1)
    {
        printf("[%s] - Failed to set tcsetattr\n", __func__);
    }

    _exit(EXIT_SUCCESS);
}

int main()
{
    if (ttySetRawMode(&userTerm) == -1)
    {
        printf("[%s] - ttySetRawMode failed\n", __func__);
        exit(EXIT_SUCCESS);
    }

    struct sigaction sa, prev;
    sa.sa_handler = handler;
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        printf("[%s] - sigaction failed\n", __func__);
        exit(EXIT_SUCCESS);
    }

    // disable stdout buffering
    setbuf(stdout, NULL);

    char ch;
    while (true)
    {
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n == -1)
        {
            printf("[%s] - read failed\n", __func__);
            break;
        }
        else if (n == 0)
        {
            // can happen if terminal disconnects
            break;
        }

        if (isalpha(ch))
        {
            putchar(tolower(ch));
        }
        else if (ch == '\n' || ch == '\r')
        {
            putchar(ch);
        }
        else if (iscntrl(ch))
        {
            printf("^%c", ch ^ 64);
        }
        else
        {
            putchar('*');
        }

        if (ch == 'q') break;

        if (ch == 12)
        {
            // Ctrl-L
            //toggleEcho();
        }

    }

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &userTerm) == -1)
    {
        printf("Restoring userTerm failed\n");
    }

    return 0;
}
