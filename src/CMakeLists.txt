cmake_minimum_required(VERSION 3.1)
project(SphereServer)

# Project files
set(common_SRCS
    common/crashdump/crashdump.cpp
    common/crashdump/crashdump.h
    common/crashdump/mingwdbghelp.h
    common/libev/wrapper_ev.c
    common/libev/wrapper_ev.h
    common/sqlite/SQLite.cpp
    common/sqlite/SQLite.h
    common/sqlite/sqlite3.c
    common/sqlite/sqlite3.h
    common/twofish/twofish2.cpp
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
    common/CArray.cpp
    common/CArray.h
    common/CAssoc.cpp
    common/CAssoc.h
    common/CCacheableScriptFile.cpp
    common/CCacheableScriptFile.h
    common/CCSVFile.cpp
    common/CCSVFile.h
    common/CDataBase.cpp
    common/CDataBase.h
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
    common/CListDefMap.cpp
    common/CListDefMap.h
    common/CMD5.cpp
    common/CMD5.h
    common/CMemBlock.h
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
    common/os_common.h
    common/os_unix.h
    common/os_windows.h)

set(graysvr_SRCS
    graysvr/CAccount.cpp
    graysvr/CAccount.h
    graysvr/CBase.cpp
    graysvr/CBase.h
    graysvr/CCharAct.cpp
    graysvr/CCharBase.cpp
    graysvr/CCharBase.h
    graysvr/CChar.cpp
    graysvr/CChar.h
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
    graysvr/CChat.h
    graysvr/CClient.cpp
    graysvr/CClientDialog.cpp
    graysvr/CClientEvent.cpp
    graysvr/CClient.h
    graysvr/CClientLog.cpp
    graysvr/CClientMsg.cpp
    graysvr/CClientTarg.cpp
    graysvr/CClientUse.cpp
    graysvr/CContain.cpp
    graysvr/CGMPage.cpp
    graysvr/CGMPage.h
    graysvr/CItemBase.cpp
    graysvr/CItemBase.h
    graysvr/CItem.cpp
    graysvr/CItem.h
    graysvr/CItemMulti.cpp
    graysvr/CItemMulti.h
    graysvr/CItemMultiCustom.cpp
    graysvr/CItemMultiCustom.h
    graysvr/CItemShip.cpp
    graysvr/CItemShip.h
    graysvr/CItemSpawn.cpp
    graysvr/CItemStone.cpp
    graysvr/CItemStone.h
    graysvr/CItemVend.cpp
    graysvr/CLog.cpp
    graysvr/CLog.h
    graysvr/CObjBase.cpp
    graysvr/CObjBase.h
    graysvr/CParty.cpp
    graysvr/CParty.h
    graysvr/CPathFinder.cpp
    graysvr/CPathFinder.h
    graysvr/CPingServer.cpp
    graysvr/CPingServer.h
    graysvr/CResourceCalc.cpp
    graysvr/CResource.cpp
    graysvr/CResourceDef.cpp
    graysvr/CResource.h
    graysvr/CSector.cpp
    graysvr/CSector.h
    graysvr/CServer.cpp
    graysvr/CServer.h
    graysvr/CServRef.cpp
    graysvr/CServRef.h
    graysvr/CWebPage.cpp
    graysvr/CWebPage.h
    graysvr/CWorld.cpp
    graysvr/CWorld.h
    graysvr/CWorldImport.cpp
    graysvr/CWorldMap.cpp
    graysvr/graysvr.cpp
    graysvr/graysvr.h
    graysvr/resource.h)

set(network_SRCS
    network/network.cpp
    network/network.h
    network/packet.cpp
    network/packet.h
    network/receive.cpp
    network/receive.h
    network/send.cpp
    network/send.h)

set(sphere_SRCS
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
    graysvr/CNTService.cpp
    graysvr/CNTService.h
    graysvr/CNTWindow.cpp
    graysvr/CNTWindow.h
    graysvr/CUnixTerminal.cpp
    graysvr/CUnixTerminal.h
    graysvr/SphereSvr.rc)

# Path to required external libs
include_directories(common/ common/mysql/include/)
link_directories(/usr/lib common/mysql/lib/)

# Use C++11 standard
set(CMAKE_CXX_STANDARD 11)

# Get Git revision
find_package(Git)
if(GIT_FOUND)
  execute_process(COMMAND git rev-list --count HEAD
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  OUTPUT_VARIABLE GITREVCMD
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(COMMAND git rev-parse --short HEAD
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  OUTPUT_VARIABLE GITHASHCMD
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

  math(EXPR GITREVCMD "${GITREVCMD} - 2406")
  message(STATUS "Current build revision: ${GITREVCMD} (Git hash: ${GITHASHCMD})")

  configure_file("${PROJECT_SOURCE_DIR}/common/version/GitRevision.h.in"
                 "${PROJECT_SOURCE_DIR}/common/version/GitRevision.h")
endif()

# Compile the project
add_executable(SphereSvr ${common_SRCS} ${graysvr_SRCS} ${network_SRCS} ${sphere_SRCS})
add_executable(SphereSvrNightly ${common_SRCS} ${graysvr_SRCS} ${network_SRCS} ${sphere_SRCS})

if(MSVC)
  include("CMakeMSVC.cmake")
elseif(MINGW)
  include("CMakeMINGW.cmake")
elseif(UNIX)
  include("CMakeUnix.cmake")
endif()
