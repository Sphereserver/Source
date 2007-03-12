VER	= 0.0.1
PLAT	= Linux

# Generic makefile
MARCH = -march=i486
OPTDEFAULT = -fno-omit-frame-pointer -ffast-math -fpermissive $(MARCH)
OPT	= -O2 $(OPTDEFAULT) -fpic -shared
WARN	= -w

DEBUG	= -s
INCLUDE	= -I/usr/include/mysql

# Linux
LIBS	= -dynamic -L/usr/lib -lpthread -lmysqlclient

# FreeBSD
#LIBS	= -L/usr/lib -pthread
#DEFBSD	= -D_BSD

DEFINES = -DGRAY_SVR -D_CONSOLE -D_REENTRANT $(DEFBSD) -D_LINUX

EXE	= mysqldll.so

CC	= g++
CCO	= gcc

NO	= -fno-rtti -fno-exceptions
EX	= -fexceptions -fnon-call-exceptions
STRICT  = # -mstrict-align
SPECIAL = $(EX) $(STRICT) $(DEBUG)

PROF	= -pg
PIPE	= -pipe

SRC	:= 	MySqlDll.cpp \
		stdafx.cpp


O_FLAGS	= $(WARN) $(PIPE) $(SPECIAL)
C_FLAGS	= $(OPT) $(INCLUDE) $(DEFINES)


.PHONY:	all clean tidy

all:	$(EXE)

clean:	tidy
	rm -f *.o $(EXE) $(REXE)

.depend:
	@$(CCO) -MM $(INCLUDE) $(SRC) $(COMMONSRC) > .depend
	perl -pi -e 's/([^.]+)\.o/o\/\1.o/g' .depend

tidy:
	rm -f *~ *orig *bak *rej

tags:	$(SRC)
	ctags $(SRC)

#cmn:	$(COMMONSRC:.cpp=.o)

gray:	$(SRC:.cpp=.o) $(SRC:.c=.co)

#commonlib:	$(COMMONSRC:.cpp=.o)
#	ar r $@ $?

flags:  
	@echo "Compiler Flags: $(CC) -c $(O_FLAGS) $(C_FLAGS)"

$(EXE): flags gray
	@$(CC) $(O_FLAGS) $(C_FLAGS) $(LIBS) -o $(EXE) *.o

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
	@cp $(EXE)
	@tar cvfz MySQLDll$(VER)-$(PLAT).tgz
	@echo DONE -------
	@ls -la MySQLDll$(VER)-$(PLAT).tgz
	@tar tvfz MySQLDll$(VER)-$(PLAT).tgz
	@echo -----------
#	@scp SphereServer$(VER)-$(PLAT).tgz REVISIONS.TXT MANUAL.TXT uo@www.dagonar.com:~/public_html/sphere/files/
