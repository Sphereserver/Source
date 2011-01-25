#include "wrapper_ev.h"

// libev produces many warnings which isn't really appropriate for us to
// address since it is 3rd party code that could be overwritten at any time
// with a new version
#pragma warning(disable:4100 4101 4127 4189 4245 4706 4996)
#include "ev.c"
