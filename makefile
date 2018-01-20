SHELL	= /bin/bash

# Generic makefile
MARCH	= -march=i686 -m32

OPTDEFAULT	= -fno-strict-aliasing -fno-omit-frame-pointer -ffast-math -fpermissive $(MARCH)
COPTDEFAULT	= -fno-strict-aliasing -fno-omit-frame-pointer -ffast-math $(MARCH)
OPT 		= -Os $(OPTDEFAULT)
COPT		= -Os $(COPTDEFAULT)
WARN		= -Wall -Wno-unknown-pragmas -Wno-unused-but-set-variable -Wno-unused-result -Wno-maybe-uninitialized -Wno-switch -Wno-invalid-offsetof
CWARN		= -Wall -Wno-unknown-pragmas -Wno-unused-but-set-variable -Wno-unused-result -Wno-maybe-uninitialized -Wno-switch -Wno-implicit-function-declaration

# DB includes + libs
DBINCLUDE	= -L/usr/lib/mysql
DBLIBS		= -lmysqlclient

# Linux
INCLUDE		= $(DBINCLUDE)
LIBS		= -dynamic -lpthread -lrt -ldl $(DBLIBS)

ifdef NIGHTLY
	NIGHTLYDEFS = -D_NIGHTLYBUILD		# -DTHREAD_TRACK_CALLSTACK
endif

ifdef DBG
	DBGDEFS = -D_DEBUG -DTHREAD_TRACK_CALLSTACK		# -D_TESTEXCEPTION -DDEBUG_CRYPT_MSGS
	DBGWARN = -ggdb3
endif

DEFINES	= -D_MTNETWORK $(NIGHTLYDEFS) $(DBGDEFS)

EXE	= spheresvr

CC	= g++
CCO	= gcc

NO	= -fno-rtti -fno-exceptions
EX	= -fexceptions -fnon-call-exceptions
SPECIAL	= -s $(EX) $(DBGWARN)

GITREVISION	= $(shell expr $(shell git rev-list --count HEAD) - 2406)
GITHASH		= $(shell git rev-parse --short HEAD)

PROF	= -pg
PIPE	= -pipe

SRC	:= 	./src/graysvr/CAccount.cpp \
		./src/graysvr/CBase.cpp \
		./src/graysvr/CChar.cpp \
		./src/graysvr/CCharact.cpp \
		./src/graysvr/CCharBase.cpp \
		./src/graysvr/CCharFight.cpp \
		./src/graysvr/CCharNPC.cpp \
		./src/graysvr/CCharNPCAct.cpp \
		./src/graysvr/CCharNPCPet.cpp \
		./src/graysvr/CCharNPCStatus.cpp \
		./src/graysvr/CCharSkill.cpp \
		./src/graysvr/CCharSpell.cpp \
		./src/graysvr/CCharStatus.cpp \
		./src/graysvr/CCharUse.cpp \
		./src/graysvr/CChat.cpp \
		./src/graysvr/CClient.cpp \
		./src/graysvr/CClientDialog.cpp \
		./src/graysvr/CClientEvent.cpp \
		./src/graysvr/CClientGMPage.cpp \
		./src/graysvr/CClientLog.cpp \
		./src/graysvr/CClientMsg.cpp \
		./src/graysvr/CClientTarg.cpp \
		./src/graysvr/CClientUse.cpp \
		./src/graysvr/CContain.cpp \
		./src/graysvr/CGMPage.cpp \
		./src/graysvr/CItem.cpp \
		./src/graysvr/CItemBase.cpp \
		./src/graysvr/CItemMulti.cpp \
		./src/graysvr/CItemMultiCustom.cpp \
		./src/graysvr/CItemShip.cpp \
		./src/graysvr/CItemSpawn.cpp \
		./src/graysvr/CItemStone.cpp \
		./src/graysvr/CItemVend.cpp \
		./src/graysvr/CLog.cpp \
		./src/graysvr/CObjBase.cpp \
		./src/graysvr/CParty.cpp \
		./src/graysvr/CPathFinder.cpp \
		./src/graysvr/CResource.cpp \
		./src/graysvr/CResourceCalc.cpp \
		./src/graysvr/CResourceDef.cpp \
		./src/graysvr/CSector.cpp \
		./src/graysvr/CServer.cpp \
		./src/graysvr/CServRef.cpp \
		./src/graysvr/CWebPage.cpp \
		./src/graysvr/CWorld.cpp \
		./src/graysvr/CWorldImport.cpp \
		./src/graysvr/CWorldMap.cpp \
		./src/graysvr/graysvr.cpp \
		./src/graysvr/PingServer.cpp \
		./src/graysvr/UnixTerminal.cpp \
		./src/common/twofish/twofish2.cpp \
		./src/common/libev/wrapper_ev.c \
		./src/common/zlib/adler32.c \
		./src/common/zlib/compress.c \
		./src/common/zlib/crc32.c \
		./src/common/zlib/deflate.c \
		./src/common/zlib/gzclose.c \
		./src/common/zlib/gzlib.c \
		./src/common/zlib/gzread.c \
		./src/common/zlib/gzwrite.c \
		./src/common/zlib/infback.c \
		./src/common/zlib/inffast.c \
		./src/common/zlib/inflate.c \
		./src/common/zlib/inftrees.c \
		./src/common/zlib/trees.c \
		./src/common/zlib/uncompr.c \
		./src/common/zlib/zutil.c \
		./src/common/CArray.cpp \
		./src/common/CAssoc.cpp \
		./src/common/CDataBase.cpp \
		./src/common/CEncrypt.cpp \
		./src/common/CExpression.cpp \
		./src/common/CException.cpp \
		./src/common/CacheableScriptFile.cpp \
		./src/common/CFile.cpp \
		./src/common/CFileList.cpp \
		./src/common/CGrayData.cpp \
		./src/common/CGrayInst.cpp \
		./src/common/CGrayMap.cpp \
		./src/common/CMD5.cpp \
		./src/common/CQueue.cpp \
		./src/common/CRect.cpp \
		./src/common/CRegion.cpp \
		./src/common/CResourceBase.cpp \
		./src/common/CScript.cpp \
		./src/common/CScriptObj.cpp \
		./src/common/CSectorTemplate.cpp \
		./src/common/CSocket.cpp \
		./src/common/CsvFile.cpp \
		./src/common/CTime.cpp \
		./src/common/CString.cpp \
		./src/common/CVarDefMap.cpp \
		./src/common/CVarFloat.cpp \
		./src/common/ListDefContMap.cpp \
		./src/common/graycom.cpp \
		./src/common/sqlite/SQLite.cpp \
		./src/common/sqlite/sqlite3.c \
		./src/sphere/mutex.cpp \
		./src/sphere/strings.cpp \
		./src/sphere/threads.cpp \
		./src/sphere/linuxev.cpp \
		./src/sphere/asyncdb.cpp \
		./src/sphere/ProfileData.cpp \
		./src/network/network.cpp \
		./src/network/packet.cpp \
		./src/network/send.cpp \
		./src/network/receive.cpp

O_FLAGS		= $(WARN) $(DBGWARN) $(PIPE) $(SPECIAL)
CO_FLAGS	= $(CWARN) $(DBGWARN) $(PIPE) $(SPECIAL)
C_FLAGS		= $(OPT) $(INCLUDE) $(DEFINES)
CC_FLAGS	= $(COPT) $(INCLUDE) $(DEFINES)


.PHONY:	all clean tidy

all:	$(EXE)

clean:	tidy
	rm -f ./src/graysvr/*.o ./src/common/*.o ./src/common/mtrand/*.o ./src/common/twofish/*.o ./src/common/libev/*.o ./src/common/zlib/*.o ./src/common/sqlite/*.o ./src/sphere/*.o ./src/network/*.o ./src/tests/*.o $(EXE)

tidy:
	rm -f ./src/graysvr/*~ ./src/graysvr/*orig ./src/graysvr/*bak ./src/graysvr/*rej \
		./src/common/*~ ./src/common/*orig ./src/common*bak \
		./src/common/mtrand/*~ ./src/common/mtrand/*orig ./src/common/mtrand/*bak \
		./src/common/twofish/*~ ./src/common/twofish/*orig ./src/common/twofish/*bak \
		./src/common/libev/*~ ./src/common/libev/*orig ./src/common/libev/*bak \
		./src/common/zlib/*~ ./src/common/zlib/*orig ./src/common/zlib/*bak \
		./src/common/sqlite/*~ ./src/common/sqlite/*orig ./src/common/sqlite/*bak \
		./src/sphere/*~ ./src/sphere/*orig ./src/sphere/*bak \
		./src/network/*~ ./src/network/*orig ./src/network/*bak \
		./src/tests/*~ ./src/tests/*orig ./src/tests/*bak

tags:	$(SRC)
	ctags $(SRC)

git:
ifdef GITREVISION
	@echo 'Current build revision: $(GITREVISION) (GIT hash: $(GITHASH))'
	@echo '#define __GITREVISION__ $(GITREVISION)' > ./src/common/version/GitRevision.h
	@echo '#define __GITHASH__ "$(GITHASH)"' >> ./src/common/version/GitRevision.h
endif

gray:	$(SRC:.cpp=.o) $(SRC:.c=.co)


flags:
	@echo "Compiler Flags: $(CC) -c $(O_FLAGS) $(C_FLAGS)"

$(EXE): git flags gray
	@$(CC) $(O_FLAGS) $(C_FLAGS) -o $(EXE) ./src/graysvr/*.o ./src/common/*.o ./src/common/twofish/*.o ./src/common/libev/*.o ./src/common/zlib/*.o ./src/common/sqlite/*.o ./src/sphere/*.o ./src/network/*.o $(LIBS)

%.o:	%.cpp
	@echo " Compiling $<"
	@$(CC) -c $(O_FLAGS) $(C_FLAGS) $< -o $@

%.co:	%.c
	@echo " Compiling $<"
	@$(CCO) -c $(CO_FLAGS) $(CC_FLAGS) $< -o $(@:.co=.o)
