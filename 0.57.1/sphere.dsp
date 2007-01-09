# Microsoft Developer Studio Project File - Name="Sphere" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=SPHERE - WIN32 DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sphere.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sphere.mak" CFG="SPHERE - WIN32 DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Sphere - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Sphere - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "Sphere - Win32 Testing" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Sphere - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "src/Release"
# PROP BASE Intermediate_Dir "src/Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "src/Release"
# PROP Intermediate_Dir "src/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /MD /W2 /GR /GX /O2 /Ob2 /I "../common/mysql/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "GRAY_SVR" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib /nologo /version:0.10 /subsystem:windows /map /debug /machine:I386 /out:"src/Release/sphereSvr.exe" /libpath:"../common/mysql/lib/debug"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Sphere - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "src/Debug"
# PROP Intermediate_Dir "src/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gz /MDd /W2 /Gm /Gi /GR /GX /ZI /Od /I "../common/mysql/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "GRAY_SVR" /D "DEBUGPACKETS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib /nologo /version:0.12 /subsystem:windows /map /debug /machine:I386 /out:"src\Debug\sphereSvr.exe" /libpath:"../common/mysql/lib/release" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /incremental:no

!ELSEIF  "$(CFG)" == "Sphere - Win32 Testing"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Sphere___Win32_Testing"
# PROP BASE Intermediate_Dir "Sphere___Win32_Testing"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "src/Testing"
# PROP Intermediate_Dir "src/Testing"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /Gr /MD /W2 /GR /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "GRAY_SVR" /FD /c
# SUBTRACT BASE CPP /Fr /YX
# ADD CPP /nologo /G6 /Gr /MD /W2 /GR /GX /Zi /O2 /Ob2 /I "../common/mysql/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "GRAY_SVR" /Fr /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib /nologo /version:0.10 /subsystem:windows /map /debug /machine:I386 /out:"Release/sphereSvr.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib /nologo /version:0.10 /subsystem:windows /map /debug /machine:I386 /out:"src\Release\sphereSvr.exe" /libpath:"../common/mysql/lib/release"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Sphere - Win32 Release"
# Name "Sphere - Win32 Debug"
# Name "Sphere - Win32 Testing"
# Begin Group "Sources"

# PROP Default_Filter "cpp,c,h,rc"
# Begin Source File

SOURCE=.\src\abilities.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CAccount.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CBase.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CChar.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CCharact.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CCharBase.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CCharFight.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CCharNPC.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CCharNPCAct.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CCharNPCPet.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CCharNPCStatus.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CCharSkill.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CCharSpell.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CCharStatus.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CCharUse.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CChat.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CClient.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CClientDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CClientEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CClientGMPage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CClientLog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CClientMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CClientTarg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CClientUse.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CContain.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CGMPage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CItem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CItemBase.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CItemMulti.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CItemSp.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CItemStone.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CItemVend.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CObjBase.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CPathFinder.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CQuest.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CResource.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CResourceCalc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CResourceDef.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CSector.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CServer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CServRef.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CWebPage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CWorld.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CWorldImport.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CWorldMap.cpp
# End Source File
# Begin Source File

SOURCE=.\src\graysvr.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ntwindow.cpp
# End Source File
# Begin Source File

SOURCE=.\src\quest.cpp
# End Source File
# Begin Source File

SOURCE=.\src\SimpleCommand.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\src\abilities.h
# End Source File
# Begin Source File

SOURCE=.\src\CAccount.h
# End Source File
# Begin Source File

SOURCE=.\src\CBase.h
# End Source File
# Begin Source File

SOURCE=.\src\CClient.h
# End Source File
# Begin Source File

SOURCE=.\src\CObjBase.h
# End Source File
# Begin Source File

SOURCE=.\src\CPathFinder.h
# End Source File
# Begin Source File

SOURCE=.\src\CResource.h
# End Source File
# Begin Source File

SOURCE=.\src\CSector.h
# End Source File
# Begin Source File

SOURCE=.\src\CServRef.h
# End Source File
# Begin Source File

SOURCE=.\src\CWorld.h
# End Source File
# Begin Source File

SOURCE=.\src\graysvr.h
# End Source File
# Begin Source File

SOURCE=.\src\quest.h
# End Source File
# Begin Source File

SOURCE=.\src\SimpleCommand.h
# End Source File
# End Group
# Begin Group "network"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\network\network.cpp
# End Source File
# Begin Source File

SOURCE=.\src\network\network.h
# End Source File
# Begin Source File

SOURCE=.\src\network\packet.cpp
# End Source File
# Begin Source File

SOURCE=.\src\network\packet.h
# End Source File
# Begin Source File

SOURCE=.\src\network\receive.cpp
# End Source File
# Begin Source File

SOURCE=.\src\network\receive.h
# End Source File
# Begin Source File

SOURCE=.\src\network\send.cpp
# End Source File
# Begin Source File

SOURCE=.\src\network\send.h
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Group "mtrand"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\common\mtrand\mtrand.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\mtrand\mtrand.h
# End Source File
# End Group
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\common\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\compress.c
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\crc32.h
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\gzio.c
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\infback.c
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\inflate.h
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\zutil.c
# End Source File
# Begin Source File

SOURCE=.\src\common\zlib\zutil.h
# End Source File
# End Group
# Begin Group "twofish"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\common\twofish\aes.h
# End Source File
# Begin Source File

SOURCE=.\src\common\twofish\debug.h
# End Source File
# Begin Source File

SOURCE=.\src\common\twofish\platform.h
# End Source File
# Begin Source File

SOURCE=.\src\common\twofish\table.h
# End Source File
# Begin Source File

SOURCE=.\src\common\twofish\twofish2.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\common\CArray.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CArray.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CAssoc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CAssoc.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CAtom.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CAtom.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CDataBase.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CDataBase.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CEncrypt.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CEncrypt.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CExpression.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CExpression.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CFile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CFile.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CFileList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CFileList.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CGrayData.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CGrayInst.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CGrayInst.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CGrayMap.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CGrayMap.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CMD5.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CMD5.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CMemBlock.h
# End Source File
# Begin Source File

SOURCE=.\src\common\common.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\common.h
# End Source File
# Begin Source File

SOURCE=.\src\common\config.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\config.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CRect.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CRect.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CRegion.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CRegion.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CResourceBase.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CResourceBase.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CScript.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CScript.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CScriptCompiler.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CScriptCompiler.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CScriptObj.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CScriptObj.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CSocket.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CString.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CString.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CTime.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CTime.h
# End Source File
# Begin Source File

SOURCE=.\src\common\CWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\CWindow.h
# End Source File
# Begin Source File

SOURCE=.\src\common\exceptions.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\exceptions.h
# End Source File
# Begin Source File

SOURCE=.\src\common\graycom.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\graycom.h
# End Source File
# Begin Source File

SOURCE=.\src\common\graymul.h
# End Source File
# Begin Source File

SOURCE=.\src\common\grayproto.h
# End Source File
# Begin Source File

SOURCE=.\src\common\os_unix.h
# End Source File
# Begin Source File

SOURCE=.\src\common\os_windows.h
# End Source File
# Begin Source File

SOURCE=.\src\common\threads.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\threads.h
# End Source File
# Begin Source File

SOURCE=.\src\common\VariableList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\common\VariableList.h
# End Source File
# Begin Source File

SOURCE=.\src\common\version.h
# End Source File
# End Group
# Begin Group "Template tables"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\tables\CBaseBaseDef_props.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CChar_functions.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CChar_props.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CCharBase_props.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CCharPlayer_props.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CClient_functions.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CClient_props.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CFile_functions.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CFile_props.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\classnames.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CObjBase_functions.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CObjBase_props.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\config.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CQuest_functions.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CQuest_props.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CScriptCompiler.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CScriptObj_functions.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\CSector_functions.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\defmessages.tbl
# End Source File
# Begin Source File

SOURCE=.\src\tables\triggers.tbl
# End Source File
# End Group
# Begin Group "Documents"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\docs\REVISIONS.TXT
# End Source File
# Begin Source File

SOURCE=.\docs\sphere.dic
# End Source File
# Begin Source File

SOURCE=.\docs\Sphere.ini
# End Source File
# Begin Source File

SOURCE=.\docs\sphereCrypt.ini
# End Source File
# Begin Source File

SOURCE=.\docs\TODO.TXT
# End Source File
# End Group
# Begin Group "res"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\res\GraySvr.rc
# End Source File
# Begin Source File

SOURCE=.\src\res\resource.h
# End Source File
# Begin Source File

SOURCE=.\src\res\spheresvr.ico
# End Source File
# End Group
# End Target
# End Project
