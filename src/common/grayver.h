#ifndef _INC_GRAYVER_H
#define _INC_GRAYVER_H
#pragma once

#include "version.h"
#if defined(GIT_COMMIT_COUNT)
	#define SPHERE_VER_BUILD		GIT_COMMIT_COUNT
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
#else
	#define SPHERE_VER_STR_FULL		SPHERE_VER_STR "-Release"
	#define SPHERE_VER_FILEFLAGS	0x0L
#endif

#define SPHERE_TITLE				"Sphere"
#define SPHERE_TITLE_VER			"Sphere V" SPHERE_VER_STR_FULL
#define SPHERE_WEBSITE				"www.spherecommunity.net"

#if defined(_WIN32)
	#define SPHERE_VER_FILEOS		VOS_NT_WINDOWS32
#else
	#define SPHERE_VER_FILEOS		VOS_UNKNOWN
#endif

#if defined(_WIN64) || defined(x64)
	#define SPHERE_VER_ARCH			"64bit"
#else
	#define SPHERE_VER_ARCH			"32bit"
#endif

#endif	// _INC_GRAYVER_H
