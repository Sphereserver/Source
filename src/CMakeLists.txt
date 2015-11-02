# Main config.
PROJECT (sphereserver)
 
CMAKE_MINIMUM_REQUIRED (VERSION 3.0.0)

INCLUDE ("CMakeSources.cmake")
INCLUDE ("CMakeGitStatus.cmake")

LINK_DIRECTORIES ("/usr/lib")
LINK_DIRECTORIES ("./common/mysql/lib/")

# Need to clear shared library flags. If not, cmake sets -rdynamic and this
# add to the executable the full symbol table (included unused symbols).
# This is a problem because the binary is ~700 KB bigger.
SET (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
SET (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

ADD_EXECUTABLE (spheresvr
		${game_SRCS}
		${common_SRCS}
		${network_SRCS}
		${sphere_SRCS}
		${crashdump_SRCS}
		${libev_SRCS}
		${sqlite_SRCS}
		${zlib_SRCS}
		${tables_SRCS}
		${docs_TEXT}
)

# Nightly project, maybe we should create it only with enabling a CMake option like 'BuildNightly' or something.
ADD_EXECUTABLE (spheresvrNightly
		${game_SRCS}
		${common_SRCS}
		${network_SRCS}
		${sphere_SRCS}
		${crashdump_SRCS}
		${libev_SRCS}
		${sqlite_SRCS}
		${zlib_SRCS}
		${tables_SRCS}
		${docs_TEXT}
)

IF (MSVC)
	INCLUDE ("CMakeMSVC.cmake")
ENDIF(MSVC)
IF (MINGW)
	INCLUDE ("CMakeMINGW.cmake")
ENDIF (MINGW)
IF (UNIX)
	INCLUDE ("CMakeUnix.cmake")
ENDIF (UNIX)
