#include "regexp_int.h"
#include "regexp_custom.h"
#include <stdlib.h>
#include <ctype.h>

/* default unicode routines.  They use the normal C runtime. */
/* Also contains an isblank() implementation if one isn't available */
#ifdef REGEXP_UNICODE
CHAR_TYPE* re_ansi_to_unicode(const char* s)
{
    size_t sz;
    CHAR_TYPE* data;
    
    sz = mbstowcs(NULL, s, 0);
    if(sz == (size_t)-1)
        return NULL;
    
    data = re_malloc((sz + 1) * sizeof(CHAR_TYPE));
    if(!data)
        return NULL;
    mbstowcs(data, s, sz + 1);

    return data;
}

char* re_unicode_to_ansi(const CHAR_TYPE* s)
{
    size_t sz;
    char* data;

    sz = wcstombs(NULL, s, 0);
    if(sz == (size_t)-1)
        return NULL;

    data = re_malloc((sz + 1) * sizeof(char));
    if(!data)
        return NULL;
    wcstombs(data, s, sz + 1);
    
    return data;
}

#ifndef isblank
int iswblank(wint_t wc)
{
    /* cheap implementation */
    return wc == L'\t' || wc == L' ';
}
#endif

#else

#ifndef isblank
int isblank(int c)
{
    return c == '\t' || c == ' ';
}
#endif

#endif
