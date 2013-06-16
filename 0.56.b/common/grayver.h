#define	GRAY_VER_NUM	0x00005602L	// version for some internal usage, like compiled scripts
#define GRAY_VER_STR	"0.56b"		// share version with all files.
#ifdef _NIGHTLYBUILD
# define GRAY_VERSION GRAY_VER_STR "-Nightly"
#else
# define GRAY_VERSION GRAY_VER_STR
#endif
#ifdef _PRERELEASE
# define GRAY_VERSION GRAY_VER_STR "-Pre-Release"
#else
# define GRAY_VERSION GRAY_VER_STR
#endif
