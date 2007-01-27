#include "regexp_int.h"
#include "regexp_custom.h"

#include <stdlib.h>

void* re_malloc(size_t sz)
{
    return malloc(sz);
}

void re_cfree(void* p)
{
    free(p);
}

