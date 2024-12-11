#include "syntax.h"
#include "utils.h"
#include "terminal.h"

#include <stdlib.h>
#include <string.h>


struct keyword
{
    char *key;
    termColor_e color;
};


static struct keyword keywords[] = 
{
    { "int", TERM_COLOR_RED },
};

termColor_e synGetKeyword(char *tok)
{
    for (int i = 0; i < ARRAY_SIZE(keywords); i++)
    {
        if (strcmp(tok, keywords[i].key) == 0) return keywords[i].color;
    }

    return TERM_COLOR_NONE;
}

size_t synCountKeywords(const char *string)
{
    size_t keywords = 0;
    char *str = strdup(string);
    char *token = strtok(str, " ");

    while (token)
    {
        if (synGetKeyword(token) != TERM_COLOR_NONE) keywords++;
        token = strtok(NULL, " ");
    }

    free(str);
    return keywords;
}
