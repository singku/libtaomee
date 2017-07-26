INC_DIR=
LIB_DIR=
LIB= -lconfuse
FLAG= -g -Wall
CC=gcc

BINARY=ctrl_tlog

all: $(BINARY)

ctrl_tlog: ctrl_tlog.c ctrl_tlog.h svc.c svc.h
	$(CC) $(FLAG) $(INC_DIR) -o $@ $^ $(LIB)

clean:
	rm -rf *.o $(BINARY)
