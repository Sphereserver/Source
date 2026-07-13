# GLOBAL SETTINGS

MAKEFLAGS	:= -j$(shell nproc)
TARGET		:= spheresvr

CXX		:= g++ -std=c++20
CC		:= gcc


# CODE GENERATION FLAGS

OVERALL_FLAGS		:= -pipe
OPTIMIZATION_FLAGS	:= -O2 -flto=auto -fno-omit-frame-pointer -fno-strict-aliasing
DEBUG_FLAGS		:=
CODE_GEN_FLAGS		:= -fexceptions -fnon-call-exceptions
ARCH_FLAGS		:= -m64 -Dx64

ifdef DEBUG
	CPPFLAGS	:= -D_DEBUG
	DEBUG_FLAGS	+= -ggdb3
else
	CPPFLAGS	:= -D_THREAD_TRACK_CALLSTACK
endif

ifdef NIGHTLY
	CPPFLAGS	+= -D_NIGHTLYBUILD
endif


# C/C++ FLAGS

WARN_CXXFLAGS	:= -Wall -Wno-register
CXXFLAGS	:= $(OVERALL_FLAGS) $(OPTIMIZATION_FLAGS) $(DEBUG_FLAGS) $(CODE_GEN_FLAGS) $(ARCH_FLAGS) $(CPPFLAGS) $(WARN_CXXFLAGS)

WARN_CFLAGS	:= -Wall -Wno-implicit-function-declaration -Wno-misleading-indentation -Wno-unused-result
CFLAGS		:= $(OVERALL_FLAGS) $(OPTIMIZATION_FLAGS) $(DEBUG_FLAGS) $(CODE_GEN_FLAGS) $(ARCH_FLAGS) $(CPPFLAGS) $(WARN_CFLAGS)


# LINKER FLAGS

WARN_LDFLAGS	:= -Wno-aggressive-loop-optimizations -Wno-lto-type-mismatch -Wno-return-local-addr
LDFLAGS		:= -L/usr/lib/mysql -pthread $(OPTIMIZATION_FLAGS) $(CODE_GEN_FLAGS) $(ARCH_FLAGS) $(WARN_LDFLAGS)

ifndef DEBUG
	LDFLAGS	+= -s
endif

LDLIBS		:= -lmysqlclient -lpthread -ldl


# SOURCE FILES

SRC :=	./src/common/CArray.cpp \
	./src/common/CAssoc.cpp \
	./src/common/CCacheableScriptFile.cpp \
	./src/common/CCSVFile.cpp \
	./src/common/CDataBase.cpp \
	./src/common/CEncrypt.cpp \
	./src/common/CException.cpp \
	./src/common/CExpression.cpp \
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
	./src/common/CString.cpp \
	./src/common/CTime.cpp \
	./src/common/CVarDefMap.cpp \
	./src/common/CVarFloat.cpp \
	./src/common/graycom.cpp \
	./src/common/libev/wrapper_ev.c \
	./src/common/sqlite/SQLite.cpp \
	./src/common/sqlite/sqlite3.c \
	./src/common/twofish/twofish2.cpp \
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
	./src/graysvr/CAccount.cpp \
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
	./src/network/network.cpp \
	./src/network/packet.cpp \
	./src/network/receive.cpp \
	./src/network/send.cpp \
	./src/sphere/asyncdb.cpp \
	./src/sphere/linuxev.cpp \
	./src/sphere/mutex.cpp \
	./src/sphere/ProfileData.cpp \
	./src/sphere/strings.cpp \
	./src/sphere/threads.cpp


# OBJECT FILES

OBJS := $(patsubst %.cpp,%.o,$(filter %.cpp,$(SRC))) \
	$(patsubst %.c,%.o,$(filter %.c,$(SRC)))


# BUILD RULES

.PHONY:	all clean flags version

all: $(TARGET)

clean:
	@rm -f $(TARGET)
	@find . -name "*.o" -type f -delete

flags:
	@echo 'C++ compiler:	$(CXX)'
	@echo 'C++ flags:	$(CXXFLAGS)'
	@echo ''
	@echo 'C compiler:	$(CC)'
	@echo 'C flags:	$(CFLAGS)'
	@echo ''
	@echo 'Linker flags:	$(LDFLAGS)'
	@echo 'Linker libs:	$(LDLIBS)'
	@echo ''

version: flags
	@VERSIONING_FILE="./src/common/version.h"; \
	echo "// This file is auto-generated by compiler pre-build event. Do not edit." > "$${VERSIONING_FILE}"; \
	if [ "$$(git rev-parse --is-inside-work-tree 2>/dev/null)" ]; then \
		GIT_COMMIT_COUNT=$$(( $$(git rev-list --count HEAD) - 2406 )); \
		GIT_COMMIT_HASH=$$(git rev-parse --short HEAD); \
		echo "#define GIT_COMMIT_COUNT $${GIT_COMMIT_COUNT}" >> "$${VERSIONING_FILE}"; \
		echo "#define GIT_COMMIT_HASH \"$${GIT_COMMIT_HASH}\"" >> "$${VERSIONING_FILE}"; \
		echo "Current build number: $${GIT_COMMIT_COUNT} (Git hash: $${GIT_COMMIT_HASH})"; \
	else \
		echo "// Versioning disabled (not a git repository)" >> "$${VERSIONING_FILE}"; \
	fi

$(OBJS): version

$(TARGET): $(OBJS)
	@$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	@echo '  $(lastword $(MAKEFILE_LIST)) -> $(shell readlink -f $@)'

%.o: %.cpp
	@echo '  $(notdir $<)'
	@$(CXX) -c $(CXXFLAGS) $< -o $@

%.o: %.c
	@echo '  $(notdir $<)'
	@$(CC) -c $(CFLAGS) $< -o $@
