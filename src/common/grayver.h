#ifndef _INC_GRAYVER_H
#define _INC_GRAYVER_H
#pragma once

#include "version/GitRevision.h"
#if defined(__GITREVISION__)
	#define SPHERE_VER_BUILD		__GITREVISION__
#else
	#define SPHERE_VER_BUILD		0
#endif

#define SPHERE_VER_FILEVERSION		0,56,4,SPHERE_VER_BUILD		// version to be set on generated .exe file
#define SPHERE_VER_STR				"0.56d"						// share version with all files

#if defined(_DEBUG)
	#define SPHERE_VER_STR_FULL		SPHERE_VER_STR "-Debug"
	#define SPHERE_VER_FILEFLAGS	VS_FF_DEBUG
#elif defined(_NIGHTLYBUILD)
	#define SPHERE_VER_STR_FULL		SPHERE_VER_STR "-Nightly"
	#define SPHERE_VER_FILEFLAGS	VS_FF_PRERELEASE
#elif defined(_PRIVATEBUILD)
	#define SPHERE_VER_STR_FULL		SPHERE_VER_STR "-Private"
	#define SPHERE_VER_FILEFLAGS	VS_FF_PRIVATEBUILD
#else
	#define SPHERE_VER_STR_FULL		SPHERE_VER_STR "-Release"
	#define SPHERE_VER_FILEFLAGS	0x0L
#endif

#define SPHERE_TITLE				"Sphere"
#define SPHERE_TITLE_VER			"Sphere V" SPHERE_VER_STR_FULL
#define SPHERE_WEBSITE				"www.spherecommunity.net"

#if defined(_WIN32)
	#define SPHERE_VER_FILEOS		VOS_NT_WINDOWS32
#elif defined(_BSD)
	#define SPHERE_VER_FILEOS		VOS_UNKNOWN
#else
	#define SPHERE_VER_FILEOS		VOS_UNKNOWN
#endif

#if defined(_WIN32)
	#define SPHERE_PLATFORM			"Windows"
#elif defined(__linux__)
	#define SPHERE_PLATFORM			"Linux"
#elif defined(__FreeBSD__)
	#define SPHERE_PLATFORM			"FreeBSD"
#else
	#define SPHERE_PLATFORM			"Unknown"
#endif

#if defined(_WIN64) || defined(x64)
	#define SPHERE_VER_ARCH			"64-bit"
#else
	#define SPHERE_VER_ARCH			"32-bit"
#endif

#endif	// _INC_GRAYVER_H
