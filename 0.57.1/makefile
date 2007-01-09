VER	= 0.57.1
PLAT	= Linux

# Generic makefile
OPT 	= -fno-omit-frame-pointer -ffast-math -fno-expensive-optimizations -fpermissive -march=i486
#WARN	= -Wall -W
WARN	= -w

DEBUG	= -ggdb2
#DEBUG	= -s
INCLUDE	= -Isrc/common -Isrc/network -I/usr/include/mysql

# Linux
LIBS	= -L/usr/lib -lpthread -lmysqlclient
# FreeBSD
#LIBS	= -L/usr/lib -pthread
#DEFBSD	= -D_BSD

DEFINES = -DGRAY_SVR -D_CONSOLE -D_REENTRANT $(DEFBSD)

EXE	= graysvr
REXE	= spheresvr

CC	= g++

NO	= -fno-rtti -fno-exceptions
#EX	= -fexceptions -fasynchronous-exceptions -fsjlj-exceptions
EX	= -fexceptions -fnon-call-exceptions
STRICT  = # -mstrict-align
SPECIAL = $(EX) $(STRICT) $(DEBUG)

PROF	= -pg
PIPE	= -pipe

SRC	:=	src/abilities.cpp \
		src/CAccount.cpp \
		src/CBase.cpp \
		src/CChar.cpp \
		src/CCharact.cpp \
		src/CCharBase.cpp \
		src/CCharFight.cpp \
		src/CCharNPC.cpp \
		src/CCharNPCAct.cpp \
		src/CCharNPCPet.cpp \
		src/CCharNPCStatus.cpp \
		src/CCharSkill.cpp \
		src/CCharSpell.cpp \
		src/CCharStatus.cpp \
		src/CCharUse.cpp \
		src/CChat.cpp \
		src/CClient.cpp \
		src/CClientDialog.cpp \
		src/CClientEvent.cpp \
		src/CClientGMPage.cpp \
		src/CClientLog.cpp \
		src/CClientMsg.cpp \
		src/CClientTarg.cpp \
		src/CClientUse.cpp \
		src/CContain.cpp \
		src/CGMPage.cpp \
		src/CItem.cpp \
		src/CItemBase.cpp \
		src/CItemMulti.cpp \
		src/CItemSp.cpp \
		src/CItemStone.cpp \
		src/CItemVend.cpp \
		src/CObjBase.cpp \
		src/CPathFinder.cpp \
		src/CResource.cpp \
		src/CResourceCalc.cpp \
		src/CResourceDef.cpp \
		src/CSector.cpp \
		src/CServer.cpp \
		src/CServRef.cpp \
		src/CQuest.cpp \
		src/CWebPage.cpp \
		src/CWorld.cpp \
		src/CWorldImport.cpp \
		src/CWorldMap.cpp \
		src/graysvr.cpp \
		src/quest.cpp \
		src/common/mtrand/mtrand.cpp \
		src/common/twofish/twofish2.cpp \
		src/common/CArray.cpp \
		src/common/CAtom.cpp \
		src/common/CAssoc.cpp \
		src/common/CDataBase.cpp \
		src/common/CEncrypt.cpp \
		src/common/CExpression.cpp \
		src/common/CFile.cpp \
		src/common/CFileList.cpp \
		src/common/CGrayData.cpp \
		src/common/CGrayInst.cpp \
		src/common/CGrayMap.cpp \
		src/common/CMD5.cpp \
		src/common/common.cpp \
		src/common/config.cpp \
		src/common/CRect.cpp \
		src/common/CRegion.cpp \
		src/common/CResourceBase.cpp \
		src/common/CScript.cpp \
		src/common/CScriptCompiler.cpp \
		src/common/CScriptObj.cpp \
		src/common/CSocket.cpp \
		src/common/CTime.cpp \
		src/common/CString.cpp \
		src/common/exceptions.cpp \
		src/common/threads.cpp \
		src/common/graycom.cpp \
		src/common/VariableList.cpp \
		src/network/network.cpp \
		src/network/packet.cpp \
		src/network/receive.cpp \
		src/network/send.cpp \
                src/common/zlib/adler32.c \
                src/common/zlib/compress.c \
                src/common/zlib/crc32.c \
                src/common/zlib/deflate.c \
                src/common/zlib/gzio.c \
                src/common/zlib/infback.c \
                src/common/zlib/inffast.c \
                src/common/zlib/inflate.c \
                src/common/zlib/inftrees.c \
                src/common/zlib/trees.c \
                src/common/zlib/uncompr.c \
                src/common/zlib/zutil.c

O_FLAGS	= $(WARN) $(PIPE) $(SPECIAL) $(STATIC) 
C_FLAGS	= $(OPT) $(INCLUDE) $(DEFINES)


.PHONY:	all clean tidy

all:	$(EXE)

clean:	#tidy
	rm -f src/*.o src/common/*.o src/common/mtrand/*.o src/common/twofish/*.o src/network/*.o src/common/mysql/*.o src/common/zlib/*.o $(EXE) $(REXE)
	$(MAKE)

.depend:
	gcc -MM $(INCLUDE) $(SRC) $(COMMONSRC) > .depend
	perl -pi -e 's/([^.]+)\.o/o\/\1.o/g' .depend

tidy:
	rm -f src/*~ src/*orig src/*bak src/*rej src/common/*~ src/common/*orig ../common*bak src/common/mtrand/*~ src/common/mtrand/*orig \ 
	src/common/mtrand/*bak src/common/twofish/*~ src/common/twofish/*orig src/common/twofish/*bak

tags:	$(SRC)
	ctags $(SRC)

#cmn:	$(COMMONSRC:.cpp=.o)

gray:	$(SRC:.cpp=.o) $(SRC:.c=.co)

#commonlib:	$(COMMONSRC:.cpp=.o)
#	ar r $@ $?

flags:  
	@echo "Compiler Flags: $(CC) -c $(O_FLAGS) $(C_FLAGS)"

$(EXE): flags gray
	g++ $(O_FLAGS) $(C_FLAGS) $(LIBS) -o $(EXE) src/*.o src/common/*.o src/common/mtrand/*.o src/common/twofish/*.o src/common/zlib/*.o src/network/*.o

version.h:	VERSION
	echo "#define VERSION 'head -l VERSION'" > version.h
	echo "#define COMPILED_BY \"$(COMPILED_BY)\"" >> version.h

%.o:	%.cpp
	@echo " Compiling $<"
	@$(CC) -c $(O_FLAGS) $(C_FLAGS) $< -o $@

%.co:	%.c
	@echo " Compiling $<"
	@gcc -c $(O_FLAGS) $(C_FLAGS) $< -o $(@:.co=.o)

dist:
	@cp $(EXE) $(REXE)
	@cp REVISIONS.TXT Revisions$(VER).txt
	@cp MANUAL.TXT Manual$(VER).txt
	@tar cvfz SphereServer$(VER)-$(PLAT).tgz $(REXE)  Revisions$(VER).txt  Manual$(VER).txt  ../scripts/sphere_msgs.scp ../scripts/sphere_types.scp ../scripts/sphere_serv_triggers.scp
	@echo DONE -------
	@ls -la SphereServer$(VER)-$(PLAT).tgz
	@tar tvfz SphereServer$(VER)-$(PLAT).tgz
	@echo -----------
#	@scp SphereServer$(VER)-$(PLAT).tgz REVISIONS.TXT MANUAL.TXT uo@www.dagonar.com:~/public_html/sphere/files/
	@rm Revisions$(VER).txt Manual$(VER).txt $(REXE)
