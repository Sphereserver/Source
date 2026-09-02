#ifndef _INC_OS_WINDOWS_H
#define _INC_OS_WINDOWS_H
#pragma once

#ifdef _WIN32
#include <intsafe.h>
#include <io.h>
#include <process.h>
#include <time.h>

#undef FD_SETSIZE
#define FD_SETSIZE 1024		// override system default limit of 64 socket slots
#include <winsock2.h>
#endif	// _WIN32

#endif	// _INC_OS_WINDOWS_H
