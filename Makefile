PROGS =	server

CC = gcc -g

LIBS =  -lsocket\
	/home/courses/cse533/Stevens/unpv13e_solaris2.10/libunp.a

CFLAGS = -I /home/courses/cse533/Stevens/unpv13e_solaris2.10/lib

all: ${PROGS}

server: server.o
	${CC} ${CFLAGS} server.c -o server ${LIBS}
clean:
	rm -f ${PROGS} 
	rm -f *~
	rm -f *.o
