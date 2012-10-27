PROGS =	server client

CC = gcc 

FLAGS = -g -O2

LIBS =  -lsocket -lresolv -lnsl -D_REENTRANT \
	/home/courses/cse533/Stevens/unpv13e_solaris2.10/libunp.a \

CFLAGS = ${FLAGS} -I /home/courses/cse533/Stevens/unpv13e_solaris2.10/lib \
	-I /home/courses/cse533/Asgn2_code

all: ${PROGS}

server: server.o 
	${CC} ${CFLAGS} -o $@ /home/courses/cse533/Asgn2_code/get_ifi_info_plus.o server.o ${LIBS}

server.o: server.c
	${CC} ${CFLAGS} -c server.c
	
client: client.o rttC.o
	${CC} ${CFLAGS} -o $@ /home/courses/cse533/Asgn2_code/get_ifi_info_plus.o client.o rttC.o ${LIBS}

client.o: client.c
	${CC} ${CFLAGS} -c client.c
	
example02: example02.o list.o
	${CC} ${CFLAGS} -o $@ example02.o ${LIBS}

list: 
	${CC} ${CFLAGS} list.c ${LIBS}
	
rttC.o: rttC.c
	${CC} ${CFLAGS} -c rttC.c

clean:
	rm -f ${PROGS} 
	rm -f *~
	rm -f *.o
