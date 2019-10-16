MAKEFLAGS		= -j$(shell nproc)
ARCH_FLAGS		= -march=i686 -m32
CODE_GEN_FLAGS		= -fexceptions -fnon-call-exceptions
GENERAL_FLAGS		= -pipe
LINKER_FLAGS		= -s
OPTIMIZATION_FLAGS	= -Os -ffast-math -fno-strict-aliasing -fno-omit-frame-pointer
O_WARNING_FLAGS		= -Wall -Wno-maybe-uninitialized -Wno-switch -Wno-unknown-pragmas -Wno-unused-result -Wno-invalid-offsetof
CO_WARNING_FLAGS	= -Wall -Wno-maybe-uninitialized -Wno-switch -Wno-unknown-pragmas -Wno-unused-result -Wno-implicit-function-declaration

ifdef NIGHTLY
	NIGHTLY_DEFS	= -D_NIGHTLYBUILD #-DTHREAD_TRACK_CALLSTACK
endif
ifdef DBG
	DEBUG_DEFS	= -D_DEBUG #-DTHREAD_TRACK_CALLSTACK -D_TESTEXCEPTION -DDEBUG_CRYPT_MSGS
	DEBUG_FLAGS	= -ggdb3
endif
DEFINES	= $(NIGHTLY_DEFS) $(DEBUG_DEFS)

ifneq ($(shell git rev-parse --git-dir),)
	GITREVISION	= $(shell expr $(shell git rev-list --count HEAD) - 2406)
	GITHASH		= $(shell git rev-parse --short HEAD)
endif

INCLUDE_DIRS	= -L/usr/lib/mysql
INCLUDE_LIBS	= -lmysqlclient -lpthread -ldl

O_FLAGS		= $(CODE_GEN_FLAGS) $(DEBUG_FLAGS) $(GENERAL_FLAGS) $(LINKER_FLAGS) $(O_WARNING_FLAGS)
CO_FLAGS	= $(CODE_GEN_FLAGS) $(DEBUG_FLAGS) $(GENERAL_FLAGS) $(LINKER_FLAGS) $(CO_WARNING_FLAGS)
C_FLAGS		= $(ARCH_FLAGS) $(DEFINES) $(INCLUDE_DIRS) $(OPTIMIZATION_FLAGS)
CC_FLAGS	= $(ARCH_FLAGS) $(DEFINES) $(INCLUDE_DIRS) $(OPTIMIZATION_FLAGS)

CC	= g++ -std=c++11
CCO	= gcc
EXE	= spheresvr

SRC :=	./src/graysvr/CAccount.cpp \
	./src/graysvr/CBase.cpp \
	./src/graysvr/CChar.cpp \
	./src/graysvr/CCharAct.cpp \
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
	./src/graysvr/CPingServer.cpp \
	./src/graysvr/CResource.cpp \
	./src/graysvr/CResourceCalc.cpp \
	./src/graysvr/CResourceDef.cpp \
	./src/graysvr/CSector.cpp \
	./src/graysvr/CServer.cpp \
	./src/graysvr/CServRef.cpp \
	./src/graysvr/CUnixTerminal.cpp \
	./src/graysvr/CWebPage.cpp \
	./src/graysvr/CWorld.cpp \
	./src/graysvr/CWorldImport.cpp \
	./src/graysvr/CWorldMap.cpp \
	./src/graysvr/graysvr.cpp \
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
	./src/common/CCacheableScriptFile.cpp \
	./src/common/CCSVFile.cpp \
	./src/common/CDataBase.cpp \
	./src/common/CEncrypt.cpp \
	./src/common/CExpression.cpp \
	./src/common/CException.cpp \
	./src/common/CFile.cpp \
	./src/common/CFileList.cpp \
	./src/common/CGrayData.cpp \
	./src/common/CGrayInst.cpp \
	./src/common/CGrayMap.cpp \
	./src/common/CListDefMap.cpp \
	./src/common/CMD5.cpp \
	./src/common/CQueue.cpp \
	./src/common/CRect.cpp \
	./src/common/CRegion.cpp \
	./src/common/CResourceBase.cpp \
	./src/common/CScript.cpp \
	./src/common/CScriptObj.cpp \
	./src/common/CSectorTemplate.cpp \
	./src/common/CSocket.cpp \
	./src/common/CTime.cpp \
	./src/common/CString.cpp \
	./src/common/CVarDefMap.cpp \
	./src/common/CVarFloat.cpp \
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


.PHONY:	build clean

build:	git flags gray
	@$(CC) $(O_FLAGS) $(C_FLAGS) -o $(EXE) ./src/common/*.o ./src/common/libev/*.o ./src/common/sqlite/*.o ./src/common/twofish/*.o ./src/common/zlib/*.o ./src/graysvr/*.o ./src/network/*.o ./src/sphere/*.o $(INCLUDE_LIBS)

clean:
	rm -rf ./$(EXE)
	find . -name \*.o -type f -delete

git:
ifdef GITREVISION
	@echo 'Current build revision: $(GITREVISION) (Git hash: $(GITHASH))'
	@echo '#define __GITREVISION__ $(GITREVISION)' > ./src/common/version/GitRevision.h
	@echo '#define __GITHASH__ "$(GITHASH)"' >> ./src/common/version/GitRevision.h
endif

flags:
	@echo 'Compiler flags: $(CC) $(O_FLAGS) $(C_FLAGS)'

gray:	$(SRC:.cpp=.o) $(SRC:.c=.co)

%.o:	%.cpp
	@echo '  Compiling $<'
	@$(CC) -c $(O_FLAGS) $(C_FLAGS) $< -o $@

%.co:	%.c
	@echo '  Compiling $<'
	@$(CCO) -c $(CO_FLAGS) $(CC_FLAGS) $< -o $(@:.co=.o)
