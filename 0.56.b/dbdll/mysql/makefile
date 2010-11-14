# MySQL Plugin

PLUGIN = mysql.so
include ../makefile.common

INCLUDE	:= $(INCLUDE) -I/usr/include/mysql
LIBS := $(LIBS) -lmysqlclient

SRC :=	MySqlConnection.cpp \
	MySqlPlugin.cpp \
	stdafx.cpp

# Targets

clean:	tidy
	rm -f *.o ../bin/$(PLUGIN)

tidy:
	rm -f *~ *orig *bak *rej

plugin:	$(SRC:.cpp=.o) $(SRC:.c=.co)

$(PLUGIN): flags plugin
	@$(CC) $(O_FLAGS) $(C_FLAGS) $(INCLUDE) $(LIBS) -o ../bin/$(PLUGIN) *.o

