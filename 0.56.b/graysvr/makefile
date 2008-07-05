VER	= 0.56b
PLAT	= Linux

# Generic makefile
MARCH = -march=i686
OPTDEFAULT = -fno-omit-frame-pointer -ffast-math -fpermissive $(MARCH)
#OPT 	= -O0 -fno-expensive-optimizations $(OPTDEFAULT)
OPT	= -O2 $(OPTDEFAULT)
#WARN	= -Wall -W
WARN	= -w

#DEBUG	= -ggdb2
DEBUG	= -s
INCLUDE	= -I../common -I/usr/include/mysql

# Linux
LIBS	= -dynamic -L/usr/lib -lpthread -lmysqlclient -lrt
DEFNIX  = -D_LINUX

NIGHTLYDEFS = -D_NIGHTLYBUILD
#EXTRADEFS = -D_NEWGUILDSYSTEM
#DEBUGDEFS = -D_DEBUG
DEFINES = -DGRAY_SVR -D_CONSOLE -D_REENTRANT $(DEFNIX) $(NIGHTLYDEFS) $(EXTRADEFS) $(DEBUGDEFS)

EXE	= graysvr
REXE	= spheresvr

CC	= g++
CCO	= gcc

NO	= -fno-rtti -fno-exceptions
#EX	= -fexceptions -fasynchronous-exceptions -fsjlj-exceptions
EX	= -fexceptions -fnon-call-exceptions
STRICT  = # -mstrict-align
SPECIAL = $(EX) $(STRICT) $(DEBUG)

PROF	= -pg
PIPE	= -pipe

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
		../common/mtrand/mtrand.cpp \
		../common/twofish/twofish2.cpp \
		../common/libev/wrapper_ev.c \
		../common/zlib/adler32.c \
		../common/zlib/compress.c \
		../common/zlib/crc32.c \
		../common/zlib/deflate.c \
		../common/zlib/gzio.c \
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
		../common/graycom.cpp \
		../sphere/mutex.cpp \
		../sphere/strings.cpp \
		../sphere/threads.cpp \
		../sphere/linuxev.cpp \
		../sphere/asyncdb.cpp

O_FLAGS	= $(WARN) $(PIPE) $(SPECIAL)
C_FLAGS	= $(OPT) $(INCLUDE) $(DEFINES)


.PHONY:	all clean tidy

all:	$(EXE)

clean:	tidy
	rm -f *.o ../common/*.o ../common/mtrand/*.o ../common/twofish/*.o ../common/libev/*.o ../common/zlib/*.o ../sphere/*.o ../tests/*.o $(EXE) $(REXE)

.depend:
	@$(SCCO) -MM $(INCLUDE) $(SRC) $(COMMONSRC) > .depend
	perl -pi -e 's/([^.]+)\.o/o\/\1.o/g' .depend

tidy:
	rm -f *~ *orig *bak *rej ../common/*~ ../common/*orig ../common*bak ../common/mtrand/*~ ../common/mtrand/*orig ../common/mtrand/*bak \
													 ../common/twofish/*~ ../common/twofish/*orig ../common/twofish/*bak ../common/libev/*~ \
													 ../common/libev/*orig ../common/libev/*bak ../common/zlib/*~ ../common/zlib/*orig \
													 ../common/zlib/*bak ../sphere/*~ ../sphere/*orig ../sphere/*bak ../tests/*~ ../tests/*orig ../tests/*bak

tags:	$(SRC)
	ctags $(SRC)

#cmn:	$(COMMONSRC:.cpp=.o)

gray:	$(SRC:.cpp=.o) $(SRC:.c=.co)

#commonlib:	$(COMMONSRC:.cpp=.o)
#	ar r $@ $?

flags:  
	@echo "Compiler Flags: $(CC) -c $(O_FLAGS) $(C_FLAGS)"

$(EXE): flags gray
	@$(CC) $(O_FLAGS) $(C_FLAGS) $(LIBS) -o $(EXE) *.o ../common/*.o ../common/mtrand/*.o ../common/twofish/*.o ../common/libev/*.o ../common/zlib/*.o ../sphere/*.o

version.h:	VERSION
	echo "#define VERSION 'head -l VERSION'" > version.h
	echo "#define COMPILED_BY \"$(COMPILED_BY)\"" >> version.h

%.o:	%.cpp
	@echo " Compiling $<"
	@$(CC) -c $(O_FLAGS) $(C_FLAGS) $< -o $@

%.co:	%.c
	@echo " Compiling $<"
	@$(CCO) -c $(O_FLAGS) $(C_FLAGS) $< -o $(@:.co=.o)

dist:
	@cp $(EXE) $(REXE)
	@cp ../../docs/REVISIONS-56-SERIES.TXT Revisions$(VER).txt
	@cp MANUAL.TXT Manual$(VER).txt
	@cp ../../scripts/sphere_msgs.scp sphere_msgs.scp
	@cp ../../scripts/sphere_types.scp sphere_types.scp
	@cp ../../scripts/sphere_serv_triggers.scp sphere_serv_triggers.scp
	@tar cvfz SphereServer$(VER)-$(PLAT).tgz $(REXE) Revisions$(VER).txt  Manual$(VER).txt  ../Sphere.ini ../sphere.dic ../sphereCrypt.ini sphere_msgs.scp sphere_types.scp sphere_serv_triggers.scp
	@echo DONE -------
	@ls -la SphereServer$(VER)-$(PLAT).tgz
	@tar tvfz SphereServer$(VER)-$(PLAT).tgz
	@echo -----------
#	@scp SphereServer$(VER)-$(PLAT).tgz REVISIONS.TXT MANUAL.TXT uo@www.dagonar.com:~/public_html/sphere/files/
	@rm Revisions$(VER).txt Manual$(VER).txt $(REXE) sphere_msgs.scp sphere_types.scp sphere_serv_triggers.scp
