CC=gcc

PROGS = server

OBJ_SERVEUR = server.o

all: ${PROGS}

server: ${OBJ_SERVEUR}
	${CC} -o $@ $^ -g

server.o: server.c
	${CC} -c $^
