# Main config.
PROJECT (sphereserver)

CMAKE_MINIMUM_REQUIRED (VERSION 2.6.0)

# Sources.

SET (graysvr_SRCS
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
graysvr/CItemSp.cpp
graysvr/CItemStone.cpp
graysvr/CItemVend.cpp
graysvr/CLog.cpp
graysvr/CObjBase.cpp
graysvr/CObjBase.h
graysvr/CPathFinder.cpp
graysvr/CPathFinder.h
graysvr/CQuest.cpp
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
#graysvr/GraySvr.rc
#graysvr/libmysqlclient.so
#graysvr/ntservice.cpp
#graysvr/ntservice.h
#graysvr/ntwindow.cpp
graysvr/PingServer.cpp
graysvr/PingServer.h
graysvr/resource.h
graysvr/UnixTerminal.cpp
graysvr/UnixTerminal.h
)

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
#common/CMapCache.cpp
#common/CMapCache.h
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
#common/CScriptCompiler.cpp
#common/CScriptCompiler.h
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
#common/CWindow.cpp
#common/CWindow.h
common/graycom.cpp
common/graycom.h
common/graymul.h
common/grayproto.h
common/grayver.h
common/ListDefContMap.cpp
common/ListDefContMap.h
common/os_unix.h
common/os_windows.h

)

SET (network_SRCS
network/network.cpp
network/network.h
network/packet.cpp
network/packet.h
network/receive.cpp
network/receive.h
network/send.cpp
network/send.h
)

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
)

SET (crashdump_SRCS
common/crashdump/crashdump.cpp
common/crashdump/crashdump.h
)

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

SET (mtrand_SRCS
common/mtrand/mtrand.h
)

SET (regex_SRCS
common/regex/deelx.h
)

SET (sqlite_SRCS
common/sqlite/sqlite3.c
common/sqlite/sqlite3.h
common/sqlite/SQLite.cpp
common/sqlite/SQLite.h
)

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

SET (twofish_SRCS
common/twofish/twofish2.cpp
)


INCLUDE_DIRECTORIES (
common/mysql/include/
common/
)


IF (WIN32)
# No windows build! Need fix!
ELSE (WIN32)
# Get git revision / version.
SET (GIT_CMD git)
SET (GIT_ARGS rev-parse HEAD)
SET (GIT_REV_CMD git rev-list --count HEAD)
MESSAGE (STATUS "Checking git revision...")
EXECUTE_PROCESS (COMMAND ${GIT_REV_CMD}
WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
OUTPUT_VARIABLE GITREVCMD
OUTPUT_STRIP_TRAILING_WHITESPACE
)
MATH ( EXPR GITREVCMD "${GITREVCMD} + 410")
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


# Set cxx and c compiler flags.
SET (CXX_WARNING_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-switch")
SET (CXX_ARCH_OPTS "-march=i686 -m32")
SET (CXX_OPTS "-fno-omit-frame-pointer -ffast-math -fpermissive -O0 -fno-expensive-optimizations")
SET (CXX_SPECIAL "-fexceptions -fnon-call-exceptions")
SET (C_WARNING_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -Wno-switch")
SET (C_ARCH_OPTS "-march=i686 -m32")
SET (C_OPTS "-fno-omit-frame-pointer -ffast-math -O0 -fno-expensive-optimizations")
SET (C_SPECIAL "-fexceptions -fnon-call-exceptions")
SET (CMAKE_CXX_FLAGS "${CXX_WARNING_FLAGS} ${CXX_ARCH_OPTS} ${CXX_OPTS} ${CXX_SPECIAL}")
SET (CMAKE_C_FLAGS "${C_WARNING_FLAGS} ${C_ARCH_OPTS} ${C_OPTS} ${C_SPECIAL}")

LINK_DIRECTORIES ( /usr/lib )

# Two executables: 1 for nightlies and 1 for release.

ADD_EXECUTABLE(spheresvrNightly
${graysvr_SRCS}
${common_SRCS}
${network_SRCS}
${sphere_SRCS}
${crashdump_SRCS}
${libev_SRCS}
${mtrand_SRCS}
${regex_SRCS}
${sqlite_SRCS}
${zlib_SRCS}
${twofish_SRCS}
)

ADD_EXECUTABLE(spheresvr
${graysvr_SRCS}
${common_SRCS}
${network_SRCS}
${sphere_SRCS}
${crashdump_SRCS}
${libev_SRCS}
${mtrand_SRCS}
${regex_SRCS}
${sqlite_SRCS}
${zlib_SRCS}
${twofish_SRCS}
)

TARGET_COMPILE_DEFINITIONS (spheresvr
# Extra defs
PUBLIC _MTNETWORK
# SVN defs
PUBLIC _SUBVERSION
# Defines
PUBLIC GRAY_SVR
PUBLIC _CONSOLE
PUBLIC _REENTRANT
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
PUBLIC GRAY_SVR
PUBLIC _CONSOLE
PUBLIC _REENTRANT
PUBLIC _LINUX
)

TARGET_LINK_LIBRARIES (
spheresvr
pthread 
mysqlclient
rt
dl
)

TARGET_LINK_LIBRARIES (
spheresvrNightly
pthread 
mysqlclient
rt
dl
)

# Need to clear shared library flags. If not, cmake sets -rdynamic and this 
# add to the executable the full symbol table (included unused symbols).
# This is a problem because the binary is ~700 KB bigger.
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
# Force dynamic linking.
SET(CMAKE_EXE_LINKER_FLAGS "-dynamic")

# For release version force full optimization.
SET_TARGET_PROPERTIES (spheresvr PROPERTIES 
COMPILE_FLAGS -O3
)
ENDIF (WIN32)

