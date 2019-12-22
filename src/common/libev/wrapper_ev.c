#include "wrapper_ev.h"

// libev produces many warnings which isn't really appropriate for us to
// address since it is 3rd party code that could be overwritten at any time
// with a new version
#pragma warning(disable:4100 4101 4127 4189 4245 4706 4996 4068)
#pragma GCC diagnostic ignored "-Wcomment"
#pragma GCC diagnostic ignored "-Wold-style-declaration"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wunused-variable"

#if (__GNUC__ >= 6)
	#pragma GCC diagnostic ignored "-Wmisleading-indentation"
	#pragma GCC diagnostic ignored "-Wreturn-type"
#endif
#if (__GNUC__ >= 7)
	#pragma GCC diagnostic ignored "-Wdangling-else"
#endif

#ifdef __MINGW32__
#include <time.h>
struct timespec
{
    time_t tv_sec;
    long int tv_nsec;
};
#endif  // __MINGW32__
#include "ev.c"
