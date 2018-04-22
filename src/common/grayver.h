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
	#define SPHERE_VERSION			SPHERE_VER_STR "-Debug"
	#define SPHERE_VER_FILEFLAGS	0x1L	//VS_FF_DEBUG
#elif defined(_NIGHTLYBUILD)
	#define SPHERE_VERSION			SPHERE_VER_STR "-Nightly"
	#define SPHERE_VER_FILEFLAGS	0x2L	//VS_FF_PRERELEASE
#elif defined(_PRIVATEBUILD)
	#define SPHERE_VERSION			SPHERE_VER_STR "-Private"
	#define SPHERE_VER_FILEFLAGS	0x8L	//VS_FF_PRIVATEBUILD
#else
	#define SPHERE_VERSION			SPHERE_VER_STR "-Release"
	#define SPHERE_VER_FILEFLAGS	0x0L
#endif

#if defined(_WIN32)
	#define SPHERE_VER_FILEOS		0x4L	//VOS__WINDOWS32
	#define SPHERE_VER_FILEOS_STR	"[WIN32]"
#elif defined(_BSD)
	#define SPHERE_VER_FILEOS		0x0L	//VOS_UNKNOWN
	#define SPHERE_VER_FILEOS_STR	"[FreeBSD]"
#else
	#define SPHERE_VER_FILEOS		0x0L	//VOS_UNKNOWN
	#define SPHERE_VER_FILEOS_STR	"[Linux]"
#endif

#endif	// _INC_GRAYVER_H
