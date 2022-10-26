CC=gcc

PROGS = client

OBJ_CLIENT = client2.o

all: ${PROGS}

client: ${OBJ_CLIENT}
	${CC} -o $@ $^

client2.o: client2.c
	${CC} -c $^
