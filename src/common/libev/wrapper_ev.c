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

#include "ev.c"
