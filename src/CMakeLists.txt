# Main config.
PROJECT (sphereserver)
 
CMAKE_MINIMUM_REQUIRED (VERSION 3.0.0)
# Main game files.
SET (game_SRCS
graysvr/CAccount.cpp
graysvr/CAccount.h
graysvr/CBase.cpp
graysvr/CBase.h
graysvr/CCharact.cpp
graysvr/CCharBase.cpp
graysvr/CChar.cpp
graysvr/CCharFight.cpp
graysvr/CCharNPCAct.cpp
graysvr/CCharNPC.cpp
graysvr/CCharNPCPet.cpp
graysvr/CCharNPCStatus.cpp
graysvr/CCharSkill.cpp
graysvr/CCharSpell.cpp
graysvr/CCharStatus.cpp
graysvr/CCharUse.cpp
graysvr/CChat.cpp
graysvr/CClient.cpp
graysvr/CClientDialog.cpp
graysvr/CClientEvent.cpp
graysvr/CClientGMPage.cpp
graysvr/CClient.h
graysvr/CClientLog.cpp
graysvr/CClientMsg.cpp
graysvr/CClientTarg.cpp
graysvr/CClientUse.cpp
graysvr/CContain.cpp
graysvr/CGMPage.cpp
graysvr/CItemBase.cpp
graysvr/CItem.cpp
graysvr/CItemMulti.cpp
graysvr/CItemMultiCustom.cpp
graysvr/CItemShip.cpp
graysvr/CItemSpawn.cpp
graysvr/CItemStone.cpp
graysvr/CItemVend.cpp
graysvr/CLog.cpp
graysvr/CObjBase.cpp
graysvr/CObjBase.h
graysvr/CPathFinder.cpp
graysvr/CPathFinder.h
graysvr/CParty.cpp
graysvr/CResourceCalc.cpp
graysvr/CResource.cpp
graysvr/CResourceDef.cpp
graysvr/CResource.h
graysvr/CSector.cpp
graysvr/CServer.cpp
graysvr/CServRef.cpp
graysvr/CServRef.h
graysvr/CWebPage.cpp
graysvr/CWorld.cpp
graysvr/CWorld.h
graysvr/CWorldImport.cpp
graysvr/CWorldMap.cpp
graysvr/graysvr.cpp
graysvr/graysvr.h
graysvr/resource.h
)
source_group(Game FILES ${game_SRCS})

# Files containing 'background work'
SET (common_SRCS
common/CacheableScriptFile.cpp
common/CacheableScriptFile.h
common/CArray.cpp
common/CArray.h
common/CAssoc.cpp
common/CAssoc.h
common/CAtom.cpp
common/CAtom.h
common/CDataBase.cpp
common/CDataBase.h
common/CDatabaseLoader.cpp
common/CDatabaseLoader.h
common/CEncrypt.cpp
common/CEncrypt.h
common/CException.cpp
common/CException.h
common/CExpression.cpp
common/CExpression.h
common/CFile.cpp
common/CFile.h
common/CFileList.cpp
common/CFileList.h
common/CGrayData.cpp
common/CGrayInst.cpp
common/CGrayInst.h
common/CGrayMap.cpp
common/CGrayMap.h
common/CMD5.cpp
common/CMD5.h
common/CMemBlock.h
common/common.h
common/CQueue.cpp
common/CQueue.h
common/CRect.cpp
common/CRect.h
common/CRegion.cpp
common/CRegion.h
common/CResourceBase.cpp
common/CResourceBase.h
common/CScript.cpp
common/CScript.h
common/CScriptObj.cpp
common/CScriptObj.h
common/CSectorTemplate.cpp
common/CSectorTemplate.h
common/CSocket.cpp
common/CSocket.h
common/CString.cpp
common/CString.h
common/CsvFile.cpp
common/CsvFile.h
common/CTime.cpp
common/CTime.h
common/CVarDefMap.cpp
common/CVarDefMap.h
common/CVarFloat.cpp
common/CVarFloat.h
common/CWindow.cpp
common/CWindow.h
common/graycom.cpp
common/graycom.h
common/graymul.h
common/grayproto.h
common/grayver.h
common/ListDefContMap.cpp
common/ListDefContMap.h
common/os_unix.h
common/os_windows.h
common/twofish/twofish2.cpp
common/mtrand/mtrand.h
common/regex/deelx.h
)
source_group(Common FILES ${common_SRCS})

# Network management files
SET (network_SRCS
network/network.cpp
network/network.h
network/packet.cpp
network/packet.h
network/receive.cpp
network/receive.h
network/send.cpp
network/send.h
graysvr/PingServer.cpp
graysvr/PingServer.h
)
source_group(Network FILES ${network_SRCS})

# Main program files: threads, console...
SET (sphere_SRCS
sphere/asyncdb.cpp
sphere/asyncdb.h
sphere/containers.h
sphere/linuxev.cpp
sphere/linuxev.h
sphere/mutex.cpp
sphere/mutex.h
sphere/ProfileData.cpp
sphere/ProfileData.h
sphere/strings.cpp
sphere/strings.h
sphere/threads.cpp
sphere/threads.h
graysvr/ntservice.cpp
graysvr/ntservice.h
graysvr/ntwindow.cpp
graysvr/UnixTerminal.cpp
graysvr/UnixTerminal.h
graysvr/GraySvr.rc
)
source_group(Sphere FILES ${sphere_SRCS})
 
# CrashDump files
SET (crashdump_SRCS
common/crashdump/crashdump.cpp
common/crashdump/crashdump.h
)
source_group(CrashDump FILES ${crashdump_SRCS})
 
# LibEv files
SET (libev_SRCS
#common/libev/ev.c
#common/libev/ev_config.h
#common/libev/event.c
#common/libev/event.h
#common/libev/ev_epoll.c
#common/libev/ev.h
#common/libev/ev++.h
#common/libev/ev_kqueue.c
#common/libev/ev_poll.c
#common/libev/ev_port.c
#common/libev/ev_select.c
#common/libev/ev_vars.h
#common/libev/ev_win32.c
#common/libev/ev_wrap.h
common/libev/wrapper_ev.c
common/libev/wrapper_ev.h
)
source_group(libev FILES ${libev_SRCS})

# SQLite files
SET (sqlite_SRCS
common/sqlite/sqlite3.c
common/sqlite/sqlite3.h
common/sqlite/SQLite.cpp
common/sqlite/SQLite.h
)
source_group(SQLite FILES ${sqlite_SRCS})

# ZLib files
SET (zlib_SRCS
common/zlib/adler32.c
common/zlib/compress.c
common/zlib/crc32.c
common/zlib/crc32.h
common/zlib/deflate.c
common/zlib/deflate.h
common/zlib/gzclose.c
common/zlib/gzguts.h
common/zlib/gzlib.c
common/zlib/gzread.c
common/zlib/gzwrite.c
common/zlib/infback.c
common/zlib/inffast.c
common/zlib/inffast.h
common/zlib/inffixed.h
common/zlib/inflate.c
common/zlib/inflate.h
common/zlib/inftrees.c
common/zlib/inftrees.h
common/zlib/trees.c
common/zlib/trees.h
common/zlib/uncompr.c
common/zlib/zconf.h
common/zlib/zlib.h
common/zlib/zutil.c
common/zlib/zutil.h
)
source_group(ZLib FILES ${zlib_SRCS})

# Table definitions
SET (tables_SRCS
tables/CBaseBaseDef_props.tbl
tables/CChar_functions.tbl
tables/CChar_props.tbl
tables/CCharBase_props.tbl
tables/CCharNpc_props.tbl
tables/CCharPlayer_functions.tbl
tables/CCharPlayer_props.tbl
tables/CClient_functions.tbl
tables/CClient_props.tbl
tables/CFile_functions.tbl
tables/CFile_props.tbl
tables/CFileObjContainer_functions.tbl
tables/CFileObjContainer_props.tbl
tables/CItem_functions.tbl
tables/CItem_props.tbl
tables/CItemBase_props.tbl
tables/CItemStone_functions.tbl
tables/CItemStone_props.tbl
tables/classnames.tbl
tables/CObjBase_functions.tbl
tables/CObjBase_props.tbl
tables/CParty_functions.tbl
tables/CParty_props.tbl
tables/CScriptObj_functions.tbl
tables/CSector_functions.tbl
tables/CStoneMember_functions.tbl
tables/CStoneMember_props.tbl
tables/defmessages.tbl
tables/triggers.tbl
)
source_group(Tables FILES ${tables_SRCS})

# Misc doc and *.ini files 
SET (docs_TEXT
../docs/REVISIONS-51-54-SERIES.TXT
../docs/REVISIONS-55-SERIES.TXT
../docs/REVISIONS-56-Pre_release.TXT
../docs/REVISIONS-56-SERIES.TXT
Sphere.ini
sphereCrypt.ini
)

INCLUDE_DIRECTORIES (
common/mysql/include/
common/
)

# Git revision checker
SET (GIT_CMD git)
SET (GIT_ARGS rev-parse HEAD)
SET (GIT_REV_CMD git rev-list --count HEAD)
MESSAGE (STATUS "Checking git revision...")
EXECUTE_PROCESS (COMMAND ${GIT_REV_CMD}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE GITREVCMD
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
MATH ( EXPR GITREVCMD "${GITREVCMD}")
MESSAGE (STATUS "Git revision ${GITREVCMD}")
MESSAGE (STATUS "Checking git revision hash...")
EXECUTE_PROCESS (COMMAND ${GIT_CMD} ${GIT_ARGS}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE GITHASHCMD
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
MESSAGE (STATUS "Git revision hash ${GITHASHCMD}")

CONFIGURE_FILE (
 "${PROJECT_SOURCE_DIR}/common/version/GitRevision.h.in"
 "${PROJECT_SOURCE_DIR}/common/version/GitRevision.h")

# Set cxx and c compiler flags for Windows.
if (MSVC)       #Visual Studio flags.
	# Here it is setting the Visual Studio warning level to 4 and forcint MultiProccessor compilation
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W4 -MP ")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W4 -MP")
endif(MSVC)

# Compiler flags for Linux.
if(CMAKE_C_COMPILER MATCHES "gcc" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
	SET (C_WARNING_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -Wno-switch")
	SET (CXX_WARNING_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-switch")
	SET (C_ARCH_OPTS "-march=i686 -m32")
	SET (CXX_ARCH_OPTS "-march=i686 -m32")
	SET (C_OPTS "-s -fno-omit-frame-pointer -ffast-math -O3 -fno-expensive-optimizations")
	SET (CXX_OPTS "-s -fno-omit-frame-pointer -ffast-math -fpermissive -O3")
	SET (C_SPECIAL "-fexceptions -fnon-call-exceptions")
	SET (CXX_SPECIAL "-fexceptions -fnon-call-exceptions")
	SET (CMAKE_C_FLAGS "${C_WARNING_FLAGS} ${C_ARCH_OPTS} ${C_OPTS} ${C_SPECIAL}")
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	SET (CMAKE_CXX_FLAGS "${CXX_WARNING_FLAGS} ${CXX_ARCH_OPTS} ${CXX_OPTS} ${CXX_SPECIAL}")
endif(CMAKE_C_COMPILER MATCHES "gcc" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")

LINK_DIRECTORIES ( /usr/lib )
LINK_DIRECTORIES(./common/mysql/lib/)

ADD_EXECUTABLE(spheresvr
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
ADD_EXECUTABLE(spheresvrNightly
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

# Need to clear shared library flags. If not, cmake sets -rdynamic and this
# add to the executable the full symbol table (included unused symbols).
# This is a problem because the binary is ~700 KB bigger.
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

# Setting different TARGET_COMPILE_DEFINITIONS between Linux and windows and for regurlar/nightly builds, also setting correct libraries for each platform
IF(WIN32)
	TARGET_COMPILE_DEFINITIONS (spheresvr
		# Extra defs
		PUBLIC _MTNETWORK
		# SVN defs
		PUBLIC _SUBVERSION
		# Defines
		PUBLIC _WIN32
		# Temporary setting _CRT_SECURE_NO_WARNINGS to do not spamm so much in the buil proccess while we get rid of -W4 warnings and, after it, -Wall.
		PUBLIC _CRT_SECURE_NO_WARNINGS
	)
	TARGET_COMPILE_DEFINITIONS (spheresvrNightly
		# Extra defs
		PUBLIC _MTNETWORK
		# SVN defs
		PUBLIC _SUBVERSION
		# Defines
		PUBLIC _NIGHTLYBUILD
		PUBLIC _WIN32
		PUBLIC _CRT_SECURE_NO_WARNINGS
	)
	set_target_properties(spheresvr 
		PROPERTIES 
			# Setting the exe to be a windows application and not a console one.
			LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
			# -O3 only works on gcc, using -Ox for 'full optimization' instead.
			COMPILE_FLAGS -Ox	
	)
	set_target_properties(spheresvrNightly 
		PROPERTIES 
			# Setting the exe to be a windows application and not a console one.
			LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
			# -O3 only works on gcc, using -Ox for 'full optimization' instead.
			COMPILE_FLAGS -Ox	
	)
	
	# Windows libs.
	TARGET_LINK_LIBRARIES ( spheresvr
		libmysql
		ws2_32
		wsock32
	)
	TARGET_LINK_LIBRARIES ( spheresvrNightly
		libmysql
		ws2_32
		wsock32
	)
ELSE(WIN32)
	TARGET_COMPILE_DEFINITIONS (spheresvr
		# Extra defs
		PUBLIC _MTNETWORK
		# SVN defs
		PUBLIC _SUBVERSION
		# Defines
		PUBLIC _CONSOLE
		PUBLIC _LINUX
	)
	TARGET_COMPILE_DEFINITIONS (spheresvrNightly
		# Nighly defs
		PUBLIC _NIGHTLYBUILD
		PUBLIC THREAD_TRACK_CALLSTACK
		# Debug defs 
		PUBLIC _DEBUG
		PUBLIC _PACKETDUMP
		PUBLIC _TESTEXCEPTION
		PUBLIC DEBUG_CRYPT_MSGS
		# Extra defs
		PUBLIC _MTNETWORK
		# SVN defs
		PUBLIC _SUBVERSION
		# Defines
		PUBLIC _CONSOLE
		PUBLIC _LINUX
	)
	# Optimization flags set to max.
	SET_TARGET_PROPERTIES (spheresvr PROPERTIES	COMPILE_FLAGS -O3 )
	# Force dynamic linking.
	SET(CMAKE_EXE_LINKER_FLAGS "-dynamic")
	
	# Linux libs.
	TARGET_LINK_LIBRARIES (
		spheresvr
		pthread
		mysqlclient
		rt
		dl
	)
ENDIF(WIN32)
