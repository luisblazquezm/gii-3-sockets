#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h> 
#include "tftp.h"

/* TCP */
#define PUERTO 10460 //17278
#define TAM_BUFFER 1024

/* UDP */
#define ADDRNOTFOUND	0xffffffff	/* value returned for unknown host */
#define RETRIES	5		/* number of times to retry before givin up */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define PUERTO 17278
#define TIMEOUT 6
#define MAXHOST 512
#define TAM_BUFFER 1024

int serverTCP(char *filename, int file_mode);
int serverUDP(char *filename, int file_mode);

int
main(int argc, char *argv[]) {
    int protocol_mode = 0;
    int file_mode = 0;
    char filename[100];
    
    if (argc == 1) { // cliente localhost [TCP|UDP] [r|w] filename
		fprintf(stderr, "Usage:  %s <remote host><[r|w] request>\n", argv[0]);
		exit(1);
	} else if (argc == 2) {
		fprintf(stderr, "Usage:  %s %s <[r|w] request>\n", argv[0], argv[1]);
		exit(1);
	} else if (argc == 5) {
        if (strcmp(argv[3], "r"))
	        file_mode = READ_TYPE;
	    else if (strcmp(argv[3], "l"))
		    file_mode = WRITE_TYPE;
		    
		if (strcmp(argv[2], "TCP"))
	        protocol_mode = TCP_TYPE;
	    else if (strcmp(argv[2], "UDP"))
		    protocol_mode = UDP_TYPE;
		    
		strncpy(filename,argv[3], sizeof(filename));
	}
	
	if (TCP_TYPE == protocol_mode) serverTCP(filename, file_mode);
	else if (UDP_TYPE == protocol_mode) serverUDP(filename, file_mode);
	
	return 0;
}


int serverTCP(char *filename, int file_mode)
{
    /* VARIABLES UTILIZADAS DADAS POR ELLOS */

   	struct addrinfo hints, *res;
    struct sockaddr_in myaddr_in;	            /* for local socket address */
    struct sockaddr_in servaddr_in;	            /* for server socket address */
    int s;				                        /* connected socket descriptor */
    long timevar;			                    /* contains time returned by time() */
	int addrlen, i, j, errcode;
	char buf[TAM_BUFFER];                       /* This example uses TAM_BUFFER byte messages. */
	
	
	/* VARIABLES UTILIZADAS POR NOSOTROS */
	
	int eof_flag;
	char rfilename[] = "thisispdf.txt";         
	char wfilename[100];     
	int nwrittenbytes = 0;                      /* Keeps the number of bytes written in a file */
	int nreadbytes = 0;                         /* Keeps the number of bytes read from file */
	char str[1000];
	short msg_type = 0;
	int tcp_mode = -1;                          
    int last_block = -1;
	FILE *ptr = NULL;
	
	// Mensajes de emisión
	rw_msg_t *rw_msg = NULL;
	ack_msg_t *ack_msg = NULL;
	data_msg_t *data_msg_rcv = NULL;
	
	// Mensajes de recepción
	data_msg_t data_msg;
	ack_msg_t ack_msg_rcv;

	strncpy(wfilename, filename, sizeof(wfilename));
	tcp_mode = file_mode;
	
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
            if (TFTP_DATA_SIZE == sizeof(data_msg.data)) { // DATA = 512     
                
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
                
            } else if (TFTP_DATA_SIZE > sizeof(data_msg.data)) { // DATA < 512  

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
		         
            } else if (0 == sizeof(data_msg.data)) { // DATA = 0
            
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

int serverUDP(char *filename, int file_mode)
{
    struct sockaddr_in myaddr_in;	    /* for local socket address */
    struct sockaddr_in servaddr_in;	    /* for server socket address */
    struct in_addr reqaddr;		        /* for returned internet address */
    struct sigaction vec;
    struct addrinfo hints, *res;
    int i, errcode;
	int retry = RETRIES;		        /* holds the retry count */
    int s;				                /* socket descriptor */
    long timevar;                       /* contains time returned by time() */
    int	addrlen, n_retry;
   	char hostname[MAXHOST];


    /* VARIABLES UTILIZADAS POR NOSOTROS */
	
	char buf[TAM_BUFFER];//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< No se si esta bien asi.
	int eof_flag = 0;
	char rfilename[] = "thisispdf.txt";     
	char wfilename[100]; //"fichero1.txt";      
	int nwrittenbytes = 0;                  /* Keeps the number of bytes written in a file */
	int nreadbytes = 0;                     /* Keeps the number of bytes read from file */
	char str[1000];
	short msg_type = 0;
	int tcp_mode = -1;                      /* Esto luego será argv[3] */
    int last_block = 0;
	FILE *ptr = NULL;
	
	// Mensajes de emisión
	rw_msg_t *rw_msg = NULL;
	ack_msg_t *ack_msg = NULL;
	data_msg_t *data_msg_rcv = NULL;
	
	// Mensajes de recepción
	data_msg_t data_msg;
	ack_msg_t ack_msg_rcv;

    strncpy(wfilename, filename, sizeof(wfilename));
	tcp_mode = file_mode;
	
    /* Create the socket. */
	s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
		exit(1);
	}
	
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
	
	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_port = 0;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind socket\n", argv[0]);
		exit(1);
	}
	
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
            perror(argv[0]);
            fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
            exit(1);
    }

    time(&timevar);

    printf("Connected to %s on port %u at %s", argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

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

    vec.sa_handler = (void *) handler;
    vec.sa_flags = 0;
    if (sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
            perror(" sigaction(SIGALRM)");
            fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
            exit(1);
    }
	
    n_retry=RETRIES; // 5 intentos máximo 
    
    if (tcp_mode == READ_TYPE){
    	rw_msg = create_rw_msg(READ_TYPE, rfilename);
    } else if (tcp_mode == WRITE_TYPE) {
	    rw_msg = create_rw_msg(WRITE_TYPE, wfilename);
	}

    memcpy((void *)buf, (const void *)rw_msg, sizeof(*rw_msg));
    printf("Enviando el fichero %s\n", rw_msg->filename);
    
	while (n_retry > 0) {
	    fprintf(stderr, "Enviando sendto\n");

        if (sendto (s, buf, TAM_BUFFER, 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
            perror(argv[0]);
            fprintf(stderr, "%s: unable to send request\n", argv[0]);
            exit(1);
        }
        
        /* Set up a timeout so I don't hang in case the packet
        * gets lost.  After all, UDP does not guarantee
        * delivery.
        */
        alarm(TIMEOUT); // 6 milisegundos

        /* Wait for the reply to come in. */
        if (recvfrom (s, buf, TAM_BUFFER, 0, (struct sockaddr *)&servaddr_in, &addrlen) == -1) {
            if (errno == EINTR) {
                /* Alarm went off and aborted the receive.
                * Need to retry the request if we have
                * not already exceeded the retry limit.
                */
                printf("attempt %d (retries %d).\n", n_retry, RETRIES);
                n_retry--; 
            } else {
                printf("Unable to get response from");
                exit(1); 
            }

        } else {
                    /*
            alarm(0);

            
            if (reqaddr.s_addr == ADDRNOTFOUND) {
                printf("Host %s unknown by nameserver %s\n", argv[2], argv[1]);
            } else {
                if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
                    perror(" inet_ntop \n");
                    printf("Address for %s is %s\n", argv[2], hostname);
            }
            */
            
            /* Separar los tipos de mensajes */       
            msg_type = *buf;
		
            switch(msg_type) {
                case ACK_TYPE:
                    memcpy((void *)&ack_msg_rcv, (const void *)&buf, sizeof(ack_msg));		
                    fprintf(stderr, "El block este ANTES era %d y nreadbytes %d\n", last_block, nreadbytes);
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
                        fprintf(stderr, "clientudp.c: DATA_TYPE: error in read_from_file\n");
                        return -1;
                    }
                    
                    fprintf(stderr, "El block este es %d\n", last_block);
                    printf("Received ACK number %d\n", ack_msg_rcv.n_block);
                    memcpy((void *)buf, (const void *)data_msg_rcv, sizeof(*data_msg_rcv));
                    nreadbytes += TFTP_DATA_SIZE;

                break;
                case DATA_TYPE:
                    fprintf(stderr,"EN DATA_TYPE\n");
                    memcpy((void *)&data_msg, (const void *)&buf, sizeof(data_msg));

                    /*
                    * Para el bloque X
                    *      1- Escribir DATA X
                    *      2- Mandar ACK (n = X)
                    */
                    fprintf(stderr, "El tamaño de esta huea es %ld\n", strlen(data_msg.data));
                    if (TFTP_DATA_SIZE == strlen(data_msg.data)) { // DATA = 512     

                        if ((write_data_into_file(data_msg, wfilename, nwrittenbytes)) == -1){
                            fprintf(stderr, "clientudp.c: DATA_TYPE: error in write_data_into_file\n");
                            return -1;
                        }

                        printf("Received block number %d\n", data_msg.n_block);
                        ack_msg = create_ack_msg(data_msg.n_block);
                        memcpy((void *)buf, (const void *)ack_msg, sizeof(*ack_msg));
                        nwrittenbytes += TFTP_DATA_SIZE;

                    } else if (TFTP_DATA_SIZE > strlen(data_msg.data)) { // DATA < 512  

                        if ((write_data_into_file(data_msg, wfilename, nwrittenbytes)) == -1){
                            fprintf(stderr, "clientudp.c: DATA_TYPE: error in write_data_into_file\n");
                            return -1;
                        }

                        printf("Received block number %d\n", data_msg.n_block);

                        ack_msg = create_ack_msg(data_msg.n_block);
                        memcpy((void *)buf, (const void *)ack_msg, sizeof(*ack_msg));
                        nwrittenbytes += strlen(data_msg.data);
                        
                        eof_flag = 1;

                    } else if (0 == strlen(data_msg.data)) { // DATA = 0

                        printf("Received block number %d\n", data_msg.n_block);
                        ack_msg = create_ack_msg(data_msg.n_block);
                        memcpy((void *)buf, (const void *)ack_msg, sizeof(*ack_msg));

                        eof_flag = 1;
                    } 

                break;
                case ERROR_TYPE:

                        /*
                        * Basicamente, finalizar la conexion
                        */

                break;
                default:
                    printf("clientudp: default: Invalid type of message\n");
                    return -1;
                break;
                
            } /* End of switch-case*/
	    
            if(eof_flag)
                break;    
     }/* FIN if-else del recvfrom */
  
  } /* FIn del while (numero de intentos > 0)*/

  if (n_retry == 0) {
     printf("Unable to get response from");
     printf(" %s after %d attempts.\n", argv[1], RETRIES);
  }
  
}


