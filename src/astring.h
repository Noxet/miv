#pragma once

typedef struct astring_s astring;

#define NEW_STRING {NULL, 0}

astring* astringNew();
int astringAppend(astring *astr, const char *text, int len);
const char *astringGetString(astring *astr);
int astringGetLen(astring *astr);
void astringFree(astring **astr);

