# ODBC Plugin

PLUGIN = odbc.so
include ../makefile.common

INCLUDE := $(INCLUDE)
LIBS := $(LIBS) -lodbc

SRC :=	OdbcConnection.cpp \
	OdbcPlugin.cpp \
	stdafx.cpp

# Targets

clean:	tidy
	rm -f *.o ../bin/$(PLUGIN)

tidy:
	rm -f *~ *orig *bak *rej

plugin:	$(SRC:.cpp=.o) $(SRC:.c=.co)

$(PLUGIN): flags plugin
	@$(CC) $(O_FLAGS) $(C_FLAGS) $(INCLUDE) $(LIBS) -o ../bin/$(PLUGIN) *.o

