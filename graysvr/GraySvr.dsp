# Microsoft Developer Studio Project File - Name="GraySvr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=GRAYSVR - WIN32 DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GraySvr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GraySvr.mak" CFG="GRAYSVR - WIN32 DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GraySvr - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "GraySvr - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GraySvr", FCAAAAAA"
# PROP Scc_LocalPath "d:\menace\graysvr"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GraySvr - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /MD /W2 /GR /GX /O2 /Ob2 /I "../../common/boost/boost_1_33_1" /I "../../common/mysql/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "GRAY_SVR" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib libmysql.lib /nologo /version:0.10 /subsystem:windows /map /debug /machine:I386 /out:"Release/sphereSvr.exe" /libpath:"../../common/mysql/lib/release"
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "GraySvr - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gz /MDd /W2 /Gm /Gi /GR /GX /ZI /Od /I "../../common/boost/boost_1_33_1" /I "../../common/mysql/include" /D "WIN32" /D "_DEBUG" /D "__STL_DEBUG" /D "_WINDOWS" /D "GRAY_SVR" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib libmysql.lib /nologo /version:0.12 /subsystem:windows /map /debug /machine:I386 /out:"Debug\sphereSvr.exe" /pdbtype:sept /libpath:"../../common/mysql/lib/debug"
# SUBTRACT LINK32 /pdb:none /incremental:no

!ENDIF 

# Begin Target

# Name "GraySvr - Win32 Release"
# Name "GraySvr - Win32 Debug"
# Begin Group "Sources"

# PROP Default_Filter "cpp,c,h,rc"
# Begin Source File

SOURCE=.\CAccount.cpp
# End Source File
# Begin Source File

SOURCE=.\CBase.cpp
# End Source File
# Begin Source File

SOURCE=.\CChar.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharact.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharBase.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharFight.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharNPC.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharNPCAct.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharNPCPet.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharNPCStatus.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharSkill.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharSpell.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharStatus.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharUse.cpp
# End Source File
# Begin Source File

SOURCE=.\CChat.cpp
# End Source File
# Begin Source File

SOURCE=.\CClient.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientGMPage.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientLog.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientTarg.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientUse.cpp
# End Source File
# Begin Source File

SOURCE=.\CContain.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CEncrypt.cpp
# End Source File
# Begin Source File

SOURCE=.\CGMPage.cpp
# End Source File
# Begin Source File

SOURCE=.\CItem.cpp
# End Source File
# Begin Source File

SOURCE=.\CItemBase.cpp
# End Source File
# Begin Source File

SOURCE=.\CItemMulti.cpp
# End Source File
# Begin Source File

SOURCE=.\CItemSp.cpp
# End Source File
# Begin Source File

SOURCE=.\CItemStone.cpp
# End Source File
# Begin Source File

SOURCE=.\CItemVend.cpp
# End Source File
# Begin Source File

SOURCE=.\CLog.cpp
# End Source File
# Begin Source File

SOURCE=.\CObjBase.cpp
# End Source File
# Begin Source File

SOURCE=.\CPathFinder.cpp
# End Source File
# Begin Source File

SOURCE=.\CQuest.cpp
# End Source File
# Begin Source File

SOURCE=.\CResource.cpp
# End Source File
# Begin Source File

SOURCE=.\CResourceCalc.cpp
# End Source File
# Begin Source File

SOURCE=.\CResourceDef.cpp
# End Source File
# Begin Source File

SOURCE=.\CSector.cpp
# End Source File
# Begin Source File

SOURCE=.\CServer.cpp
# End Source File
# Begin Source File

SOURCE=.\CServRef.cpp
# End Source File
# Begin Source File

SOURCE=.\CWebPage.cpp
# End Source File
# Begin Source File

SOURCE=.\CWorld.cpp

!IF  "$(CFG)" == "GraySvr - Win32 Release"

!ELSEIF  "$(CFG)" == "GraySvr - Win32 Debug"

!ELSEIF  "$(CFG)" == "GraySvr - Win32 Testing"

# SUBTRACT CPP /D "GRAY_SVR"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CWorldImport.cpp
# End Source File
# Begin Source File

SOURCE=.\CWorldMap.cpp
# End Source File
# Begin Source File

SOURCE=.\graysvr.cpp
# End Source File
# Begin Source File

SOURCE=.\ntservice.cpp
# End Source File
# Begin Source File

SOURCE=.\ntwindow.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CAccount.h
# End Source File
# Begin Source File

SOURCE=.\CBase.h
# End Source File
# Begin Source File

SOURCE=.\CClient.h
# End Source File
# Begin Source File

SOURCE=..\common\CEncrypt.h
# End Source File
# Begin Source File

SOURCE=..\common\CMD5.h
# End Source File
# Begin Source File

SOURCE=.\CObjBase.h
# End Source File
# Begin Source File

SOURCE=.\CPathFinder.h
# End Source File
# Begin Source File

SOURCE=.\CResource.h
# End Source File
# Begin Source File

SOURCE=.\CServRef.h
# End Source File
# Begin Source File

SOURCE=..\common\CWindow.h
# End Source File
# Begin Source File

SOURCE=.\CWorld.h
# End Source File
# Begin Source File

SOURCE=.\graysvr.h
# End Source File
# Begin Source File

SOURCE=.\GraySvr.rc
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=..\common\zlib\compress.c
# End Source File
# Begin Source File

SOURCE=..\common\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=..\common\zlib\crc32.h
# End Source File
# Begin Source File

SOURCE=..\common\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=..\common\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=..\common\zlib\gzio.c
# End Source File
# Begin Source File

SOURCE=..\common\zlib\infback.c
# End Source File
# Begin Source File

SOURCE=..\common\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=..\common\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=..\common\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\common\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=..\common\zlib\inflate.h
# End Source File
# Begin Source File

SOURCE=..\common\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\common\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\common\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=..\common\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=..\common\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\common\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=..\common\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=..\common\zlib\zutil.c
# End Source File
# Begin Source File

SOURCE=..\common\zlib\zutil.h
# End Source File
# End Group
# Begin Group "twofish"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\twofish\aes.h
# End Source File
# Begin Source File

SOURCE=..\common\twofish\debug.h
# End Source File
# Begin Source File

SOURCE=..\common\twofish\platform.h
# End Source File
# Begin Source File

SOURCE=..\common\twofish\table.h
# End Source File
# Begin Source File

SOURCE=..\common\twofish\twofish2.cpp
# End Source File
# End Group
# Begin Group "mtrand"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\mtrand\mtrand.cpp
# End Source File
# Begin Source File

SOURCE=..\common\mtrand\mtrand.h
# End Source File
# End Group
# Begin Group "regexp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\regex\deelx.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\common\CArray.cpp
# End Source File
# Begin Source File

SOURCE=..\common\carray.h
# End Source File
# Begin Source File

SOURCE=..\common\CAssoc.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cassoc.h
# End Source File
# Begin Source File

SOURCE=..\common\CAtom.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CAtom.h
# End Source File
# Begin Source File

SOURCE=..\common\CDataBase.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CDataBase.h
# End Source File
# Begin Source File

SOURCE=..\common\CDatabaseLoader.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CDatabaseLoader.h
# End Source File
# Begin Source File

SOURCE=..\common\CException.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CException.h
# End Source File
# Begin Source File

SOURCE=..\common\CExpression.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cexpression.h
# End Source File
# Begin Source File

SOURCE=..\common\CFile.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cfile.h
# End Source File
# Begin Source File

SOURCE=..\common\CFileList.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cfilelist.h
# End Source File
# Begin Source File

SOURCE=..\common\CGrayData.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CGrayInst.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cgrayinst.h
# End Source File
# Begin Source File

SOURCE=..\common\CGrayMap.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cGrayMap.h
# End Source File
# Begin Source File

SOURCE=..\common\CMapCache.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CMapCache.h
# End Source File
# Begin Source File

SOURCE=..\common\CMD5.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cmemblock.h
# End Source File
# Begin Source File

SOURCE=..\common\common.h
# End Source File
# Begin Source File

SOURCE=..\common\CQueue.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CQueue.h
# End Source File
# Begin Source File

SOURCE=..\common\CRect.cpp
# End Source File
# Begin Source File

SOURCE=..\common\crect.h
# End Source File
# Begin Source File

SOURCE=..\common\CRegion.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cregion.h
# End Source File
# Begin Source File

SOURCE=..\common\CResourceBase.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cresourcebase.h
# End Source File
# Begin Source File

SOURCE=..\common\CScript.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cscript.h
# End Source File
# Begin Source File

SOURCE=..\common\CScriptObj.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cscriptobj.h
# End Source File
# Begin Source File

SOURCE=..\common\CSectorTemplate.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cSectorTemplate.h
# End Source File
# Begin Source File

SOURCE=..\common\CSocket.cpp
# End Source File
# Begin Source File

SOURCE=..\common\csocket.h
# End Source File
# Begin Source File

SOURCE=..\common\CString.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cstring.h
# End Source File
# Begin Source File

SOURCE=..\common\CThread.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cthread.h
# End Source File
# Begin Source File

SOURCE=..\common\CTime.cpp
# End Source File
# Begin Source File

SOURCE=..\common\ctime.h
# End Source File
# Begin Source File

SOURCE=..\common\CVarDefMap.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CVarDefMap.h
# End Source File
# Begin Source File

SOURCE=..\common\CWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\common\graycom.cpp
# End Source File
# Begin Source File

SOURCE=..\common\graycom.h
# End Source File
# Begin Source File

SOURCE=..\common\graymul.h
# End Source File
# Begin Source File

SOURCE=..\common\grayproto.h
# End Source File
# Begin Source File

SOURCE=..\common\grayver.h
# End Source File
# Begin Source File

SOURCE=..\common\os_unix.h
# End Source File
# Begin Source File

SOURCE=..\common\os_windows.h
# End Source File
# End Group
# Begin Group "Template tables"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\tables\CBaseBaseDef_props.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CChar_functions.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CChar_props.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CCharBase_props.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CCharNpc_props.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CCharPlayer_props.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CClient_functions.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CClient_props.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CFile_functions.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CFile_props.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CFileObjContainer_functions.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CFileObjContainer_props.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\classnames.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CObjBase_functions.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CObjBase_props.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CQuest_functions.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CQuest_props.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CScriptCompiler.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CScriptObj_functions.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\CSector_functions.tbl
# End Source File
# Begin Source File

SOURCE=..\tables\defmessages.tbl
# End Source File
# End Group
# Begin Source File

SOURCE=..\knownbugs.txt
# End Source File
# Begin Source File

SOURCE=.\REVISIONS.txt
# End Source File
# Begin Source File

SOURCE=..\sphere.dic
# End Source File
# Begin Source File

SOURCE=..\Sphere.ini
# End Source File
# Begin Source File

SOURCE=.\spheresvr.ico
# End Source File
# Begin Source File

SOURCE=..\TODO.TXT
# End Source File
# End Target
# End Project
