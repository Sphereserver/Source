# Main game files.
SET (game_SRCS
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
graysvr/CClientGMPage.cpp
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
graysvr/CServer.h
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
SOURCE_GROUP (Game FILES ${game_SRCS})

# Files containing 'background work'
SET (common_SRCS
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
common/os_windows.h
common/twofish/twofish2.cpp
common/mtrand/mtrand.h
common/regex/deelx.h
)
SOURCE_GROUP (Common FILES ${common_SRCS})

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
graysvr/CPingServer.cpp
graysvr/CPingServer.h
)
SOURCE_GROUP (Network FILES ${network_SRCS})

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
graysvr/CNTService.cpp
graysvr/CNTService.h
graysvr/CNTWindow.cpp
graysvr/CUnixTerminal.cpp
graysvr/CUnixTerminal.h
graysvr/SphereSvr.rc
)
SOURCE_GROUP (Sphere FILES ${sphere_SRCS})

# CrashDump files
SET (crashdump_SRCS
common/crashdump/crashdump.cpp
common/crashdump/crashdump.h
common/crashdump/mingwdbghelp.h
)
SOURCE_GROUP (CrashDump FILES ${crashdump_SRCS})

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
SOURCE_GROUP (libev FILES ${libev_SRCS})

# SQLite files
SET (sqlite_SRCS
common/sqlite/sqlite3.c
common/sqlite/sqlite3.h
common/sqlite/SQLite.cpp
common/sqlite/SQLite.h
)
SOURCE_GROUP (SQLite FILES ${sqlite_SRCS})

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
SOURCE_GROUP (ZLib FILES ${zlib_SRCS})

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
SOURCE_GROUP (Tables FILES ${tables_SRCS})

# Misc doc and *.ini files
SET (docs_TEXT
../docs/REVISIONS-51-54-SERIES.TXT
../docs/REVISIONS-55-SERIES.TXT
../docs/REVISIONS-56-Pre_release.TXT
../docs/REVISIONS-56-SERIES.TXT
sphere.ini
sphereCrypt.ini
)

INCLUDE_DIRECTORIES (
common/mysql/include/
common/
)
