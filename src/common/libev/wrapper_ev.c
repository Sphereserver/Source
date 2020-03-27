#include "wrapper_ev.h"

// libev produces many warnings which isn't really appropriate for us to
// address since it is 3rd party code that could be overwritten at any time
// with a new version
#pragma GCC diagnostic ignored "-Wcomment"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-value"

#ifdef __MINGW32__
#include <time.h>
struct timespec
{
    time_t tv_sec;
    long int tv_nsec;
};
#endif  // __MINGW32__
#include "ev.c"
