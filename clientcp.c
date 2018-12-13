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

#define PUERTO 6357
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
	char wfilename[50]; //= "fichero1.txt";  /* Esto luego será argv[4] */
	int nwrittenbytes = 0; /* Keeps the number of bytes written in a file */
	int nreadbytes = 0; /* Keeps the number of bytes read from file */
	char str[1000];
	short msg_type = 0;
	int tcp_mode = -1; /* Esto luego será argv[3] */
    char last_block[3] = "00";//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    int lastblock = 0;
	FILE *ptr = NULL;

	if (argc == 1) { // clientcp localhost [r|w] filename
		fprintf(stderr, "Usage:  %s <remote host><[r|w] request>\n", argv[0]);
		exit(1);
	} else if (argc == 2) {
		fprintf(stderr, "Usage:  %s %s <[r|w] request>\n", argv[0], argv[1]);
		exit(1);
	} else if (argc == 4) {
	    if (!strcmp(argv[2], "r"))
	        tcp_mode = READ_TYPE;
	    else if (!strcmp(argv[2], "l"))
		    tcp_mode = WRITE_TYPE;
		    
	    strncpy(wfilename,argv[3], sizeof(wfilename));
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
    	char *r_msg = create_r_msg(rfilename);
	    if (send(s, r_msg, TAM_BUFFER, 0) != TAM_BUFFER) {
		    fprintf(stderr, "%s: Connection aborted on error ",	argv[0]);
		    fprintf(stderr, "on send number %d\n", i);
		    exit(1);
	    }
	    printf("Enviando el fichero %s\n", rfilename);
    } else if (tcp_mode == WRITE_TYPE) {
	    char *w_msg = create_w_msg(wfilename);
	    if (send(s, w_msg, TAM_BUFFER, 0) != TAM_BUFFER) {
		    fprintf(stderr, "%s: Connection aborted on error ",	argv[0]);
		    fprintf(stderr, "on send number %d\n", i);
		    exit(1);
	    }
	    printf("Enviando el fichero %s\n", wfilename);
    }
    
    
    /* RECEPTION CLIENT <--- SERVER */
    
	while (i = recv(s, buf, TAM_BUFFER , 0)) {
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
		
		
		if (buf[0] == '0' && buf[1] == '4') { //ACK
		    fprintf(stderr,"EN ACK_TYPE\n");
		    char nblock[3];
		    strncpy(nblock, buf + 2, 2);
		    nblock[2] = '\0';
		    
		    char *data;
            data = calloc(1, TFTP_DATA_SIZE);

		    if (('0' == last_block[0] && '0' != last_block[1]) && (nblock[0] == last_block[0] && nblock[1] == last_block[1])){
			    fprintf(stderr,"\nReached end of file\n\n");
			    eof_flag = 1;
			    break;
		    }

		    /*
		     * Para el ACK X:
		     *      1- Enviar bloque X + 1
		     */   
		     

		     if ((lastblock = read_from_file(data, rfilename, nreadbytes)) == -1){//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< &buf???
		            fprintf(stderr, "clientcp.c: DATA_TYPE: error in read_from_file\n");
			        return -1;
		     }
		     
		     if (lastblock == 1){
		        strncpy(last_block, buf + 2, 2);
		        last_block[2] = '\0';
		      }
		     
		     int n_block = atoi(nblock);
		     
		     printf("Received ACK number %d\n", n_block);

		     char *data_msg = create_data_msg(n_block, data);

		     nreadbytes += TFTP_DATA_SIZE;

		     if (send(s, data_msg, TAM_BUFFER, 0) != TAM_BUFFER){
		        printf("clientcp.c: ACK_TYPE: error in send\n");
	                return -1;
		     }
		
		} else if (buf[0] == '0' && buf[1] == '3') { // DATA
		    fprintf(stderr,"EN DATA_TYPE\n");
            char data[TFTP_DATA_SIZE + 1];
            strcpy(data, buf + 4);
            data[TFTP_DATA_SIZE] = '\0';
            
            char nblock[3];
		    strncpy(nblock, buf + 2, 2);
		    nblock[2] = '\0';
		    
		    int n_block = atoi(nblock);
		    
		    /*
		     * Para el bloque X
		     *      1- Escribir DATA X
		     *      2- Mandar ACK (n = X)
		     */
		     
		     fprintf(stderr, "El tamaño es %ld\n", sizeof(data));
		     fprintf(stderr, "%s\n", data);
		    if (TFTP_DATA_SIZE == (sizeof(data) - 1)) { // DATA = 512     //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		        
		        if ((write_data_into_file(data, wfilename, nwrittenbytes)) == -1){
		            fprintf(stderr, "clientcp.c: DATA_TYPE: error in write_data_into_file\n");
			        return -1;
		        }
		        
		        
		        printf("Received block number %d\n", n_block);
		        
		        char *ack_msg = create_ack_msg(nblock);

		        nwrittenbytes += TFTP_DATA_SIZE;
		        
		        if (send(s, ack_msg, TAM_BUFFER, 0) != TAM_BUFFER){
				    printf("clientcp.c: DATA_TYPE: error in send\n");
			        return -1;
				}
		        
		    } else if (TFTP_DATA_SIZE > (sizeof(data) - 1)) { // DATA < 512   //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

		        if ((write_data_into_file(data, wfilename, nwrittenbytes)) == -1){
		            fprintf(stderr, "clientcp.c: DATA_TYPE: error in write_data_into_file\n");
			        return -1;
		        }
		            
		        printf("Received block number %d\n", n_block);

		        char *ack_msg = create_ack_msg(nblock);

		        nwrittenbytes += strlen(data);
		        
		        if (send(s, ack_msg, TAM_BUFFER, 0) != TAM_BUFFER){
				    fprintf(stderr, "clientcp.c: DATA_TYPE: error in send\n");
			        return -1;
				}
				
				eof_flag = 1;
				 
		    } else if (0 == sizeof(data)) { // DATA = 0
		    
		        printf("Received block number %d\n", n_block);
		        char *ack_msg = create_ack_msg(nblock);
		      
		        if (send(s, ack_msg, TAM_BUFFER, 0) != TAM_BUFFER){
			        printf("clientcp.c: DATA_TYPE: error in send\n");
		            return -1;
			    }
			    
			    eof_flag = 1;
			    
		    } 

		} else {
			printf("clientcp: default: Invalid type of message\n");
		    	return -1;
		}
		
		if(eof_flag)
		    break;
	}

    /* Print message indicating completion of task. */
	time(&timevar);
	printf("All done at %s", (char *) ctime(&timevar));
	return 0;
}
