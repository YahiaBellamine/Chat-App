CC=gcc

PROGS = serveur

OBJ_SERVEUR = server2.o

all: ${PROGS}

serveur: ${OBJ_SERVEUR}
	${CC} -o $@ $^

server2.o: server2.c
	${CC} -c $^