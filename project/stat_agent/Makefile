#DEBUG	= -g -DDEBUG
DEBUG	= -O2 -DDEBUG
OPT	= -funroll-loops -fPIC -D__USE_STRING_INLINES

CC	= gcc

SRCS 	= msglog.c

INCL	+=
CFLAGS	= -Wall $(INCL) -MMD $(DEBUG) $(OPT) 

all: $(SRCS:.c=.o)
	-cp *.o ../../
install:
	@-mkdir /usr/include/libtaomee/project/stat_agent
	-cp -v *.h /usr/include/libtaomee/project/stat_agent
%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<
	@-mv -f $*.d .dep.$@

clean:
	@-rm -f *.o *.po *.so *.d .dep.* core

-include /dev/null $(wildcard .dep.*)
