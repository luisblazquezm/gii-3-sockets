CC = gcc
CFLAGS = 
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
LIBS =

PROGS = servidor clientcp clientudp
FILES = debug.txt fichero1.txt

all: ${PROGS}

servidor: servidor.o debugging.o tftp.o debugging.h tftp.h
	${CC} ${CFLAGS} -o $@ servidor.o debugging.o tftp.o debugging.h tftp.h ${LIBS}
	
clientcp: clientcp.o tftp.o tftp.h
	${CC} ${CFLAGS} -o $@ clientcp.o tftp.o tftp.h ${LIBS}

clientudp: clientudp.o tftp.o tftp.h
	${CC} ${CFLAGS} -o $@ clientudp.o tftp.o tftp.h ${LIBS}

exe: servidor clientcp
	./servidor; ./clientcp 127.0.0.1

clean:
	rm *.o ${PROGS} ${FILES}
