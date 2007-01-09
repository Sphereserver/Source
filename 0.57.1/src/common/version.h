
#define	SPHERE_VER_NUM	0x00005701L	// version for some internal usage, like compiled scripts
#define SPHERE_VER_STR	"0.57.1"	// share version with all files.
#define SPHERE_TITLE "Sphere"

#ifdef _NIGHTLYBUILD
	#define SPHERE_VERSION SPHERE_VER_STR "-Nightly"
#else
	#define SPHERE_VERSION SPHERE_VER_STR
#endif

#ifdef _WIN32
#define	SPHERE_OS	"WIN32"
#else
#ifdef _BSD
#define SPHERE_OS	"FreeBSD"
#else
#define SPHERE_OS	"Linux"
#endif
#endif

#ifdef _DEBUG
#define SPHERE_EXINFO	"[DEBUG]"
#else
#define SPHERE_EXINFO	""
#endif

#define	SPHERE_FULL		SPHERE_TITLE " Version " SPHERE_VERSION " [" SPHERE_OS  "]" SPHERE_EXINFO
