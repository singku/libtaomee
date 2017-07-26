DEBUG	= -g -DDEBUG
OPT	= -funroll-loops -fPIC -D__USE_STRING_INLINES

CC	= gcc

SRCS 	=

INCL	+=
CFLAGS	= -Wall $(INCL) -MMD $(DEBUG) $(OPT) 

all: $(SRCS:.c=.o) subdir
	-cp *.o ../
install:
	@-mkdir /usr/include/libtaomee/project
	-cp -v *.h /usr/include/libtaomee/project
	make -C stat_agent install
%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<
	@-mv -f $*.d .dep.$@

subdir: 
	make -C stat_agent

clean:
	@-rm -f *.o *.po *.so *.d .dep.* core
	make -C stat_agent clean

-include /dev/null $(wildcard .dep.*)
