/*
 *          		S E R V I D O R
 *
 *	This is an example program that demonstrates the use of
 *	sockets TCP and UDP as an IPC mechanism.  
 *
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "tftp.h"
#include "debugging.h"


#define PUERTO 17278
#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define TAM_BUFFER 1024
#define MAXHOST 128

extern int errno;

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */
 
void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in);
void errout(char *);		/* declare error out routine */

int FIN = 0;             /* Para el cierre ordenado */
void finalizar(){ FIN = 1; }

int main(argc, argv)
int argc;
char *argv[];
{

    int s_TCP, s_UDP;		/* connected socket descriptor */
    int ls_TCP;				/* listen socket descriptor */
    
    int cc;				    /* contains the number of bytes read */
     
    struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */
    
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in clientaddr_in;	/* for peer socket address */
	int addrlen;
	
    fd_set readmask;
    int numfds,s_mayor;
    
    char buffer[BUFFERSIZE];	/* buffer for packets to be read into */
    
    struct sigaction vec;

		/* Create the listen socket. */
	ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

    addrlen = sizeof(struct sockaddr_in);

		/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
		/* The server should listen on the wildcard address,
		 * rather than its own internet address.  This is
		 * generally good practice for servers, because on
		 * systems which are connected to more than one
		 * network at once will be able to have one server
		 * listening on all networks at once.  Even when the
		 * host is connected to only one network, this is good
		 * practice, because it makes the server program more
		 * portable.
		 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}
		/* Initiate the listen on the socket so remote users
		 * can connect.  The listen backlog is set to 5, which
		 * is the largest currently supported.
		 */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}
	
	
	/* Create the socket UDP. */
	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	   }
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	    }

		/* Now, all the initialization of the server is
		 * complete, and any user errors will have already
		 * been detected.  Now we can fork the daemon and
		 * return to the user.  We need to do a setpgrp
		 * so that the daemon will no longer be associated
		 * with the user's control terminal.  This is done
		 * before the fork, so that the child will not be
		 * a process group leader.  Otherwise, if the child
		 * were to open a terminal, it would become associated
		 * with that terminal as its control terminal.  It is
		 * always best for the parent to do the setpgrp.
		 */
	setpgrp();

	switch (fork()) {
	case -1:		/* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0:     /* The child process (daemon) comes here. */

			/* Close stdin and stderr so that they will not
			 * be kept open.  Stdout is assumed to have been
			 * redirected to some logging file, or /dev/null.
			 * From now on, the daemon will not report any
			 * error messages.  This daemon will loop forever,
			 * waiting for connections and forking a child
			 * server to handle each one.
			 */
		fclose(stdin);
		fclose(stderr);

			/* Set SIGCLD to SIG_IGN, in order to prevent
			 * the accumulation of zombies as each child
			 * terminates.  This means the daemon does not
			 * have to make wait calls to clean them up.
			 */
		if ( sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror(" sigaction(SIGCHLD)");
            fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
            exit(1);
            }
            
		    /* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
        vec.sa_handler = (void *) finalizar;
        vec.sa_flags = 0;
        if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
            perror(" sigaction(SIGTERM)");
            fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
            exit(1);
            }
        
		while (!FIN) {
            /* Meter en el conjunto de sockets los sockets UDP y TCP */
            FD_ZERO(&readmask);
            FD_SET(ls_TCP, &readmask);
            FD_SET(s_UDP, &readmask);
            /* 
            Seleccionar el descriptor del socket que ha cambiado. Deja una marca en 
            el conjunto de sockets (readmask)
            */ 
    	    if (ls_TCP > s_UDP) s_mayor=ls_TCP;
    		else s_mayor=s_UDP;

            if ( (numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
                if (errno == EINTR) {
                    FIN=1;
		            close (ls_TCP);
		            close (s_UDP);
                    perror("\nFinalizando el servidor. SeÃal recibida en elect\n "); 
                }
            }
           else { 

                /* Comprobamos si el socket seleccionado es el socket TCP */
                if (FD_ISSET(ls_TCP, &readmask)) {
                    /* Note that addrlen is passed as a pointer
                     * so that the accept call can return the
                     * size of the returned address.
                     */
    				/* This call will block until a new
    				 * connection arrives.  Then, it will
    				 * return the address of the connecting
    				 * peer, and a new socket descriptor, s,
    				 * for that connection.
    				 */
    			s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
    			if (s_TCP == -1) exit(1);
    			switch (fork()) {
        			case -1:	/* Can't fork, just exit. */
        				exit(1);
        			case 0:		/* Child process comes here. */
                    			close(ls_TCP); /* Close the listen socket inherited from the daemon. */
        				serverTCP(s_TCP, clientaddr_in);
        				exit(0);
        			default:	/* Daemon process comes here. */
        					/* The daemon needs to remember
        					 * to close the new accept socket
        					 * after forking the child.  This
        					 * prevents the daemon from running
        					 * out of file descriptor space.  It
        					 * also means that when the server
        					 * closes the socket, that it will
        					 * allow the socket to be destroyed
        					 * since it will be the last close.
        					 */
        				close(s_TCP);
        			}
             } /* De TCP*/
          /* Comprobamos si el socket seleccionado es el socket UDP */
          if (FD_ISSET(s_UDP, &readmask)) {
                /* This call will block until a new
                * request arrives.  Then, it will
                * return the address of the client,
                * and a buffer containing its request.
                * BUFFERSIZE - 1 bytes are read so that
                * room is left at the end of the buffer
                * for a null character.
                */
                cc = recvfrom(s_UDP, buffer, BUFFERSIZE - 1, 0,
                   (struct sockaddr *)&clientaddr_in, &addrlen);
                if ( cc == -1) {
                    perror(argv[0]);
                    printf("%s: recvfrom error\n", argv[0]);
                    exit (1);
                    }
                /* Make sure the message received is
                * null terminated.
                */
                buffer[cc]='\0';
                serverUDP (s_UDP, buffer, clientaddr_in);
                }
          }
		}   /* Fin del bucle infinito de atención a clientes */
        /* Cerramos los sockets UDP y TCP */
        close(ls_TCP);
        close(s_UDP);
    
        printf("\nFin de programa servidor!\n");
        
	default:		/* Parent process comes here. */
		exit(0);
	}

}

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
    /* Estas variables son para la gestion de los ficheros.
     * Realmente habria que cambiarlas para trabajar con arrays dinamicos
     * pero por ahora vamos a tener un unico fichero (y solo pa' leer)
     */
     int nreadbytes = 0; /* Keeps the number of bytes read from file */
     int nwrittenbytes = 0;
     int rreqcnt = 1; /* Keeps count of read requests */
     char filename[100];
     rw_msg_t rw_msg;
     data_msg_t *data_msg = NULL;
     data_msg_t data_msg_rcv;
     ack_msg_t ack_msg;
     ack_msg_t *ack_msg_send = NULL;
     char str[1000];
     short msg_type = 0;
     FILE *ptr = NULL;
     int last_block = -1;
   /*******************************************************************/


	int reqcnt = 0;		/* keeps count of number of requests */
	char buf[TAM_BUFFER];		/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST];		/* remote host's name string */
	
    int len, len1, status;
    struct hostent *hp;		/* pointer to host info for remote host */
    long timevar;			/* contains time returned by time() */
    
    struct linger linger;		/* allow a lingering, graceful close; */
    				            /* used when setting SO_LINGER */
    				
	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */
	 
     status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);
     if(status){
           	/* The information is unavailable for the remote
			 * host.  Just format its internet address to be
			 * printed out in the logging information.  The
			 * address will be shown in "internet dot format".
			 */
			 /* inet_ntop para interoperatividad con IPv6 */
            if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            	perror(" inet_ntop \n");
             }
    /* Log a startup message. */
    time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	printf("Startup from %s port %u at %s",
		hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));

		/* Set the socket for a lingering, graceful close.
		 * This will cause a final close of this socket to wait until all of the
		 * data sent on it has been received by the remote host.
		 */
	linger.l_onoff  =1;
	linger.l_linger =1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
					sizeof(linger)) == -1) {
		errout(hostname);
	}

		/* Go into a loop, receiving requests from the remote
		 * client.  After the client has sent the last request,
		 * it will do a shutdown for sending, which will cause
		 * an end-of-file condition to appear on this end of the
		 * connection.  After all of the client's requests have
		 * been received, the next recv call will return zero
		 * bytes, signalling an end-of-file condition.  This is
		 * how the server will know that no more requests will
		 * follow, and the loop will be exited.
		 */
printmtof("Aqui esperando al CLIENT ",
		                  "debug.txt");
	while (len = recv(s, buf, TAM_BUFFER, 0)) {
		if (len == -1) errout(hostname); /* error from recv */
			/* The reason this while loop exists is that there
			 * is a remote possibility of the above recv returning
			 * less than TAM_BUFFER bytes.  This is because a recv returns
			 * as soon as there is some data, and will not wait for
			 * all of the requested data to arrive.  Since TAM_BUFFER bytes
			 * is relatively small compared to the allowed TCP
			 * packet sizes, a partial receive is unlikely.  If
			 * this example had used 2048 bytes requests instead,
			 * a partial receive would be far more likely.
			 * This loop will keep receiving until all TAM_BUFFER bytes
			 * have been received, thus guaranteeing that the
			 * next recv at the top of the loop will start at
			 * the begining of the next request.
			 */
			 		
		while (len < TAM_BUFFER) {
			len1 = recv(s, &buf[len], TAM_BUFFER-len, 0);
			if (len1 == -1) errout(hostname);
			len += len1;
		}
		

		
		msg_type = *buf;
	
		// Vamo a aserlo con 1 fixero
		// Ya si eso luego bien (arrays dinamicos con structs para cada fichero)
		printmtof("LLega hasta el servidor. OK ",
		                  "debug.txt");
		char prueba[100];
		sprintf(prueba,"Msg_type es %d\n",msg_type);
		printmtof(prueba,
		                  "debug.txt");
		switch(msg_type) {
		case READ_TYPE:
		    
		    memcpy((void *)&rw_msg, (const void *)&buf, sizeof(rw_msg));

		    data_msg = create_data_msg(1);
		    
		    /*
		     * 1- Leer de fichero 
		     * 2- Crear un mensaje DATA
		     * 3- Enviar datos
		     */
		     
		     if ((ptr = fopen(rw_msg.filename, "r")) == NULL) {
		        printmtof("servidor.c: serverTCP: READ_TYPE: could not open file to read: ",
		                  "debug.txt");
		        printmtof(rw_msg.filename, "debug.txt");
		        return;
		     }
		     
		     if (0 != fseek(ptr, nreadbytes, SEEK_SET)) {
		        printmtof("servidor.c: serverTCP: READ_TYPE: error in fseek",
		                  "debug.txt");
		        return;
		     }
		     
		     if (1 != fread((void *)&(data_msg->data), sizeof(data_msg->data), 1, ptr)){
	             if (ferror(ptr)) {
	                printmtof("servidor.c: serverTCP: READ_TYPE: error in fread",
	                          "debug.txt");
	                return;
	             } else if (feof(ptr)) {
	                if (0 == strlen(data_msg->data)) {
	                    last_block = data_msg->n_block;
	                }
	             }
		     }
		     
		     if (0 != fclose(ptr)){
		         printmtof("servidor.c: serverTCP: READ_TYPE: error in fclose",
	                          "debug.txt");
	             return;
		     }
		     
		     ptr = NULL;
		     
		     memcpy((void *)buf, (const void *)data_msg, sizeof(*data_msg));
		     strncpy(filename,(const char*) &(rw_msg.filename), sizeof(filename));
		    
		     nreadbytes += TFTP_DATA_SIZE;
		break;
		case WRITE_TYPE:
		     printmtof("Entro en WriteType",
	                          "debug.txt");
		     memcpy((void *)&rw_msg, (const void *)&buf, sizeof(rw_msg));
		     strncpy(filename,(const char*) &(rw_msg.filename), sizeof(filename));
		    /*
		     * 1- Enviar ACK (n = 0)
		     */

		     ack_msg_send = create_ack_msg(0);

		     memcpy((void *)buf, (const void *)ack_msg_send, sizeof(*ack_msg_send));
		     printmtof("Acabo en WriteType",
	                          "debug.txt");
		     
		break;
		case ACK_TYPE:
			
		    memcpy((void *)&ack_msg, (const void *)&buf, sizeof(ack_msg));
		    
		    if (-1 != last_block && last_block == ack_msg.n_block){
			printmtof("GOTO ",
		                  "debug.txt");
		        goto END_OF_FILE;
		    }
		    
		    //rreqcnt = ack_msg.n_block + 1;
		    data_msg = create_data_msg(ack_msg.n_block + 1);
		
		    /*
		     * Para el ACK X:
		     *      1- Enviar bloque X + 1
		     */
		     printmtof("Abro el fichero ",
		                  "debug.txt");
		     if ((ptr = fopen(filename, "r")) == NULL) {
		        printmtof("servidor.c: serverTCP: ACK_TYPE: could not open file to read",
		                  "debug.txt");
		        return;
		     }
		     
		     printmtof("Me situo en la posicion: ",
		                  "debug.txt");
		     sprintf(prueba,"%d\n",nreadbytes);
		     printmtof(prueba,
		                  "debug.txt");
		     if (0 != fseek(ptr, nreadbytes, SEEK_SET)) {
		        printmtof("servidor.c: serverTCP: ACK_TYPE: error in fseek",
		                  "debug.txt");
		        return;
		     }
		    
		     printmtof("Me pongo a leer: ",
		                  "debug.txt");
		     if (1 != fread((void *)&(data_msg->data), sizeof(data_msg->data), 1, ptr)){
			     if (ferror(ptr)) {
			        printmtof("servidor.c: serverTCP: ACK_TYPE: error in fread",
			                  "debug.txt");
			        return;
			     } else if (feof(ptr)) {
				printmtof("Reached END OF FILE (MANDO BLOQUE VACIO): ",
				          "debug.txt");
			        if (0 == strlen(data_msg->data)) {
			            goto END_OF_FILE;
			        }
			        return;
			     }
		     }
		     
		     if (0 != fclose(ptr)){
		         printmtof("servidor.c: serverTCP: ACK_TYPE: error in fclose",
	                          "debug.txt");
	             return;
		     }
		     
		     ptr = NULL;
		     printmtof("VOY A ENVIAR EL MALDITO MENSAJE ",
		                  "debug.txt");
		     memcpy((void *)buf, (const void *)data_msg, sizeof(*data_msg));
		     nreadbytes += TFTP_DATA_SIZE;

		break;
		case DATA_TYPE:
		    printmtof("Entro en DataType",
	                          "debug.txt");
		    memcpy((void *)&data_msg_rcv, (const void *)&buf, sizeof(data_msg_rcv));

		    if (0 == strlen(data_msg_rcv.data)){
			printmtof("GOTO ",
		                  "debug.txt");
		        goto END_OF_FILE;
		    }
		    /*
		     * Para el bloque X
		     *      1- Escribir DATA X
		     *      2- Mandar ACK (n = X)
		     */
		     
		     if ((ptr = fopen(filename, "a")) == NULL) {
		        printmtof("servidor.c: serverTCP: DATA_TYPE: could not open file to read\n",
	                          "debug.txt");
		        return;
		     }
		     
		     if (0 != fseek(ptr, nwrittenbytes, SEEK_SET)) {
		        printmtof("servidor.c: serverTCP: DATA_TYPE: error in fseek\n",
	                          "debug.txt");
		        return;
		     }
		     
		     if (1 != fwrite((void *)&(data_msg_rcv.data), sizeof(data_msg_rcv.data), 1, ptr)){
	             	printmtof("servidor.c: serverTCP: DATA_TYPE: error in fwrite\n",
	                          "debug.txt");
	             	return;
		     }

		     if (0 != fclose(ptr)){
		         printmtof("servidor.c: serverTCP: DATA_TYPE: error in fclose\n",
	                          "debug.txt");
	             	 return;
		     }

        	     ptr = NULL;
		     
		     ack_msg_send = create_ack_msg(data_msg_rcv.n_block);

		     memcpy((void *)buf, (const void *)ack_msg_send, sizeof(*ack_msg_send));

		     nwrittenbytes += TFTP_DATA_SIZE;
		     printmtof("Salgo de DataType",
	                          "debug.txt");
		break;
		case ERROR_TYPE:
		
		    /*
		     * Basicamente, finalizar la conexion
		     */
		     
		break;
		default:
		    printmtof("servidor.c: serverTCP: default: invalid message type\n",
	                          "debug.txt");
		    return;
		break;
		}
		
		/*
		snprintf(str, sizeof(str),
		         "(READ: \'%d\' bytes)\n\
HEMOS ENCONTRAO\' UN STRUCT:\n\
\t- Message type: \'%d\'\n\
\t- Filename: \'%s\'\n\
\t- Mode: \'%s\'\n",
		         len, rw_msg.msg_type, rw_msg.filename, rw_msg.mode);

		printmtof(str, "read.txt"); // JAJA
		*/
		
			/* Increment the request count. */
		reqcnt++;
		msg_type = 0;
			/* This sleep simulates the processing of the
			 * request that a real server might do.
			 */
		sleep(1);
		printmtof("Servidor Envia el mensaje al cliente",
		                  "debug.txt");
			/* Send a response back to the client. */
		if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
	}

END_OF_FILE:
		/* The loop has terminated, because there are no
		 * more requests to be serviced.  As mentioned above,
		 * this close will block until all of the sent replies
		 * have been received by the remote host.  The reason
		 * for lingering on the close is so that the server will
		 * have a better idea of when the remote has picked up
		 * all of the data.  This will allow the start and finish
		 * times printed in the log file to reflect more accurately
		 * the length of time this connection was used.
		 */
	close(s);

		/* Log a finishing message. */
	time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	printf("Completed %s port %u, %d requests, at %s\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *) ctime(&timevar));
}

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);     
}


/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in)
{
    struct in_addr reqaddr;	/* for requested host's address */
    struct hostent *hp;		/* pointer to host info for requested host */
    int nc, errcode;

    struct addrinfo hints, *res;

	int addrlen;
    
   	addrlen = sizeof(struct sockaddr_in);

      memset (&hints, 0, sizeof (hints));
      hints.ai_family = AF_INET;
		/* Treat the message as a string containing a hostname. */
	    /* Esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta. */
    errcode = getaddrinfo (buffer, NULL, &hints, &res); 
    if (errcode != 0){
		/* Name was not found.  Return a
		 * special value signifying the error. */
		reqaddr.s_addr = ADDRNOTFOUND;
      }
    else {
		/* Copy address of host into the return buffer. */
		reqaddr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	}
     freeaddrinfo(res);

	nc = sendto (s, &reqaddr, sizeof(struct in_addr),
			0, (struct sockaddr *)&clientaddr_in, addrlen);
	if ( nc == -1) {
         perror("serverUDP");
         printf("%s: sendto error\n", "serverUDP");
         return;
         }   
 }
