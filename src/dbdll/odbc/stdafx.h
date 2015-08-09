/*
 *	Copyright (c) 2010 Thomas Lowe
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or sell
 *	copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	Except as contained in this notice, the name(s) of the above 
 *	copyright holders shall not be used in advertising or otherwise 
 *	to promote the sale, use or other dealings in this Software 
 *	without prior written authorization.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *	OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _STDAFX_H
#define _STDAFX_H

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
	#include <windows.h>
#else
	#include <sys/socket.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <signal.h>
	#include <fcntl.h>

	// Compatibility stuff.
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR    (-1)
	#define SOCKET			int
	#define TCP_NODELAY		0x0001
#endif

#ifndef UNREFERENCED_PARAMETER
	#define UNREFERENCED_PARAMETER(P)	(void)(P)
#endif

#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>

#endif
