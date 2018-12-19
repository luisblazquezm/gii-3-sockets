CC = gcc
CFLAGS = 
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
LIBS =

PROGS = servidor cliente
FILES = .debug.txt peticiones.log ficherosTFTPserver/fichero3.txt ficherosTFTPserver/fichero4.txt ficherosTFTPserver/fichero6.txt ficherosTFTPclient/fichero1.txt ficherosTFTPclient/fichero2.txt ficherosTFTPclient/fichero5.txt

all: ${PROGS}

servidor: servidor.o tftp.o tftp.h
	${CC} ${CFLAGS} -o $@ servidor.o tftp.o tftp.h ${LIBS}
	
cliente: cliente.o tftp.o tftp.h utils.o utils.h
	${CC} ${CFLAGS} -o $@ cliente.o tftp.o utils.o utils.h tftp.h ${LIBS}

clean:
	rm *.o ${PROGS} ${FILES}
