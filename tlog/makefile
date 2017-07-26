INC_DIR = 
LIB_DIR =
LIBS = -pthread
FLAG = -g -Wall
HEADERS= tlog.h fileutils.h netutils.h tlog_decl.h tlog_macros.h

CC=gcc

BINARY=test

all: $(BINARY)

test: test.c tlog.c fileutils.c netutils.c $(HEADERS)
	$(CC) $(FLAG) $(INC_DIR) -o $@ $^ $(LIB_DIR) $(LIBS)

clean:
	rm -rf *.o $(BINARY)
