#ifdef _SUBVERSION
 #include "./subversion/SvnRevision.h"
#endif
#ifdef __SVNREVISION__
 #define GRAY_VER_BUILD			__SVNREVISION__
#else
 #define GRAY_VER_BUILD			0
#endif

#define GRAY_VER_FILEVERSION		0,56,3,GRAY_VER_BUILD		// version to be set on generated .exe file
#define	GRAY_VER_NUM				0x00005603L					// version for some internal usage, like compiled scripts
#define GRAY_VER_STR				"0.56c"						// share version with all files

#if defined(_DEBUG)
 #define GRAY_VERSION				GRAY_VER_STR "-Debug"
 #define GRAY_VER_FILEFLAGS			0x1L	//VS_FF_DEBUG
#elif defined(_NIGHTLYBUILD)
 #define GRAY_VERSION				GRAY_VER_STR "-Nightly"
 #define GRAY_VER_FILEFLAGS			0x2L	//VS_FF_PRERELEASE
#elif defined(_PRIVATEBUILD)
 #define GRAY_VERSION				GRAY_VER_STR "-Private"
 #define GRAY_VER_FILEFLAGS			0x8L	//VS_FF_PRIVATEBUILD
#else
 #define GRAY_VERSION				GRAY_VER_STR "-Release"
 #define GRAY_VER_FILEFLAGS			0x0L
#endif

#if defined(_WIN32)
 #define GRAY_VER_FILEOS			0x4L	//VOS__WINDOWS32
 #define GRAY_VER_FILEOS_STR		"[WIN32]"
#elif defined(_BSD)
 #define GRAY_VER_FILEOS			0x0L	//VOS_UNKNOWN
 #define GRAY_VER_FILEOS_STR		"[FreeBSD]"
#else
 #define GRAY_VER_FILEOS			0x0L	//VOS_UNKNOWN
 #define GRAY_VER_FILEOS_STR		"[Linux]"
#endif
