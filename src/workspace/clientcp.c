/*
 *			C L I E N T C P
 *
 *	This is an example program that demonstrates the use of
 *	stream sockets as an IPC mechanism.  This contains the client,
 *	and is intended to operate in conjunction with the server
 *	program.  Together, these two programs
 *	demonstrate many of the features of sockets, as well as good
 *	conventions for using these features.
 *
 *
 */
 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include "tftp.h"

#define PUERTO 17278
#define TAM_BUFFER 1024

/*
 *			M A I N
 *
 *	This routine is the client which request service from the remote.
 *	It creates a connection, sends a number of
 *	requests, shuts down the connection in one direction to signal the
 *	server about the end of data, and then receives all of the responses.
 *	Status will be written to stdout.
 *
 *	The name of the system to which the requests will be sent is given
 *	as a parameter to the command.
 */
int main(argc, argv)
int argc;
char *argv[];
{
    int s;				/* connected socket descriptor */
   	struct addrinfo hints, *res;
    long timevar;			/* contains time returned by time() */
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in servaddr_in;	/* for server socket address */
	int addrlen, i, j, errcode;
    /* This example uses TAM_BUFFER byte messages. */
	char buf[TAM_BUFFER];
	
	
	/* VARIABLES UTILIZADAS POR NOSOTROS */
	
	int eof_flag;
	char rfilename[] = "thisispdf.txt"; /* Esto luego será argv[4] */
	char wfilename[] = "fichero1.txt";  /* Esto luego será argv[4] */
	int nwrittenbytes = 0; /* Keeps the number of bytes written in a file */
	int nreadbytes = 0; /* Keeps the number of bytes read from file */
	char str[1000];
	short msg_type = 0;
	int tcp_mode = -1; /* Esto luego será argv[3] */
    int last_block = -1;
	FILE *ptr = NULL;
	// Mensajes de emisión
	rw_msg_t *rw_msg = NULL;
	ack_msg_t *ack_msg = NULL;
	data_msg_t *data_msg_rcv = NULL;
	// Mensajes de recepción
	data_msg_t data_msg;
	ack_msg_t ack_msg_rcv;

	if (argc == 1) { // clientcp localhost [r|w]
		fprintf(stderr, "Usage:  %s <remote host><[r|w] request>\n", argv[0]);
		exit(1);
	} else if (argc == 2) {
		fprintf(stderr, "Usage:  %s %s <[r|w] request>\n", argv[0], argv[1]);
		exit(1);
	} else if (argc == 3) {
		tcp_mode = WRITE_TYPE; //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< Lo tengo por defecto puesto en modo lectura

	}

	s = socket (AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
		exit(1);
	}
	
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

	servaddr_in.sin_family = AF_INET;
	
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;

    errcode = getaddrinfo (argv[1], NULL, &hints, &res); 
    if (errcode != 0){
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				argv[0], argv[1]);
		exit(1);
    } else {
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	}
	
    freeaddrinfo(res);

	servaddr_in.sin_port = htons(PUERTO);

	if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
		exit(1);
	}

	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
		exit(1);
	 }

	time(&timevar);

	printf("Connected to %s on port %u at %s",
			argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));


    /* R/W REQUEST CLIENT ---> SERVER */
    
    eof_flag = 0;
    
    if (tcp_mode == READ_TYPE){
    	rw_msg = create_rw_msg(READ_TYPE, rfilename);
    } else if (tcp_mode == WRITE_TYPE) {
	    rw_msg = create_rw_msg(WRITE_TYPE, wfilename);
	}

    memcpy((void *)buf, (const void *)rw_msg, sizeof(*rw_msg));
    printf("Enviando el fichero %s\n", rw_msg->filename);
    
    if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) {
		fprintf(stderr, "%s: Connection aborted on error ",	argv[0]);
		fprintf(stderr, "on send number %d\n", i);
		exit(1);
	}
    
    
    /* RECEPTION CLIENT <--- SERVER */
    
	while (i = recv(s, buf, TAM_BUFFER, 0)) {
		if (i == -1) {
            perror(argv[0]);
			fprintf(stderr, "%s: error reading result\n", argv[0]);
			exit(1);
		}
			 
	    
		while (i < TAM_BUFFER) {
			j = recv(s, &buf[i], TAM_BUFFER-i, 0);
			if (j == -1) {
                     	     perror(argv[0]);
			     fprintf(stderr, "%s: error reading result\n", argv[0]);
			     exit(1);
               		}
			i += j;
		}
		
		msg_type = *buf;
		
		switch(msg_type) {
		case ACK_TYPE:
		    memcpy((void *)&ack_msg_rcv, (const void *)&buf, sizeof(ack_msg));		
	
	        if (0 != last_block && last_block == ack_msg_rcv.n_block){
	            fprintf(stderr,"\nReached end of file\n\n");
	            eof_flag = 1;
	            break;
		     }
		    /*
		     * Para el ACK X:
		     *      1- Enviar bloque X + 1
		     */

		     data_msg_rcv = create_data_msg(ack_msg_rcv.n_block + 1);
		     
		     if ((last_block = read_from_file(data_msg_rcv, rfilename, nreadbytes)) == -1){
                    fprintf(stderr, "clientcp.c: DATA_TYPE: error in read_from_file\n");
	                return -1;
             }
		     
		     printf("Received ACK number %d\n", ack_msg_rcv.n_block);
		     memcpy((void *)buf, (const void *)data_msg_rcv, sizeof(*data_msg_rcv));
		     nreadbytes += TFTP_DATA_SIZE;

		     if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER){
		        printf("clientcp.c: ACK_TYPE: error in send\n");
	                return -1;
		     }
		
		break;
		case DATA_TYPE:
		    fprintf(stderr,"EN DATA_TYPE\n");
		    memcpy((void *)&data_msg, (const void *)&buf, sizeof(data_msg));
		
		    /*
		     * Para el bloque X
		     *      1- Escribir DATA X
		     *      2- Mandar ACK (n = X)
		     */
            if (TFTP_DATA_SIZE == strlen(data_msg.data)) { // DATA = 512     
                
                if ((write_data_into_file(data_msg, wfilename, nwrittenbytes)) == -1){
                    fprintf(stderr, "clientcp.c: DATA_TYPE: error in write_data_into_file\n");
	                return -1;
                }
                
                printf("Received block number %d\n", data_msg.n_block);
                ack_msg = create_ack_msg(data_msg.n_block);
                memcpy((void *)buf, (const void *)ack_msg, sizeof(*ack_msg));
                nwrittenbytes += TFTP_DATA_SIZE;
                
                if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER){
		            printf("clientcp.c: DATA_TYPE: error in send\n");
	                return -1;
		        }
                
            } else if (TFTP_DATA_SIZE > strlen(data_msg.data)) { // DATA < 512  
   
                if ((write_data_into_file(data_msg, wfilename, nwrittenbytes)) == -1){
                    fprintf(stderr, "clientcp.c: DATA_TYPE: error in write_data_into_file\n");
	                return -1;
                }
                    
                printf("Received block number %d\n", data_msg.n_block);

                ack_msg = create_ack_msg(data_msg.n_block);
                memcpy((void *)buf, (const void *)ack_msg, sizeof(*ack_msg));
                nwrittenbytes += strlen(data_msg.data);
                
                if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER){
		            fprintf(stderr, "clientcp.c: DATA_TYPE: error in send\n");
	                return -1;
		        }
		        
		        eof_flag = 1;
		         
            } else if (0 == strlen(data_msg.data)) { // DATA = 0
            
                printf("Received block number %d\n", data_msg.n_block);
                ack_msg = create_ack_msg(data_msg.n_block);
                memcpy((void *)buf, (const void *)ack_msg, sizeof(*ack_msg));
              
                if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER){
	                printf("clientcp.c: DATA_TYPE: error in send\n");
                    return -1;
	            }
	            
	            eof_flag = 1;
	            
            } 

		break;
		case ERROR_TYPE:
		
		    /*
		     * Basicamente, finalizar la conexion
		     */
		     
		break;
		default:
		    printf("clientcp: default: Invalid type of message\n");
		    return -1;
		break;
		}
		
		if(eof_flag)
		    break;
	}

    /* Print message indicating completion of task. */
	time(&timevar);
	printf("All done at %s", (char *) ctime(&timevar));
	return 0;
}
