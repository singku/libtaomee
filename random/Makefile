DEBUG	= -g -DDEBUG
OPT	= -funroll-loops -fPIC -D__USE_STRING_INLINES

CC	= gcc

SRCS 	=

INCL	+=
CFLAGS	= -Wall $(INCL) -MMD $(DEBUG) $(OPT) 

all: $(SRCS:.c=.o)

install:
	@-mkdir /usr/include/libtaomee/random
	-cp -v *.h /usr/include/libtaomee/random
%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<
	@-mv -f $*.d .dep.$@

clean:
	@-rm -f *.o *.po *.so *.d .dep.* core

-include /dev/null $(wildcard .dep.*)
