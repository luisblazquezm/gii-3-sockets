/*
** Fichero: cliente.c
** Autores:
** Luis Blázquez Miñambres DNI 70910465Q
** Samuel Gómez Sánchez    DNI 45136357F
*/

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
#include "utils.h"

#define ADDRNOTFOUND	0xffffffff	/* value returned for unknown host */
#define RETRIES	5		/* number of times to retry before givin up */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define PUERTO 6357
#define TIMEOUT 6
#define MAXHOST 512
#define TAM_BUFFER 1024

int tcp_client(int argc, char *argv[]);
int udp_client(int argc, char *argv[]);


/*
 *			H A N D L E R
 *
 *	This routine is the signal handler for the alarm signal.
 */
void handler()
{
printf("Alarma recibida \n");
}

int
main(int argc, char *argv[]) {

    if (test_args(argc, argv)) return -1;
    
    // What if defunct children hahaha xd
    
    if (!strcmp(argv[2], "TCP")) {
        return tcp_client(argc, argv);
    } else {
        return udp_client(argc, argv);
    }
    
	return 0;
}


int tcp_client(int argc, char *argv[]) // We could have used execve, but let's not get fancy here
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
	char client_output_file[10];
	char temp_buf[1000];
	int eof_flag;
	char filename[50]; /* Esto luego será argv[4] */
	int nwrittenbytes = 0; /* Keeps the number of bytes written in a file */
	int nreadbytes = 0; /* Keeps the number of bytes read from file */
	char str[1000];
	short msg_type = 0;
	int tcp_mode = -1; /* Esto luego será argv[3] */
    char last_block[3] = "**";
    int lastblock_reached = 0;
	FILE *ptr = NULL;

	// Get args
	strncpy(filename, argv[4], sizeof(filename));
	if (!strcmp(argv[3], "r"))
        tcp_mode = READ_TYPE;
    else if (!strcmp(argv[3], "w"))
	    tcp_mode = WRITE_TYPE;
	
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
        fprintf(stderr, "%s: No es posible resolver la IP de %s\n", argv[0], argv[1]);
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

    fprintf(stderr, "Connected to %s on port %u at %s\n",
			argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));
			
	// CREATE LOG FILE FOR CLIENT TCP
	strcpy(client_output_file, client_log_filename(ntohs(myaddr_in.sin_port)));

    /* R/W REQUEST CLIENT ---> SERVER */
    
    eof_flag = 0;
    
    if (tcp_mode == READ_TYPE){
    
    	char *r_msg = create_r_msg(filename);
	    if (send(s, r_msg, TAM_BUFFER, 0) != TAM_BUFFER) {
	        snprintf(temp_buf, sizeof(temp_buf), "clientcp: error on read request (send no. %d). Connection aborted.", i);
	        printmtof(temp_buf, client_output_file);
		    exit(1);
	    }
	    
	    snprintf(temp_buf, sizeof(temp_buf), "clientcp: read: going to read file '%s'", filename);
	    printmtof(temp_buf, client_output_file);
	    
    } else if (tcp_mode == WRITE_TYPE) {
	    char *w_msg = create_w_msg(filename);
	    if (send(s, w_msg, TAM_BUFFER, 0) != TAM_BUFFER) {
	        snprintf(temp_buf, sizeof(temp_buf), "clientcp: error on write request (send no. %d). Connection aborted.", i);
	        printmtof(temp_buf, client_output_file);
		    exit(1);
	    }
	    
	    snprintf(temp_buf, sizeof(temp_buf), "clientcp: going to write file '%s'", filename);
	    printmtof(temp_buf, client_output_file);
    }
    
    
    /* RECEPTION CLIENT <--- SERVER */
	char data_buf[1000];   
	while (i = recv(s, buf, TAM_BUFFER , 0)) {
	
		if (i == -1) {
            perror(argv[0]);
	        printmtof("clientcp: error reading result", client_output_file);
			exit(1);
		}

		while (i < TAM_BUFFER) {
			j = recv(s, &buf[i], TAM_BUFFER-i, 0);
			if (j == -1) {
                 perror(argv[0]);
	             printmtof("clientcp: error reading result", client_output_file);
			     exit(1);
            }
			i += j;
		}

		if (buf[0] == '0' && buf[1] == '4') { //ACK

		    /*
		     * Para el ACK X:
		     *      1- Enviar bloque X + 1
		     */
		     
		    char nblock[3];
		    nblock[0] = buf[2], nblock[1] = buf[3], nblock[2] = '\0';
		    
		    snprintf(temp_buf, sizeof(temp_buf), "clientcp: ack: received ACK number %s", nblock);
	        printmtof(temp_buf, client_output_file);

		    if (('*' != last_block[0] && '*' != last_block[1])
		    && (nblock[0] == last_block[0] && nblock[1] == last_block[1])){
	            printmtof("clientcp: ack: done reading source file", client_output_file);
			    eof_flag = 1;
			    break;
		    }
		     
		     client_get_filepath(filename, temp_buf);
		     if ((lastblock_reached = read_from_file((char *)data_buf, temp_buf, nreadbytes)) == -1){
		        printmtof("clientcp: ack: error in read_from_file", client_output_file);
		        return -1;
		     }
		     
             inc_nblock(nblock);

		     char *data_msg = create_data_msg(nblock, data_buf);

		     nreadbytes += TFTP_DATA_SIZE;

		     if (send(s, data_msg, TAM_BUFFER, 0) != TAM_BUFFER){
		        printmtof("clientcp: ack: error in send", client_output_file);
	            return -1;
		     }
		     
		     if (lastblock_reached){
		     
		        strcpy(last_block, nblock);
		        snprintf(temp_buf, sizeof(temp_buf), "clientcp: ack: sent last data block (nblock: %s)", last_block);
	            printmtof(temp_buf, client_output_file);
		        
		     } else {
		        snprintf(temp_buf, sizeof(temp_buf), "clientcp: ack: sent data block (nblock: %s)", nblock);
	            printmtof(temp_buf, client_output_file);
		     }
		     
		} else if (buf[0] == '0' && buf[1] == '3') { // DATA
		    /*
		     * Para el bloque X
		     *      1- Escribir DATA X
		     *      2- Mandar ACK (n = X)
		     */
		    
            char data[TFTP_DATA_SIZE + 1];
            strcpy(data, buf + 4);
            
            char nblock[3];
		    nblock[0] = buf[2], nblock[1] = buf[3], nblock[2] = '\0';
		    
		    snprintf(temp_buf, sizeof(temp_buf), "clientcp: data: received data block (nblock: %s, size: %d)", nblock, strlen(data));
	        printmtof(temp_buf, client_output_file);
		     
		    if (0 == strlen(data)) { // DATA = 0
		    
		        char *ack_msg = create_ack_msg(nblock);
		      
		        if (send(s, ack_msg, TAM_BUFFER, 0) != TAM_BUFFER){
		            printmtof("clientcp: data: error in send", client_output_file);
		            return -1;
			    }
			    
			    snprintf(temp_buf, sizeof(temp_buf), "clientcp: data: sent last ack (nblock: %s)", nblock);
	            printmtof(temp_buf, client_output_file);
			    
			    eof_flag = 1;
			    
		    } else if (TFTP_DATA_SIZE > strlen(data)) { // DATA < 512

                client_get_filepath(filename, temp_buf);
		        if ((write_data_into_file(data, temp_buf, nwrittenbytes)) == -1){
		            printmtof("clientcp: data: error in write_data_into_file", client_output_file);
			        return -1;
		        }

		        char *ack_msg = create_ack_msg(nblock);

		        nwrittenbytes += strlen(data);
		        
		        if (send(s, ack_msg, TAM_BUFFER, 0) != TAM_BUFFER){
		            printmtof("clientcp: data: error in send", client_output_file);
			        return -1;
				}
				
				snprintf(temp_buf, sizeof(temp_buf), "clientcp: data: sent last ack (nblock: %s)", nblock);
	            printmtof(temp_buf, client_output_file);
				
				eof_flag = 1;
				 
		    } else if (TFTP_DATA_SIZE == strlen(data)) { // DATA = 512
		        
		        client_get_filepath(filename, temp_buf);
		        if ((write_data_into_file(data, temp_buf, nwrittenbytes)) == -1){
		            printmtof("clientcp: data: error in write_data_into_file", client_output_file);
			        return -1;
		        }
		        
		        char *ack_msg = create_ack_msg(nblock);

		        nwrittenbytes += TFTP_DATA_SIZE;
		        
		        if (send(s, ack_msg, TAM_BUFFER, 0) != TAM_BUFFER){
		            printmtof("clientcp: data: error in send", client_output_file);
			        return -1;
				}
				
				snprintf(temp_buf, sizeof(temp_buf), "clientcp: data: sent ack (nblock: %s)", nblock);
	            printmtof(temp_buf, client_output_file);
		    }

		} else {
		    printmtof("clientcp: invalid type of message", client_output_file);
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
int udp_client(int argc, char *argv[]) // We could have used execve, but let's not get fancy here
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
	char client_output_file[10];
	char temp_buf[1000];
	char buf[TAM_BUFFER];
	int eof_flag = 0;
	char filename[50];      /* Esto luego será argv[3] */
	//int nwrittenbytes = 0;                  /* Keeps the number of bytes written in a file */
	int nreadbytes = 0;                     /* Keeps the number of bytes read from file */
	char str[1000];
	short msg_type = 0;
	int tcp_mode = -1;                      /* Esto luego será argv[3] */
    int last_block = 0;
	
	// Get args
	strncpy(filename, argv[4], sizeof(filename));
	if (!strcmp(argv[3], "r"))
        tcp_mode = READ_TYPE;
    else if (!strcmp(argv[3], "w"))
	    tcp_mode = WRITE_TYPE;
	
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
    
	// CREATE LOG FILE FOR CLIENT TCP
    strcpy(client_output_file, client_log_filename(ntohs(myaddr_in.sin_port)));
    
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
    
    snprintf(temp_buf, sizeof(temp_buf), "clientudp: tcp_mode is %d", tcp_mode);
	printmtof(temp_buf, client_output_file);
    
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
    
    	char *r_msg = create_r_msg(filename);
    	
    	char last_data_msg[TFTP_DATA_SIZE + 1];
        strcpy(last_data_msg, "");
        char last_ack_msg[10];
        strcpy(last_ack_msg, r_msg);
        
        numbytes = sendto(s, r_msg, TAM_BUFFER, 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in));
        if (numbytes == -1) {
            perror("clientudp: read: error sending read request");
            exit(1);
        }
        
        snprintf(temp_buf, sizeof(temp_buf), "clientudp: going to read file '%s'", filename);
	    printmtof(temp_buf, client_output_file);
        
        // Aqui hace un strcat <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        
        // DUPLICATE FILE, SENDING ERROR PACKET
        /*
        
        client_get_filepath(filename, temp_buf);
        if (access(temp_buf, F_OK) != -1) { // DUPLICATE FILE
            snprintf(temp_buf, sizeof(temp_buf), "clientudp: file '%s' already exists, sending error packet and aborting...", filename);
            printmtof(temp_buf, debug_file);
            char *e_msg = create_error_msg("06", "FILE ALREADY EXISTS");
            sendto(s, e_msg, strlen(e_msg), 0, (struct sockaddr *)&servaddr_in, addrlen);
            printmtof("Connection aborted", client_output_file);
            exit(1);
        }
        */
        client_get_filepath(filename, temp_buf);
        FILE *ptr;
        if ((ptr = fopen(temp_buf, "w")) == NULL) { // ACCESS DENIED
            snprintf(temp_buf, sizeof(temp_buf), "clientudp: read: error creating file '%s'. Operation aborted.", filename);
            printmtof(temp_buf, client_output_file);
            exit(1);
        }
        
        int nbytes_written = 0;
        do {
            int addr_len = sizeof(servaddr_in);
            
            // RECEIVE DATA
            numbytes = recvfrom(s, buf, TAM_BUFFER - 1, 0, (struct sockaddr *)&servaddr_in, &addr_len);
            if (numbytes == -1 ) {
                perror("clientudp: read: error receiving data");
                exit(1);
            }
            
            buf[numbytes] = '\0';
            
            // CHECKING IF ERROR PACKET
            if(buf[0] == '0' && buf[1] == '5'){
                snprintf(temp_buf, sizeof(temp_buf), "clientudp: read: got error packet: \n\t%s", buf);
	            printmtof(temp_buf, client_output_file);
				exit(1);
            }
            
            // SEND LAST ACK AGAIN
            if(!strcmp(buf, last_data_msg)){
				sendto(s, last_data_msg, strlen(last_data_msg), 0, (struct sockaddr *)&servaddr_in, addrlen);
				continue;
            }
            
            // IF NOT ERROR, DATA. PRINT NICE MESSAGE
            char nblock[3];
		    nblock[0] = buf[2], nblock[1] = buf[3], nblock[2] = '\0';
		    
            snprintf(temp_buf, sizeof(temp_buf), "clientudp: read: received data block (nblock: %s, size: %d)", nblock, strlen(buf + 4));
	        printmtof(temp_buf, client_output_file);
            
            // WRITE FILE
            nbytes_written = strlen(buf + 4);
            if (-1 == fwrite(buf + 4, nbytes_written, 1, ptr)) {
                perror("clientudp: read: error on fwrite");
                exit(1);
            }
            
            // SEND ACK
            char *ack_msg = create_ack_msg(nblock);
            numbytes = sendto(s, ack_msg, strlen(ack_msg), 0, (struct sockaddr *)&servaddr_in, addr_len);
            if (numbytes == -1) {
                perror("clientudp: read: ack: error on sendto");
                exit(1);
            }
            
            snprintf(temp_buf, sizeof(temp_buf), "clientudp: read: sent ack (nblock: %s)", nblock);
	        printmtof(temp_buf, client_output_file);
            strcpy(last_ack_msg, ack_msg); // SAVE LAST ACK FOR RESENDING
            
        } while (nbytes_written == TFTP_DATA_SIZE);
        
        snprintf(temp_buf, sizeof(temp_buf), "clientudp: new file %s successfully made\n", filename);
	    printmtof(temp_buf, client_output_file);
        fclose (ptr);
        
    } else if (tcp_mode == WRITE_TYPE) {
    
	    char *w_msg = create_w_msg(filename);
	    char last_data_msg[TFTP_DATA_SIZE + 1];
	    int addr_len = sizeof(servaddr_in);
	    
        numbytes = sendto(s, w_msg, strlen(w_msg), 0, (struct sockaddr *)&servaddr_in, addr_len);
        if (numbytes == -1 ) {
            perror("clientudp: write: error sending write request");
            exit(1);
        }
        strcpy(last_data_msg, w_msg);
        
        snprintf(temp_buf, sizeof(temp_buf), "clientudp: write: going to write file '%s'", filename);
	    printmtof(temp_buf, client_output_file);
        
        int times;
        for (times = 0; times <= 5; times++) {
        
            if (times == 5){
                snprintf(temp_buf, sizeof(temp_buf), "clientudp: write: data: max number of tries %d reached", times);
	            printmtof(temp_buf, client_output_file);
                exit(1);
            }

            numbytes = wait_ack(s, buf, &servaddr_in, addr_len);
	        printmtof("clientudp: write: received first ACK", client_output_file);
            
            if (numbytes == -1) { // ERROR
                perror("clientudp: recvfrom after check_timeout");
                exit(1);
            } else if (numbytes == -2) { // TIMEOUT
            
                snprintf(temp_buf, sizeof(temp_buf), "clientudp: try no. %d. Trying again...", times + 1);
                printmtof(temp_buf, client_output_file);
                
                int bytes = sendto(s, last_data_msg, strlen(last_data_msg), 0, (struct sockaddr *)&servaddr_in, addrlen);
                if (bytes == -1) {
                    perror("clientudp: timeout: sendto");
                    exit(1);
                } else {
                    printmtof("clientudp: sent last data block again", client_output_file);
                }
                
                continue;
            } else { // MESSAGE RECEIVED
                break;
            }
        }
        
        buf[numbytes] = '\0';
        
        if (buf[0] == '0' && buf[1] == '4') { // ACK
        
	        printmtof("clientudp: write: ack: client received ACK", client_output_file);
            
            client_get_filepath(filename, temp_buf);
            FILE *ptr = NULL;
            if ((ptr = fopen(temp_buf, "r")) == NULL || access(temp_buf, F_OK) == -1) { // FILE NOT FOUND
                snprintf(temp_buf, sizeof(temp_buf), "clientudp: read: file '%s' does not exist. Operation aborted.", filename);
                printmtof(temp_buf, client_output_file);
                exit(1);
            }

            // Look for file size to see how much to read
            int bytes_left, numbytes;
            fseek(ptr, 0, SEEK_END);
            bytes_left = ftell(ptr);
            fseek(ptr, 0, SEEK_SET);
            if (bytes_left == 0)
                ++bytes_left;
            else if (bytes_left%TFTP_DATA_SIZE == 0)
                --bytes_left;
           
            addr_len = sizeof(servaddr_in);
            char nblock[3] = "01";
            while (bytes_left > 0) {
            
                char read_data[TFTP_DATA_SIZE + 1];
            
                if (bytes_left > TFTP_DATA_SIZE) {
                    if (1 != fread(read_data, TFTP_DATA_SIZE, 1, ptr)){
                        perror("clientudp: write: ack: error reading 512 bytes");
                        exit(1);
                    }
                    read_data[TFTP_DATA_SIZE] = '\0';
                    bytes_left -= TFTP_DATA_SIZE;
                    
                } else {
                    if (1 != fread(read_data, bytes_left, 1, ptr)){
                        perror("clientudp: write: ack: error reading bytes left");
                        exit(1);
                    }
                    read_data[bytes_left] = '\0';
                    bytes_left = 0;
                }
                
                char *data_msg = create_data_msg(nblock, read_data);
            
                numbytes = sendto(s, data_msg, strlen(data_msg), 0, (struct sockaddr *)&servaddr_in, addr_len);
                if (numbytes == -1 ) {
                    perror("clientudp: write: data: error sending data");
                    exit(1);
                } else {
                    snprintf(temp_buf, sizeof(temp_buf), "clientudp: write: data: sent data block (nblock: %s)", nblock);
	                printmtof(temp_buf, client_output_file);
                }
                
                //last_data_msg = data_msg;
                
                int times;
                for (times = 0; times <= 5 ; times++) {
                    if (times == 5){
                        snprintf(temp_buf, sizeof(temp_buf), "clientudp: write: ack: max number of tries %d reached", times);
	                    printmtof(temp_buf, client_output_file);
                        exit(1);
                    }
                    
                    numbytes = wait_ack(s, buf, &servaddr_in, addr_len);
	                printmtof("clientudp: write: ack: received ACK", client_output_file);
	                
                    if (numbytes == -1) { // ERROR
                        perror("clientudp: write: ack: recvfrom after check_timeout");
                        exit(1);
                    } else if (numbytes == -2) { // TIMEOUT
                        snprintf(temp_buf, sizeof(temp_buf), "clientudp: try no. %d. Trying again...", times + 1);
                        printmtof(temp_buf, client_output_file);
                        
                        int bytes = sendto(s, data_msg, strlen(data_msg), 0, (struct sockaddr *)&servaddr_in, addrlen);
                        if (bytes == -1) {
                            perror("clientudp: timeout: sendto");
                            exit(1);
                        } else {
                            snprintf(temp_buf, sizeof(temp_buf), "clientudp: sent data block again (nblock: %s)", nblock);
                            printmtof(temp_buf, client_output_file);
                        }
                        continue;
                    } else { // MESSAGE RECEIVED
                        break;
                    }
                }
                
                buf[numbytes] = '\0';
                
                // Check if error msg
                if(buf[0]=='0' && buf[1]=='5'){
                    snprintf(temp_buf, sizeof(temp_buf), "clientudp: write: ack: got error packet: %s", buf);
	                printmtof(temp_buf, client_output_file);
					exit(1);
                }
                
                inc_nblock(nblock);                
            } // End of while
            
            snprintf(temp_buf, sizeof(temp_buf), "clientudp: new file %s successfully written\n", filename);
	        printmtof(temp_buf, client_output_file);
            fclose(ptr);
            
        } else {
            snprintf(temp_buf, sizeof(temp_buf), "clientudp: ack: expecting but got : %s", buf);
	        printmtof(temp_buf, client_output_file);
            exit(1);
        }
            
	} else {
	    printmtof("clientudp: illegal TFTP operation received. Operation aborted.", client_output_file);
        exit(1);
	}

    if (n_retry == 0) {
        printf("Unable to get response from");
        printf(" %s after %d attempts.\n", argv[1], RETRIES);
    }

    printf("All done at %s", (char *) ctime(&timevar));
    return 0;
  
}
