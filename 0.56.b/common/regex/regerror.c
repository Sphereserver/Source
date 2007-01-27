/*
 * regerror
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "regexp.h"

struct error_message
{
    int err;
    const char* msg;
};

const struct error_message errors[] = {
    { 0, "no errors detected" },
    { REGEXP_BADARG,  "invalid argument" },
    { REGEXP_ESIZE,   "regular expression too big" },
    { REGEXP_ESPACE,  "out of memory" },
    { REGEXP_EPAREN,  "parenteses () not balanced" },
    { REGEXP_ERANGE,  "invalid character range" },
    { REGEXP_EBRACK,  "brackets [] not balanced" },
    { REGEXP_BADRPT,  "quantifier operator invalid" },
    { REGEXP_EESCAPE, "invalid escape \\ sequence" },
    { REGEXP_EEND,    "internal error!" },
    { 1,              "unknown error code (0x%x)!" }
};

#define ERROR_BUFFER_SIZE 80

void re_error(int errcode, const regexp* re, char* buffer, size_t bufsize)
{
    char convbuf[ERROR_BUFFER_SIZE];
    
    if(errcode >= 0)
    {
        strcpy(convbuf, errors[0].msg);
    }
    else
    {
        register int i;
        for(i = 1; errors[i].err != 1; ++i)
        {
            if(errors[i].err == errcode)
            {
                strcpy(convbuf, errors[i].msg);
                break;
            }
        }
        if(errors[i].err == 1)
            sprintf(convbuf, errors[i].msg, -errcode);
    }
    strncpy(buffer, convbuf, bufsize-1);
    buffer[bufsize-1] = '\0';    
}
