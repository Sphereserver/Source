SHELL   = /bin/bash

# Generic makefile
MARCH = -march=i686 -m32
OPTDEFAULT = -fno-omit-frame-pointer -ffast-math -fpermissive $(MARCH)
OPT 	= -O0 -fno-expensive-optimizations $(OPTDEFAULT)
WARN	= -Wall -Wextra -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-switch
#WARN	= -w
DEBUG	= -s

# DB includes + libs
DBINCL	= -I/usr/include/mysql
DBLIBS	= -lmysqlclient
#DBDEFS	= -D_DBPLUGIN

# Linux
INCLUDE	= -I./src/common $(DBINCL)
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

GITHASHCMD = "$(shell git log -1 HEAD --format=%h)"
GITREVISIONCMD = $(shell expr $(shell git rev-list --count HEAD) + 410)

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
		./src/graysvr/CItemSp.cpp \
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
		./src/common/CAtom.cpp \
		./src/common/CAssoc.cpp \
		./src/common/CDataBase.cpp \
		./src/common/CDatabaseLoader.cpp \
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

O_FLAGS	= $(WARN) $(PIPE) $(SPECIAL)
C_FLAGS	= $(OPT) $(INCLUDE) $(DEFINES)


.PHONY:	all clean tidy debug-nightly

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
	@echo '#define __GITHASH__ ${GITHASHCMD}' > ./src/common/version/GitRevision.h
	@echo '#define __GITREVISION__ ${GITREVISIONCMD}' >> ./src/common/version/GitRevision.h
	@echo 'Git Hash is ${GITHASHCMD}. Revision No. is ${GITREVISIONCMD}'

gray:	$(SRC:.cpp=.o) $(SRC:.c=.co)

flags:  
	@echo "Compiler Flags: $(CC) -c $(O_FLAGS) $(C_FLAGS)"

debug: ; $(eval DEFINES := $(DEFINES) $(DEBUGDEFS) $(NIGHTLYDEFS))

nightly: debug $(EXE)	

$(EXE): git flags gray
	@$(CC) $(O_FLAGS) $(C_FLAGS) -o $(EXE) ./src/graysvr/*.o ./src/common/*.o ./src/common/twofish/*.o ./src/common/libev/*.o ./src/common/zlib/*.o ./src/common/sqlite/*.o ./src/sphere/*.o ./src/network/*.o $(LIBS)

%.o:	%.cpp
	@echo " Compiling $<"
	@$(CC) -c $(O_FLAGS) $(C_FLAGS) $< -o $@

%.co:	%.c
	@echo " Compiling $<"
	@$(CCO) -c $(O_FLAGS) $(C_FLAGS) $< -o $(@:.co=.o)
