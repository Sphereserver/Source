 
SQUIRREL= ../..


OUT= $(SQUIRREL)/libs/libtwofish.a
INCZ= -I.
DEFS= 
LIB=	stdc++

OBJS= \
	twofish2.o
	
SRCS= \
	twofish2.cpp

	
	
squirrellib:
	gcc -O2 -c $(SRCS) $(INCZ) $(DEFS)
	ar rc $(OUT) *.o
	ranlib $(OUT)
	rm *.o