
#ifndef __WRAPPER_EV__H
#define __WRAPPER_EV__H

#ifdef _WIN32
	#undef FD_SETSIZE
	#define FD_SETSIZE 1024
	#define EV_SELECT_IS_WINSOCKET 1

	#ifndef EV_FD_TO_WIN32_HANDLE
	# define EV_FD_TO_WIN32_HANDLE(fd) _get_osfhandle (fd)
	#endif
	#ifndef EV_WIN32_HANDLE_TO_FD
	# define EV_WIN32_HANDLE_TO_FD(handle) _open_osfhandle (handle, 0)
	#endif
	#ifndef EV_WIN32_CLOSE_FD
	# define EV_WIN32_CLOSE_FD(fd) close (fd)
	#endif
#else
	#define EV_CONFIG "ev_config.h"
#endif
#define EV_STANDALONE 1

#endif
