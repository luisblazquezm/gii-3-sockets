/*
 *			C L I E N T U D P
 *
 *	This is an example program that demonstrates the use of
 *	sockets as an IPC mechanism.  This contains the client,
 *	and is intended to operate in conjunction with the server
 *	program.  Together, these two programs
 *	demonstrate many of the features of sockets, as well as good
 *	conventions for using these features.
 *
 *
 *	This program will request the internet address of a target
 *	host by name from the serving host.  The serving host
 *	will return the requested internet address as a response,
 *	and will return an address of all ones if it does not recognize
 *	the host name.
 *
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h> 
#include "tftp.h"

extern int errno;

#define ADDRNOTFOUND	0xffffffff	/* value returned for unknown host */
#define RETRIES	5		/* number of times to retry before givin up */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define PUERTO 6357
#define TIMEOUT 6
#define MAXHOST 512
#define TAM_BUFFER 1024

/*
 *			H A N D L E R
 *
 *	This routine is the signal handler for the alarm signal.
 */
void handler()
{
 printf("Alarma recibida \n");
}

/*
 *			M A I N
 *
 *	This routine is the client which requests service from the remote
 *	"example server".  It will send a message to the remote nameserver
 *	requesting the internet address corresponding to a given hostname.
 *	The server will look up the name, and return its internet address.
 *	The returned address will be written to stdout.
 *
 *	The name of the system to which the requests will be sent is given
 *	as the first parameter to the command.  The second parameter should
 *	be the the name of the target host for which the internet address
 *	is sought.
 */
int main(argc, argv)
int argc;
char *argv[];
{
	int i, errcode;
	int retry = RETRIES;		    /* holds the retry count */
    int s;				            /* socket descriptor */
    long timevar;                   /* contains time returned by time() */
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in servaddr_in;	/* for server socket address */
    struct in_addr reqaddr;		    /* for returned internet address */
    int	addrlen, n_retry;
    struct sigaction vec;
   	char hostname[MAXHOST];
   	struct addrinfo hints, *res;

    /* VARIABLES UTILIZADAS POR NOSOTROS */
	
	char buf[TAM_BUFFER];//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< No se si esta bien asi.
	int eof_flag = 0;
	char rfilename[] = "thisispdf.txt";     /* Esto luego será argv[4] */
	char filename[50];      /* Esto luego será argv[4] */
	//int nwrittenbytes = 0;                  /* Keeps the number of bytes written in a file */
	int nreadbytes = 0;                     /* Keeps the number of bytes read from file */
	char str[1000];
	short msg_type = 0;
	int tcp_mode = -1;                      /* Esto luego será argv[3] */
    int last_block = 0;
	//FILE *ptr = NULL;
	
	// Mensajes de emisión
	//rw_msg_t *rw_msg = NULL;
	//ack_msg_t *ack_msg = NULL;
	//data_msg_t *data_msg_rcv = NULL;
	
	// Mensajes de recepción
	//data_msg_t data_msg;
	//ack_msg_t ack_msg_rcv;


	if (argc == 1) { // clientudp localhost [r|w] filename
		fprintf(stderr, "Usage:  %s <remote host><[r|w] request>\n", argv[0]);
		exit(1);
	} else if (argc == 2) {
		fprintf(stderr, "Usage:  %s %s <[r|w] request>\n", argv[0], argv[1]);
		exit(1);
	} else if (argc == 4) {
	    if (!strcmp(argv[2], "r")) {
	        fprintf(stderr, "EL tcp_mode es %d\n", tcp_mode);
	        tcp_mode = READ_TYPE;
	    } else if (!strcmp(argv[2], "l")) {
		    tcp_mode = WRITE_TYPE;
		}
		    
		strncpy(filename, argv[3], sizeof(filename));
	}
	


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
    
	int numbytes;
    n_retry = RETRIES; // 5 intentos máximo 
    fprintf(stderr, "EL tcp_mode es %d\n", tcp_mode);
    
    if (tcp_mode == READ_TYPE){
    
    
        /*
         * 1. Crea peticion lectura
         * 2. Variables para ultimo mensaje y ultimo ack
         * 3. Enviar peticion lectura
         * 4. Abrir fichero en el que se copian los datos
         * 5. Recibir datos: Loop
         *     - recvfrom()
         *     - Añadir '\0'
         *     - Si (error) exit(1);
         *     - Si (mensaje = ultimo_mensaje) reenviar_ultimo_ack
         *     - Copiar(mensaje --> fichero)
         *     - Enviar ACK
        */
    
    	char *r_msg = create_r_msg(rfilename); //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< Esto luego tendra que ser filename
    	//char last_data_msg[TFTP_DATA_SIZE + 1];
        //strcpy(last_data_msg, "");
        
        //char last_ack_msg[10];
        //strcpy(last_ack_msg, r_msg);
        
        if (numbytes = sendto(s, r_msg, TAM_BUFFER, 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) ) {
                // Print nice message
        }
        
        printf("Client: sent %d bytes\n", numbytes);
        
        if (numbytes == -1) {
            perror("clientUDP: Write Request: sendto ACK 00");
            exit(1);
        }
        
        // Aqui hace un strcat <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        
        // DUPLICATE FILE, SENDING ERROR PACKET
        
        FILE *ptr;
        if ((ptr = fopen(filename, "w")) == NULL) {
            perror("clientUDP: fopen");
            // TODO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< Send error message
            exit(1);
        }
        
        int nbytes_written = 0;
        do {
            int addr_len = sizeof(servaddr_in);
            // RECEIVE DATA
            if (numbytes = recvfrom(s, buf, TAM_BUFFER-1, 0, (struct sockaddr *)&servaddr_in, &addr_len)) {
                //Print nice message
            }
            
            printf("Server: packet is %d bytes long\n", numbytes);
            buf[numbytes] = '\0';
            
            if (numbytes == -1 ) {
                perror("clientUDP: Write Request: recvfrom DATA");
                exit(1);
            }
            
            // SEND LAST ACK AGAIN IF NOT REACHED
            
            // WRITE FILE
            nbytes_written = strlen(buf + 4);
            if (-1 == fwrite(buf + 4, nbytes_written, 1, ptr)) {
                perror("clientUDP: Write Request: fwrite");
                exit(1);
            }
            //strcpy(last_data_msg, buf);
            printf("nbytes_written es %d\n", nbytes_written);
            
            // SEND ACK
            char nblock[3];
            strncpy(nblock, buf + 2, 2);
            nblock[2] = '\0';
            char *ack_msg = create_ack_msg(nblock);
            
            if (numbytes = sendto(s, ack_msg, strlen(ack_msg), 0, (struct sockaddr *)&servaddr_in, addr_len)) {
                // Print nice message
            }
            
            if (numbytes == -1) {
                perror("clientUDP: Read Request: sendto ACK");
                exit(1);
            }
            
            printf("Client: sent %d bytes\n", numbytes);
            //strcpy(last_ack_msg, ack_msg);
            
        } while (nbytes_written == TFTP_DATA_SIZE);
        
        printf("Client: new file %s successfully made\n", filename);
        fclose (ptr);
        
    } else if (tcp_mode == WRITE_TYPE) {
	    char *w_msg = create_w_msg(filename);
	    //char *last_data_msg;
	    int addr_len = sizeof(servaddr_in);
	    
        if (numbytes = sendto(s, w_msg, strlen(w_msg), 0, (struct sockaddr *)&servaddr_in, addr_len)){ //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< TAM_BUFFER
            // Print nice message
        }
        
        if (numbytes == -1 ) {
            perror("clientUDP: Write Request: sendto");
            exit(1);
        }
        
        
        int times;
        for (times = 0; times <= 5 ; times++) {
        
            if (times == 5){
                printf("Client: Max number of Tries %d reached \n", times);
                exit(1);
            }
            
            numbytes = wait_ack(s, buf, servaddr_in, addr_len);
            
            if (numbytes == -1) { // ERROR
                perror("clientUDP: recvfrom after check_timeout");
                exit(1);
            } else if (numbytes == -2) { // TIMEOUT
                printf("Client: Try number %d\n", times + 1);
                // TODO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< Send error message
                //int temp_bytes = 1;
                //printf("Client: sent %d bytes again \n", temp_bytes);
                continue;
            } else { // MESSAGE RECEIVED
                break;
            }
        }
        
        buf[numbytes] = '\0';
        
        if (buf[0] == '0' && buf[1] == '4') { // ACK
            
            FILE *ptr = NULL;
            char filename[128];
            strcpy(filename, buf + 2);
            if ((ptr = fopen(rfilename, "r")) == NULL) {
                perror("clientUDP: fopen");
                // TODO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< Send error message
                exit(1);
            }

            // Look for file size to see how much to read
            int bytes_left, numbytes;
            fseek(ptr, 0, SEEK_END);
            bytes_left = ftell(ptr);
            fseek(ptr, 0, SEEK_SET);
            // Missing something here?
            
            int nblock = 1;
            while (bytes_left > 0) {
            
                char read_data[TFTP_DATA_SIZE + 1];
            
                if (bytes_left > TFTP_DATA_SIZE) {
                    if (1 != fread(read_data, TFTP_DATA_SIZE, 1, ptr)){
                        // Print nice message
                    }
                    read_data[TFTP_DATA_SIZE] = '\0';
                    bytes_left -= TFTP_DATA_SIZE;
                    
                } else {
                    if (1 != fread(read_data, bytes_left, 1, ptr)){
                        // Print nice message
                    }
                    read_data[bytes_left] = '\0';
                    bytes_left = 0;
                }
                
                char *data_msg = create_data_msg(nblock, read_data);
                
                if (numbytes = sendto(s, data_msg, strlen(data_msg), 0, (struct sockaddr *)&servaddr_in, addr_len)){
                    // Print nice message
                }
                
                if (numbytes == -1 ) {
                    perror("clientUDP: Write: Data msg: sendto");
                    exit(1);
                }
                
                //last_data_msg = data_msg;
                
                int times;
                for (times = 0; times <= 5 ; times++) {
                    if (times == 5){
                        printf("Client: Max number of Tries %d reached \n", times);
                        exit(1);
                    }
                    
                    numbytes = wait_ack(s, buf, servaddr_in, addr_len);
                    
                    if (numbytes == -1) { // ERROR
                        perror("clientUDP: recvfrom after check_timeout");
                        exit(1);
                    } else if (numbytes == -2) { // TIMEOUT
                        printf("Client: Nº %d of try \n", times + 1);
                        // TODO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< Send error message
                        //int temp_bytes = 1;
                        //printf("Client: sent %d bytes again \n", temp_bytes);
                        continue;
                    } else { // MESSAGE RECEIVED
                        break;
                    }
                }
                
                buf[numbytes] = '\0';
                
                // Check if error msg
                
                ++nblock;
                // If nblock gets higher than 99 this will crash :D
                
            }
            
            fclose(ptr);
            
        } else {
            fprintf(stderr, "clientUDP: ACK : Expecting but got : %s\n", buf);
            exit(1);
        }
            
	} else {
	    fprintf(stderr, "clientUDP: Invalid message type\n");
        exit(1);
	}

    /*
    memcpy((void *)buf, (const void *)rw_msg, sizeof(*rw_msg));
    printf("Enviando el fichero %s\n", filename);
	while (n_retry > 0) {
	
	    fprintf(stderr, "Enviando sendto\n");

        if (sendto (s, buf, TAM_BUFFER, 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
            perror(argv[0]);
            fprintf(stderr, "%s: unable to send request\n", argv[0]);
            exit(1);
        }
        fprintf(stderr, "EOF flag es %d\n", eof_flag);
        if(eof_flag)
            break; 
        
        alarm(TIMEOUT); // 6 milisegundos

        if (recvfrom (s, buf, TAM_BUFFER, 0, (struct sockaddr *)&servaddr_in, &addrlen) == -1) {
            if (errno == EINTR) {

                printf("attempt %d (retries %d).\n", n_retry, RETRIES);
                n_retry--; 
            } else {
                printf("Unable to get response from");
                exit(1); 
            }

        } else {
                    
            alarm(0);

            
            if (reqaddr.s_addr == ADDRNOTFOUND) {
                printf("Host %s unknown by nameserver %s\n", argv[2], argv[1]);
            } else {
                if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
                    perror(" inet_ntop \n");
                    printf("Address for %s is %s\n", argv[2], hostname);
            }
            
            

            msg_type = *buf;
		
            switch(msg_type) {
                case ACK_TYPE:
                    memcpy((void *)&ack_msg_rcv, (const void *)&buf, sizeof(ack_msg));		
                    fprintf(stderr, "Last_block ANTES era %d y nreadbytes %d\n", last_block, nreadbytes);
                    if (0 != last_block && last_block == ack_msg_rcv.n_block){
                        fprintf(stderr,"\nReached end of file\n\n");
                        printf("All done at %s", (char *) ctime(&timevar));
                        return 0;
                    }
                    

                    data_msg_rcv = create_data_msg(ack_msg_rcv.n_block + 1);

                    if ((last_block = read_from_file(data_msg_rcv, rfilename, nreadbytes)) == -1){
                        fprintf(stderr, "clientudp.c: DATA_TYPE: error in read_from_file\n");
                        return -1;
                    }
                    
                    fprintf(stderr, "Last_block este es %d\n", last_block);
                    printf("Received ACK number %d\n", ack_msg_rcv.n_block);
                    memcpy((void *)buf, (const void *)data_msg_rcv, sizeof(*data_msg_rcv));
                    nreadbytes += TFTP_DATA_SIZE;


                break;
                case DATA_TYPE:
                    fprintf(stderr,"EN DATA_TYPE\n");
                    memcpy((void *)&data_msg, (const void *)&buf, sizeof(data_msg));

                    fprintf(stderr, "El tamaño de esta huea es %ld\n", strlen(data_msg.data));
                    if (TFTP_DATA_SIZE == strlen(data_msg.data)) { // DATA = 512     
                    fprintf(stderr, "Estoy en igual a 512\n");
                        if ((write_data_into_file(data_msg, wfilename, nwrittenbytes)) == -1){
                            fprintf(stderr, "clientudp.c: DATA_TYPE: error in write_data_into_file\n");
                            return -1;
                        }

                        printf("Received block number %d\n", data_msg.n_block);
                        ack_msg = create_ack_msg(data_msg.n_block);
                        memcpy((void *)buf, (const void *)ack_msg, sizeof(*ack_msg));
                        nwrittenbytes += TFTP_DATA_SIZE;
                        
                    } else if (TFTP_DATA_SIZE > strlen(data_msg.data)) { // DATA < 512  
                    fprintf(stderr, "Estoy en menor que 512\n");
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
                    fprintf(stderr, "Estoy en igual a 0\n");
                        printf("Received block number %d\n", data_msg.n_block);
                        ack_msg = create_ack_msg(data_msg.n_block);
                        memcpy((void *)buf, (const void *)ack_msg, sizeof(*ack_msg));

                        eof_flag = 1;
                    } 

                break;
                case ERROR_TYPE:


                break;
                default:
                    printf("clientudp: default: Invalid type of message\n");
                    return -1;
                break;
                
            } 
	    
               
     }
  
  } 
*/
  if (n_retry == 0) {
     printf("Unable to get response from");
     printf(" %s after %d attempts.\n", argv[1], RETRIES);
  }
  
  printf("All done at %s", (char *) ctime(&timevar));//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< I included this
  return 0;
  
}
