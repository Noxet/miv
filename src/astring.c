#include "astring.h"

#include <stdlib.h>
#include <string.h>

struct astring_s
{
    char *buf;
    int len;
};

astring* astringNew()
{
    astring *astr = malloc(sizeof(*astr));
    astr->buf = NULL;
    astr->len = 0;

    return astr;
}

int astringAppend(astring *astr, const char *text, int len)
{
    char *newBuf = realloc(astr->buf, astr->len + len);
    if (!newBuf) return -1;

    memcpy(&newBuf[astr->len], text, len);

    astr->buf = newBuf;
    astr->len += len;

    return 0;
}

const char *astringGetString(astring *astr)
{
    return astr->buf;
}

int astringGetLen(astring *astr)
{
    return astr->len;
}

void astringFree(astring **astr)
{
    free((*astr)->buf);
    free(*astr);
}
