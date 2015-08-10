SHELL   = /bin/bash

# Generic makefile
MARCH = -march=i686 -m32
OPTDEFAULT = -fno-omit-frame-pointer -ffast-math -fpermissive $(MARCH)
OPT 	= -O0 -fno-expensive-optimizations $(OPTDEFAULT)
WARN	= -Wall -Wextra -Wno-unknown-pragmas -Wno-invalid-offsetof
#WARN	= -w
DEBUG	= -s

# DB includes + libs
DBINCL	= -I/usr/include/mysql
DBLIBS	= -lmysqlclient
#DBDEFS	= -D_DBPLUGIN

# Linux
INCLUDE	= -I../common $(DBINCL)
LIBS	= -dynamic -L/usr/lib -lpthread $(DBLIBS) -lrt -ldl
DEFNIX  = -D_LINUX

# Nightly and Debug flags are activated by dedicated rules later on
NIGHTLYDEFS = -D_NIGHTLYBUILD -DTHREAD_TRACK_CALLSTACK
DEBUGDEFS = -D_DEBUG -D_PACKETDUMP -D_TESTEXCEPTION -DDEBUG_CRYPT_MSGS

EXTRADEFS = -D_MTNETWORK
SVNDEFS = -D_SUBVERSION
DEFINES = -DGRAY_SVR -D_CONSOLE -D_REENTRANT $(DEFNIX) $(EXTRADEFS) $(SVNDEFS) $(DBDEFS)

EXE	= spheresvr

CC	= g++
CCO	= gcc

NO	= -fno-rtti -fno-exceptions
EX	= -fexceptions -fnon-call-exceptions
STRICT  = # -mstrict-align
SPECIAL = $(EX) $(STRICT) $(DEBUG)

PROF	= -pg
PIPE	= -pipe

GITHASHCMD = "$(shell git log -1 HEAD --format=%H)"
GITREVISIONCMD = $(shell echo $(shell git rev-list --count HEAD) + 410 | bc)

SRC	:= 	CAccount.cpp \
		CBase.cpp \
		CChar.cpp \
		CCharact.cpp \
		CCharBase.cpp \
		CCharFight.cpp \
		CCharNPC.cpp \
		CCharNPCAct.cpp \
		CCharNPCPet.cpp \
		CCharNPCStatus.cpp \
		CCharSkill.cpp \
		CCharSpell.cpp \
		CCharStatus.cpp \
		CCharUse.cpp \
		CChat.cpp \
		CClient.cpp \
		CClientDialog.cpp \
		CClientEvent.cpp \
		CClientGMPage.cpp \
		CClientLog.cpp \
		CClientMsg.cpp \
		CClientTarg.cpp \
		CClientUse.cpp \
		CContain.cpp \
		CGMPage.cpp \
		CItem.cpp \
		CItemBase.cpp \
		CItemMulti.cpp \
		CItemMultiCustom.cpp \
		CItemShip.cpp \
		CItemSp.cpp \
		CItemStone.cpp \
		CItemVend.cpp \
		CLog.cpp \
		CObjBase.cpp \
		CPathFinder.cpp \
		CResource.cpp \
		CResourceCalc.cpp \
		CResourceDef.cpp \
		CSector.cpp \
		CServer.cpp \
		CServRef.cpp \
		CQuest.cpp \
		CWebPage.cpp \
		CWorld.cpp \
		CWorldImport.cpp \
		CWorldMap.cpp \
		graysvr.cpp \
		PingServer.cpp \
		UnixTerminal.cpp \
		../common/twofish/twofish2.cpp \
		../common/libev/wrapper_ev.c \
		../common/zlib/adler32.c \
		../common/zlib/compress.c \
		../common/zlib/crc32.c \
		../common/zlib/deflate.c \
		../common/zlib/gzclose.c \
		../common/zlib/gzlib.c \
		../common/zlib/gzread.c \
		../common/zlib/gzwrite.c \
		../common/zlib/infback.c \
		../common/zlib/inffast.c \
		../common/zlib/inflate.c \
		../common/zlib/inftrees.c \
		../common/zlib/trees.c \
		../common/zlib/uncompr.c \
		../common/zlib/zutil.c \
		../common/CArray.cpp \
		../common/CAtom.cpp \
		../common/CAssoc.cpp \
		../common/CDataBase.cpp \
		../common/CDatabaseLoader.cpp \
		../common/CEncrypt.cpp \
		../common/CExpression.cpp \
		../common/CException.cpp \
		../common/CacheableScriptFile.cpp \
		../common/CFile.cpp \
		../common/CFileList.cpp \
		../common/CGrayData.cpp \
		../common/CGrayInst.cpp \
		../common/CGrayMap.cpp \
		../common/CMD5.cpp \
		../common/CQueue.cpp \
		../common/CRect.cpp \
		../common/CRegion.cpp \
		../common/CResourceBase.cpp \
		../common/CScript.cpp \
		../common/CScriptObj.cpp \
		../common/CSectorTemplate.cpp \
		../common/CSocket.cpp \
		../common/CsvFile.cpp \
		../common/CTime.cpp \
		../common/CString.cpp \
		../common/CVarDefMap.cpp \
		../common/CVarFloat.cpp \
		../common/ListDefContMap.cpp \
		../common/graycom.cpp \
        ../common/sqlite/SQLite.cpp \
        ../common/sqlite/sqlite3.c \
		../sphere/mutex.cpp \
		../sphere/strings.cpp \
		../sphere/threads.cpp \
		../sphere/linuxev.cpp \
		../sphere/asyncdb.cpp \
		../sphere/ProfileData.cpp \
		../network/network.cpp \
		../network/packet.cpp \
		../network/send.cpp \
		../network/receive.cpp

O_FLAGS	= $(WARN) $(PIPE) $(SPECIAL)
C_FLAGS	= $(OPT) $(INCLUDE) $(DEFINES)


.PHONY:	all clean tidy debug-nightly

all:	$(EXE)

clean:	tidy
	rm -f *.o ../common/*.o ../common/mtrand/*.o ../common/twofish/*.o ../common/libev/*.o ../common/zlib/*.o ../sphere/*.o ../network/*.o ../tests/*.o $(EXE)

tidy:
	rm -f *~ *orig *bak *rej ../common/*~ ../common/*orig ../common*bak \
				 ../common/mtrand/*~ ../common/mtrand/*orig ../common/mtrand/*bak \
				 ../common/twofish/*~ ../common/twofish/*orig ../common/twofish/*bak \
				 ../common/libev/*~ ../common/libev/*orig ../common/libev/*bak \
				 ../common/zlib/*~ ../common/zlib/*orig ../common/zlib/*bak \
				 ../sphere/*~ ../sphere/*orig ../sphere/*bak \
				 ../network/*~ ../network/*orig ../network/*bak \
				 ../tests/*~ ../tests/*orig ../tests/*bak

tags:	$(SRC)
	ctags $(SRC)

git:
	@echo '#define __GITHASH__ ${GITHASHCMD}' > ../common/version/GitRevision.h
	@echo '#define __GITREVISION__ ${GITREVISIONCMD}' >> ../common/version/GitRevision.h

gray:	$(SRC:.cpp=.o) $(SRC:.c=.co)

flags:  
	@echo "Compiler Flags: $(CC) -c $(O_FLAGS) $(C_FLAGS)"

debug: ; $(eval DEFINES := $(DEFINES) $(DEBUGDEFS) $(NIGHTLYDEFS))

nightly: debug $(EXE)	

$(EXE): git flags gray
	@$(CC) $(O_FLAGS) $(C_FLAGS) $(LIBS) -o $(EXE) *.o ../common/*.o ../common/twofish/*.o ../common/libev/*.o ../common/zlib/*.o  ../common/sqlite/*.o ../sphere/*.o ../network/*.o $(LIBS)

%.o:	%.cpp
	@echo " Compiling $<"
	@$(CC) -c $(O_FLAGS) $(C_FLAGS) $< -o $@

%.co:	%.c
	@echo " Compiling $<"
	@$(CCO) -c $(O_FLAGS) $(C_FLAGS) $< -o $(@:.co=.o)
