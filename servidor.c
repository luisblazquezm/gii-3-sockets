/*
** Fichero: servidor.c
** Autores:
** Luis Blázquez Miñambres DNI 70910465Q
** Samuel Gómez Sánchez    DNI 45136357F
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
#include "log.h"


#define PUERTO 6357
#define ADDRNOTFOUND	0xffffffff	
#define BUFFERSIZE	1024	
#define TAM_BUFFER 1024
#define MAXHOST 128

extern int errno;

char debug_file[] = "debug.txt";
 
void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in);
void errout(char *);		/* declare error out routine */

int FIN = 0;             /* Para el cierre ordenado */
void finalizar(){ FIN = 1; }

int main(argc, argv)
int argc;
char *argv[];
{

    int s_TCP, s_UDP;		
    int ls_TCP;				
    
    int cc;				   
     
    struct sigaction sa = {.sa_handler = SIG_IGN}; 
    
    struct sockaddr_in myaddr_in;	
    struct sockaddr_in clientaddr_in;	
	int addrlen;
	
    fd_set readmask;
    int numfds,s_mayor;
    
    char buffer[BUFFERSIZE];	
    
    struct sigaction vec;
    
    int udp_value = 0;
    
    
    int u_UDP;
    struct sockaddr_in servaddr_in;
    struct addrinfo hints, *res;
    
    long errcode;
    

    /* Create the listen socket. */
	ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}

	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

    addrlen = sizeof(struct sockaddr_in);

	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}
	
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

    /*
     *     Más allá de la simple estructura padre - hijo, el sistema operativo
     * Unix organiza los procesos en sesiones (para poder, por ejemplo, termi-
     * nar con todos los procesos de un usuario) y grupos (construidos como
     * "procesos relacionados" en algún sentido).
     *     Las llamadas setpgid() y getpgrp() permiten modificar el ID de grupo
     * de un proceso; mientras que la primera permite cambiar el ID a uno de 
     * cualquier proceso, setpgrp() establece al que la llama como lider de su
     * grupo.
    */
	setpgrp(); // Eliminamos la relación entre el proceso en ejecución y la
	           // terminal. Ejecución en el fondo: daemon
    
	switch (fork()) {
	case -1:		/* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0:     /* The child process (daemon) comes here. */

		fclose(stdin);
		fclose(stderr);

        // CREATE LOG FILE
        init_log_file(LOG_FILENAME); 

		if (sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror(" sigaction(SIGCHLD)");
            fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
            exit(1);
        }
            
        vec.sa_handler = (void *) finalizar;
        vec.sa_flags = 0;
        
        if (sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
            perror(" sigaction(SIGTERM)");
            fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
            exit(1);
        }

		while (!FIN) {
            
            FD_ZERO(&readmask);
            FD_SET(ls_TCP, &readmask);
            FD_SET(s_UDP, &readmask);
           
    	    if (ls_TCP > s_UDP) s_mayor=ls_TCP;
    		else s_mayor=s_UDP;

            /*
             * select() utiliza objetos de tipo fd_set para esperar a que 
             * varios conjuntos de descriptores de fichero cambien de estado.
             *     Como primer parámetro se le debe pasar el descriptor de
             * fichero de mayor número, para que al buscar busque sólo hasta
             * ese límite.
             *     Además, cuenta con un timeout de tipo struct timeval. La
             * llamada a select quedará bloqueada hasta que:
             *     - Un descriptor de fichero esté preparado
             *     - Una llamada a signal handler la interrumpa
             *     - Expire el timeout
            */
            if ( (numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
                if (errno == EINTR) {
                    FIN=1;
		            close (ls_TCP);
		            close (s_UDP);
                    perror("\nFinalizando el servidor. Señal recibida en select\n "); 
                }
            } else { 

                    /* Comprobamos si el socket seleccionado es el socket TCP */
                    if (FD_ISSET(ls_TCP, &readmask)) {
                        s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
                        
                        if (s_TCP == -1){
                            exit(1);
                        }
                        
                        switch (fork()) {
                            case -1:	/* Can't fork, just exit. */
                                exit(1);
                            case 0:		/* Child process comes here. */
                                close(ls_TCP); 
                                serverTCP(s_TCP, clientaddr_in);
                                exit(0);
                            default:	/* Daemon process comes here. */
                                close(s_TCP);
                        }
                    } /* De TCP*/
                
                    /* Comprobamos si el socket seleccionado es el socket UDP */
                    if (FD_ISSET(s_UDP, &readmask)) {
                        cc = recvfrom(s_UDP, buffer, TAM_BUFFER - 1, 0,
                            (struct sockaddr *)&clientaddr_in, &addrlen);
                        if (cc == -1) {
                            perror(argv[0]);
                            printf("%s: recvfrom error\n", argv[0]);
                            exit (1);
                        }
                        
                        /* Make sure the message received is
                        * null terminated.
                        */
                        buffer[cc]='\0';
                        
                        //serverUDP(s_UDP, buffer, clientaddr_in);
                        char host_name[100];
                        
                        switch (fork()) {
                            case -1:	
                                exit(1);
                            case 0:
                            
                                memset (&myaddr_in, 0, sizeof(struct sockaddr_in));
	                            memset (&servaddr_in, 0, sizeof(struct sockaddr_in));
	
	                            myaddr_in.sin_family = AF_INET;
	                            myaddr_in.sin_port = 0;
	                            myaddr_in.sin_addr.s_addr = INADDR_ANY;

	                            u_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	                            if (u_UDP == -1) {
		                            perror(argv[0]);
		                            printf("%s: unable to create child\'s UDP socket\n", argv[0]);
		                            exit(1);
	                            } 
	                            
	                            if (bind(u_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		                            perror(argv[0]);
		                            printf("%s: unable to bind address UDP\n", argv[0]);
		                            exit(1);
	                            }
	                            
	                            addrlen = sizeof(struct sockaddr_in);
                                if (getsockname(u_UDP, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
                                        perror(argv[0]);
                                        fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
                                        exit(1);
                                }
                                
                                serverUDP(u_UDP, buffer, clientaddr_in);
                                
                                exit(0);
                            default:
                                continue;
                                
                        }
                        
                    }/* De UDP */
                    
              }/* Fin if-else */
              
		}/* Fin del bucle infinito de atención a clientes */
		
        /* Cerramos los sockets UDP y TCP */
        printmtof("Connection closed.", debug_file);
        close(ls_TCP);
        close(s_UDP);
    
        printf("Server finished execution.\n");
        
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
     char temp_buf[1000];
     int nreadbytes = 0; /* Keeps the number of bytes read from file */
     int nwrittenbytes = 0;
     int rreqcnt = 1; /* Keeps count of read requests */
     char filename[100];
     char str[1000];
     short msg_type = 0;
     FILE *ptr = NULL;
     char last_block[3] = "**";
     int lastblock_reached = 0;
     int eof_flag;
     char data_buf[550];
     
     // Log lines data
    //char client_hostname[HOSTNAME_LEN]; // There is another hostname variable
    char host_ipaddr[IPADDR_LEN];
    char protocol[PTCL_LEN] = "TCP";
    char host_port[PORT_LEN];
    char operation[OP_DESC_LEN];
    char error_description[ERR_DESC_LEN];
   /*******************************************************************/
    
	int reqcnt = 0;		
	char buf[TAM_BUFFER];		
	char hostname[MAXHOST];		
	
    int len, len1, status;
    struct hostent *hp;		
    long timevar;			
    
    struct linger linger;		
	 
    status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in), hostname,MAXHOST,NULL,0,0);
    if(status){
        if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            perror(" inet_ntop \n");
    }
    
    // GET CLIENT'S IP ADDRESS AND PORT IN HUMAN FORMAT
    strcpy(host_ipaddr, inet_ntoa(clientaddr_in.sin_addr));
    snprintf(temp_buf, sizeof(temp_buf), "%d", ntohs(clientaddr_in.sin_port));
    strcpy(host_port, temp_buf);

    time (&timevar);

	printf("Startup from %s port %u at %s",
		hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));

	linger.l_onoff  =1;
	linger.l_linger =1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1) {
		errout(hostname);
	}
    
	while (len = recv(s, buf, TAM_BUFFER, 0)) {
		if (len == -1) errout(hostname); 
			 		
		while (len < TAM_BUFFER) {
			len1 = recv(s, &buf[len], TAM_BUFFER-len, 0);
			if (len1 == -1) errout(hostname);
			len += len1;
		}
		
		if (buf[0] == '0' && buf[1] == '1') { // R_REQUEST
		
             strcpy(filename, buf + 2);
             
		     printmtof("server: tcp: received read request", debug_file);
		     rrq_op(filename, temp_buf);
		     write_log_data(hostname,
                            host_ipaddr,
                            protocol,
                            host_port,
                            temp_buf,
                            "",
                            (LOG_FILENAME));
            
             server_get_filepath(filename, temp_buf);
		     if ((lastblock_reached = read_from_file((char *)data_buf, temp_buf, nreadbytes)) == -1){
                printmtof("servidor.c: serverTCP: READ_TYPE: error in read_from_file", debug_file);
		        return;
             }
             
             if (lastblock_reached){
             
		       strncpy(last_block, buf + 2, 2);
		       last_block[2] = '\0';
		       
		       snprintf(temp_buf, sizeof(temp_buf), "server: tcp: sent last data block (nblock: %s)", last_block);
		       printmtof(temp_buf, debug_file);
		       
		     }
             
		     char *data_msg = create_data_msg("01", data_buf); 
		     nreadbytes += TFTP_DATA_SIZE;
		     reqcnt++;
		     sleep(1); // SIMULATE DELAY

		     if (send(s, data_msg, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
		     
		     printmtof("server: tcp: sent data block (nblock: 01)", debug_file);

		} else if (buf[0] == '0' && buf[1] == '2') {// W_REQUEST
		
		     strcpy(filename, buf + 2);
		
		     printmtof("server: tcp: received write request", debug_file);
		     
		     wrq_op(filename, temp_buf);
		     write_log_data(hostname,
                            host_ipaddr,
                            protocol,
                            host_port,
                            temp_buf,
                            "",
                            (LOG_FILENAME));
		     
		     char *ack_msg = create_ack_msg("00");
		     reqcnt++;
		     sleep(1); // SIMULATE DELAY

		     if (send(s, ack_msg, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
		     
		     printmtof("server: tcp: sent ack (nblock: 00)", debug_file);
		     
        } else if (buf[0] == '0' && buf[1] == '4') { //ACK
		    
		    /*
		     * Para el ACK X:
		     *      1- Enviar bloque X + 1
		     */   
		     
		    char nblock[3];
		    nblock[0] = buf[2], nblock[1] = buf[3], nblock[2] = '\0';
		    
		    snprintf(temp_buf, sizeof(temp_buf), "server: tcp: received ACK number %s", nblock);
		    printmtof(temp_buf, debug_file);

		    if (('*' != last_block[0] && '*' != last_block[1])
		    && (nblock[0] == last_block[0] && nblock[1] == last_block[1])){
			    printmtof("clientcp: done reading source file", debug_file);
			    goto END_OF_FILE;
		    }
		    
		    server_get_filepath(filename, temp_buf);
		    if ((lastblock_reached = read_from_file((char *)data_buf, temp_buf, nreadbytes)) == -1){//<<<<<<<<<<<<<<<<<<<<<<<<< &buf???
	           fprintf(stderr, "servidor.c: DATA_TYPE: error in read_from_file\n");
		       return;
		    }
		    
		     inc_nblock(nblock);
             
		     char *data_msg = create_data_msg(nblock, data_buf);
		     nreadbytes += TFTP_DATA_SIZE;
		     reqcnt++;
		     sleep(1);

		     if (send(s, data_msg, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
		     
		     if (lastblock_reached){
		    
		       strcpy(last_block, nblock);
		       
		       snprintf(temp_buf, sizeof(temp_buf), "server: tcp: sent last data block (nblock: %s)", last_block);
		       printmtof(temp_buf, debug_file);
		       
		    } else {
		    
             snprintf(temp_buf, sizeof(temp_buf), "server: tcp: sent data block (nblock: %s)", nblock);
             printmtof(temp_buf, debug_file);
		     
		    }
		     
		} else if (buf[0] == '0' && buf[1] == '3') { // DATA
		
            char data[TFTP_DATA_SIZE + 1];
            strcpy(data, buf + 4);
            
            char nblock[3];
		    nblock[0] = buf[2], nblock[1] = buf[3], nblock[2] = '\0';
		    
		    snprintf(temp_buf, sizeof(temp_buf), "server: tcp: received data block (nblock: %s, size: %d)", nblock, strlen(data));
		    printmtof(temp_buf, debug_file);
		    
		    /*
		     * Para el bloque X
		     *      1- Escribir DATA X
		     *      2- Mandar ACK (n = X)
		     */
		     
		    if (0 == strlen(data)) { // DATA = 0
		        
		        char *ack_msg = create_ack_msg(nblock);
		        reqcnt++;
		        sleep(1);
		        if (send(s, ack_msg, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
		        
		        snprintf(temp_buf, sizeof(temp_buf), "server: tcp: sent ack (nblock: %s)", nblock);
		        printmtof(temp_buf, debug_file);
		        
			    goto END_OF_FILE;
			    
		    } else if (TFTP_DATA_SIZE > strlen(data)) { // DATA < 512  

                server_get_filepath(filename, temp_buf);
		        if ((write_data_into_file(data, temp_buf, nwrittenbytes)) == -1){
		            fprintf(stderr, "servidor.c: DATA_TYPE: error in write_data_into_file\n");
			        return;
		        }
		        
		        snprintf(temp_buf, sizeof(temp_buf), "server: tcp: wrote last data block (nblock: %s)", nblock);
		        printmtof(temp_buf, debug_file);

		        char *ack_msg = create_ack_msg(nblock);
		        nwrittenbytes += strlen(data);
		        reqcnt++;
		        sleep(1);

		        if (send(s, ack_msg, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
		        
		        snprintf(temp_buf, sizeof(temp_buf), "server: tcp: sent ack (nblock: %s)", nblock);
		        printmtof(temp_buf, debug_file);
		        
				goto END_OF_FILE;
				 
		    } else { // DATA = 512    
		        
		        server_get_filepath(filename, temp_buf);
		        if ((write_data_into_file(data, temp_buf, nwrittenbytes)) == -1){
		            fprintf(stderr, "servidor.c: DATA_TYPE: error in write_data_into_file\n");
			        return;
		        }
		        
		        char *ack_msg = create_ack_msg(nblock);
		        nwrittenbytes += TFTP_DATA_SIZE;
		        reqcnt++;
		        sleep(1);

		        if (send(s, ack_msg, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
		        
		        snprintf(temp_buf, sizeof(temp_buf), "server: tcp: sent ack (nblock: %s)", nblock);
		        printmtof(temp_buf, debug_file);
		        
		    }

		} else {
			fprintf(stderr, "serverTCP: Invalid message type\n");
            exit(1);
		}
	    
	}// End of while

END_OF_FILE:

	close(s);

	time (&timevar);

    suc_op(filename, temp_buf);
    write_log_data(hostname,
                   host_ipaddr,
                   protocol,
                   host_port,
                   temp_buf,
                   "",
                   (LOG_FILENAME));

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
void serverUDP(int s, char *buffer, struct sockaddr_in clientaddr_in)
{
    struct in_addr reqaddr;	
    struct hostent *hp;		
    struct addrinfo hints, *res;
    int nc, errcode;
	int addrlen;
    
    /* Estas variables son para la gestion de los ficheros.
     * Realmente habria que cambiarlas para trabajar con arrays dinamicos
     * pero por ahora vamos a tener un unico fichero (y solo pa' leer)
     */
     int rreqcnt = 1; /* Keeps count of read requests */
     char temp_buf[1000];
     char filename[128];
     //char filename[100];
     //char str[1000];
     //static int last_block = 0;
     //int eof_flag;

    // Log lines data
    //char client_hostname[HOSTNAME_LEN]; // There is another hostname variable
    char host_ipaddr[IPADDR_LEN];
    char protocol[PTCL_LEN] = "UDP";
    char host_port[PORT_LEN];
    char operation[OP_DESC_LEN];
    char error_description[ERR_DESC_LEN];
    char hostname[MAXHOST];
   /*******************************************************************/

   	addrlen = sizeof(struct sockaddr_in);
   	
   	int status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in), hostname,MAXHOST,NULL,0,0);
    if(status){
        if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            perror("server: udp: inet_ntop \n");
    }
   	
   	// GET CLIENT'S IP ADDRESS AND PORT IN HUMAN FORMAT
    strcpy(host_ipaddr, inet_ntoa(clientaddr_in.sin_addr));
    snprintf(temp_buf, sizeof(temp_buf), "%d", ntohs(clientaddr_in.sin_port));
    strcpy(host_port, temp_buf);

    if (buffer[0] == '0' && buffer[1] == '1'){ // READ REQUEST
        /*
         * 1- Check if file exists
         *     + If it does not, error message and exit
         * 2- Send file
         *     + Get file size
         *     + Loop until size is reached
         *         * fread() filtering by remaining size
         *         * Add \0 so that create_data_msg works fine
         *         * Create data packet and send
         *         * Wait for ACK (with a number of retries)
         *             > If timeout, try to resend
         *         * Increment block number
         *     + Close file
        */
        
        strcpy(filename, buffer + 2);
        
        printmtof("server: udp: received read request", debug_file);
        rrq_op(filename, temp_buf);
	    write_log_data(hostname,
                       host_ipaddr,
                       protocol,
                       host_port,
                       temp_buf,
                       "",
                       (LOG_FILENAME)); 
        
        server_get_filepath(filename, temp_buf);
        FILE *ptr = NULL;            
        if ((ptr = fopen(temp_buf, "r")) == NULL || access(temp_buf, F_OK) == -1) { // FILE NOT FOUND
            snprintf(temp_buf, sizeof(temp_buf), "server: udp: file '%s' does not exist, sending error packet and aborting...", filename);
            printmtof(temp_buf, debug_file);
            rrq_op(filename, temp_buf);
            write_log_data(hostname,
                           host_ipaddr,
                           protocol,
                           host_port,
                           temp_buf,
                           "01 - FILE NOT FOUND",
                           (LOG_FILENAME));
            char *e_msg = create_error_msg("01", "FILE NOT FOUND");
            sendto(s, e_msg, strlen(e_msg), 0, (struct sockaddr *)&clientaddr_in, addrlen);
            printmtof("Connection aborted", debug_file);
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
                
        char nblock[] = "01";
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
            
            numbytes = sendto(s, data_msg, strlen(data_msg), 0, (struct sockaddr *)&clientaddr_in, addrlen);
            if (numbytes == -1) {
                perror("serverUDP: Read Request: sendto");
                exit(1);
            } else {
                snprintf(temp_buf, sizeof(temp_buf), "server: udp: sent data block (nblock: %s)", nblock);
                printmtof(temp_buf, debug_file);
            }
            
            int times;
            for (times = 0; times <= 5; times++) {
            
                if (times == 5){
                    snprintf(temp_buf, sizeof(temp_buf), "server: udp: max number of tries %d reached\n", times);
                    printmtof(temp_buf, debug_file);
                    exit(1);
                }
                
                numbytes = wait_ack(s, buffer, &clientaddr_in, addrlen);
		        printmtof("server: udp: received ACK", debug_file);
                
                if (numbytes == -1) { // ERROR
                    perror("serverUDP: recvfrom after check_timeout");
                    exit(1);
                } else if (numbytes == -2) { // TIMEOUT
                
                    snprintf(temp_buf, sizeof(temp_buf), "server: udp: try no. %d. Trying again...", times + 1);
                    printmtof(temp_buf, debug_file);
                    
                    int bytes = sendto(s, data_msg, strlen(data_msg), 0, (struct sockaddr *)&clientaddr_in, addrlen);
                    if (bytes == -1) {
                        perror("server: udp: timeout: sendto");
                        exit(1);
                    } else {
                        snprintf(temp_buf, sizeof(temp_buf), "server: udp: sent data block again (nblock: %s)", nblock);
                        printmtof(temp_buf, debug_file);
                    }
                    
                    continue;
                } else { // MESSAGE RECEIVED
                    break;
                }
            }
            
            buffer[numbytes] = '\0';
            
            inc_nblock(nblock);
        }
        
        fclose(ptr);
         
    } else if (buffer[0] == '0' && buffer[1] == '2') { // WRITE REQUEST
        /*
         * 1- Sends ACK
         *     + If it does not, error message and exit
         * 2- Send file
         *     + Get file size
         *     + Loop until size is reached
         *         * fread() filtering by remaining size
         *         * Add \0 so that create_data_msg works fine
         *         * Create data packet and send
         *         * Wait for ACK (with a number of retries)
         *             > If timeout, try to resend
         *         * Increment block number
         *     + Close file
        */

        strcpy(filename, buffer + 2);
        
        printmtof("server: udp: received write request", debug_file);
        wrq_op(filename, temp_buf);
	    write_log_data(hostname,
                       host_ipaddr,
                       protocol,
                       host_port,
                       temp_buf,
                       "",
                       (LOG_FILENAME)); 
        
        
        int numbytes;
        char *ack_msg = create_ack_msg("00"); // First is ACK 0
        
        char last_data_msg[TFTP_DATA_SIZE + 1];
        strcpy(last_data_msg, buffer);
        char last_ack_msg[10];
        strcpy(last_ack_msg, ack_msg);
			                  
        numbytes = sendto(s, ack_msg, strlen(ack_msg), 0, (struct sockaddr *)&clientaddr_in, addrlen);
        if (numbytes == -1) {
            perror("serverUDP: Write Request: sendto ACK 00");
            exit(1);
        }
        
        printmtof("server: udp: sent ack (nblock: 00)", debug_file);

        
        // Aqui hace un strcat <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        
        server_get_filepath(filename, temp_buf);
        if (access(temp_buf, F_OK) != -1) { // DUPLICATE FILE
            snprintf(temp_buf, sizeof(temp_buf), "server: udp: file '%s' already exists, sending error packet and aborting...", filename);
            printmtof(temp_buf, debug_file);
            wrq_op(filename, temp_buf);
            write_log_data(hostname,
                           host_ipaddr,
                           protocol,
                           host_port,
                           temp_buf,
                           "06 - FILE ALREADY EXISTS",
                           (LOG_FILENAME));
            char *e_msg = create_error_msg("06", "FILE ALREADY EXISTS");
            sendto(s, e_msg, strlen(e_msg), 0, (struct sockaddr *)&clientaddr_in, addrlen);
            printmtof("Connection aborted", debug_file);
            exit(1);
        }
        FILE *ptr;
        if ((ptr = fopen(temp_buf, "w")) == NULL) { // ACCESS DENIED
            snprintf(temp_buf, sizeof(temp_buf), "server: udp: access to file '%s' denied, sending error packet and aborting...", filename);
            printmtof(temp_buf, debug_file);
            wrq_op(filename, temp_buf);
            write_log_data(hostname,
                           host_ipaddr,
                           protocol,
                           host_port,
                           temp_buf,
                           "02 - ACCESS VIOLATION",
                           (LOG_FILENAME));
            char *e_msg = create_error_msg("02", "ACCESS VIOLATION");
            sendto(s, e_msg, strlen(e_msg), 0, (struct sockaddr *)&clientaddr_in, addrlen);
            printmtof("Connection aborted", debug_file);
            exit(1);
        }
        
        int nbytes_written = 0;
        int total = 0;
        do {
            // RECEIVE DATA
			int addr_len = sizeof(clientaddr_in);
            numbytes = recvfrom(s, buffer, TAM_BUFFER - 1, 0, (struct sockaddr *)&clientaddr_in, &addr_len);
            if (numbytes == -1 ) {
                perror("serverUDP: Write Request: recvfrom DATA");
                exit(1);
            }
            
            char nblock[3];
		    nblock[0] = buffer[2], nblock[1] = buffer[3], nblock[2] = '\0';
            
            snprintf(temp_buf, sizeof(temp_buf), "server: udp: received data block (nblock: %s, size: %d)", nblock, strlen(buffer + 4));
		    printmtof(temp_buf, debug_file);
		    
            buffer[numbytes] = '\0';
            
            // SEND LAST ACK AGAIN
            if(!strcmp(buffer, last_data_msg)){
				sendto(s, last_ack_msg, strlen(last_ack_msg), 0, (struct sockaddr *)&clientaddr_in, addrlen);
				continue;
            }
            
            // WRITE FILE
            nbytes_written = strlen(buffer + 4);
            if (-1 == fwrite(buffer + 4, nbytes_written, 1, ptr)) {
                perror("serverUDP: Write Request: fwrite");
                exit(1);
            }
            strcpy(last_data_msg, buffer); // SAVE LAST DATA FOR RESENDING
            
            // SEND ACK
            char *ack_msg = create_ack_msg(nblock);
            numbytes = sendto(s, ack_msg, strlen(ack_msg), 0, (struct sockaddr *)&clientaddr_in, addrlen);
            if (numbytes == -1 ) {
                perror("serverUDP: Read Request: sendto ACK");
                exit(1);
            }
            strcpy(last_ack_msg, ack_msg); // SAVE LAST ACK FOR RESENDING
            
            snprintf(temp_buf, sizeof(temp_buf), "server: udp: sent ack (nblock: %s)", nblock);
		    printmtof(temp_buf, debug_file);
            
        } while (nbytes_written == TFTP_DATA_SIZE);
        
        snprintf(temp_buf, sizeof(temp_buf), "server: udp: file %s successfully written", filename);
		printmtof(temp_buf, debug_file);
        fclose (ptr);
        
    } else { // SENDING ERROR PACKET - 04 ILEGAL OPERATION FILE
    
        printmtof("server: udp: unknown request received, sending error packet and aborting...", debug_file);
        write_log_data(hostname,
                           host_ipaddr,
                           protocol,
                           host_port,
                           temp_buf,
                           "04 - ILLEGAL TFTP OPERATION",
                           (LOG_FILENAME));
        char *e_msg = create_error_msg("04", "ILLEGAL TFTP OPERATION");
        sendto(s, e_msg, strlen(e_msg), 0, (struct sockaddr *)&clientaddr_in, addrlen);
        printmtof("Connection aborted", debug_file);
        exit(1);
    }
    
    suc_op(filename, temp_buf);
    write_log_data(hostname,
                   host_ipaddr,
                   protocol,
                   host_port,
                   temp_buf,
                   "",
                   (LOG_FILENAME)); 
  
 }
