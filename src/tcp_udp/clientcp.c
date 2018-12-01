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
#include <string.h>
#include <netdb.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
//#include "tftp.h"
//#include "utils.h"
//#include "log.h"
//#include "tftp.h"
//#include "clientcp.h"

#define PUERTO 17278
#define TAM_BUFFER 512

#define NULL_PTR_ERROR          -1

#define LOG_EMPTY_FILENAME_ERR 	1
#define OPEN_FILE_ERROR 	2
#define WRITE_FILE_ERROR 	3
#define READ_FILE_ERROR 	4
#define CLOSE_FILE_ERROR        5

#define TCP_PROTOCOL_CODE       0
#define UDP_PROTOCOL_CODE       1
#define WRONG_MSG_TYPE  -1

#define NETASCII_MODE	0
#define OCTET_MODE	1
#define MAIL_MODE	2

#define READ_TYPE       1
#define WRITE_TYPE      2
#define DATA_TYPE       3
#define ACK_TYPE        4
#define ERROR_TYPE      5

typedef struct rw_msg
{
    int msg_type;
    char filename[1000];
    char byte_1;
    char mode[10];
    char byte_2;
    
} rw_msg_t;

typedef struct data_msg
{
    int msg_type;
    int n_block;
    char data[1000];
    
} data_msg_t;

typedef struct ack_msg
{
    int msg_type;
    int n_block;
    
} ack_msg_t;

typedef struct error_msg
{
    int msg_type;
    int error_code;
    char error_msg[1000];
    char byte;
    
} error_msg_t;

rw_msg_t *create_rw_msg(int msg_type, char *filename);
ack_msg_t *create_ack_msg(int n_block);
data_msg_t *create_data_msg(int n_block, char *data);
error_msg_t *create_error_msg(int error_code);

int test_args(int argc, char *argv[]);
int invalid_option_msg(char * opt);
int short_help_msg(void);
int help_msg(void);

char* strfmt(const char* format, ...);
int test_args(int argc, char *argv[]);
int invalid_option_msg(char * opt);
int short_help_msg(void);
int help_msg(void);


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
int main(int argc, char *argv[])
//int argc;
//char *argv[];
{
    int socket_descriptor;		/* connected socket descriptor */ // OJOOOOOOOO cambiado 
    struct addrinfo hints, *res;
    long timevar;			/* contains time returned by time() */
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in servaddr_in;	/* for server socket address */
    int addrlen, i, j, errcode;
    char buf[TAM_BUFFER];               /* This example uses TAM_BUFFER byte messages. */
    
    /* Parameters */
    int tmp = 0;
    int msg_type = 0; // 0 Read , 1 Write //OJOOOOOOOOOOOOOO QUITAR EL = 0
    rw_msg_t *rw_msg;
    ack_msg_t *ack_msg;
    data_msg_t *data_msg;
    error_msg_t *error_msg;
    char filename[100];
    fprintf(stderr, "%s: invalid args\n", argv[0]);
    
    tmp = test_args(argc, argv);
    if (tmp == -1){
	fprintf(stderr, "%s: invalid args\n", argv[0]);
        return -1;
    } else {
	if (strcmp(argv[3],"r"))
        	msg_type = READ_TYPE;
	else 
		msg_type = WRITE_TYPE;
 
        strcpy(argv[4], filename);
        /*
        if (tmp & 0x4) // Testeamos a nivel de bit. Ver test_args
            debug_flag = 1;
        else
            debug_flag = 0;
        if (tmp & 0x2)
            prioridad = PRIOR_APARCAR;
        else // Por defecto, si no se indica, prioridad PD
            prioridad = PRIOR_DESAPARCAR;
        */
    }
    
    /* Create the socket. */
    socket_descriptor = socket (AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor == -1) {
	perror(argv[0]);
	fprintf(stderr, "%s: unable to create socket\n", argv[0]);
	exit(1);
    }

    /* clear out address structures */
    memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
    memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

    /* Set up the peer address to which we will connect. */
    servaddr_in.sin_family = AF_INET;

    /* 
     * Get the host information for the hostname that the
     * user passed in. 
     */
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    
    /* Esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
    errcode = getaddrinfo (argv[1], NULL, &hints, &res); 
    if (errcode != 0){
	/* 
	 * Name was not found.  
	 * Return a special value signifying the error. 
	 */
	fprintf(stderr, "%s: No es posible resolver la IP de %s\n",argv[0], argv[1]);
	exit(1);
    } else {
	/* Copy address of host */
	servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
    }
    freeaddrinfo(res);

    /* Puerto del servidor en orden de red*/
    servaddr_in.sin_port = htons(PUERTO);

    /* Try to connect to the remote server at the address
     * which was just built into peeraddr.
     */
    if (connect(socket_descriptor, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
	perror(argv[0]);
	fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
	exit(1);
    }

     /* Since the connect call assigns a free address
      * to the local end of this connection, let's use
      * getsockname to see what it assigned.  Note that
      * addrlen needs to be passed in as a pointer,
      * because getsockname returns the actual length
      * of the address.
      */
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(socket_descriptor, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
	perror(argv[0]);
	fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
	exit(1);
    }

    /* Print out a startup message for the user. */
    time(&timevar);
    /* The port number must be converted first to host byte
     * order before printing.  On most hosts, this is not
     * necessary, but the ntohs() call is included here so
     * that this program could easily be ported to a host
     * that does require it.
     */
     printf("Connected to %s on port %u at %s", argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

     /* Quitar el bucle y enviar una petición de lectura o escritura dependiendo de lo que pida el usuario  */
     /* Creamos el buffer */
	
     memset(buf, 0, sizeof(buf));
     if (msg_type == READ_TYPE) { // 0 is READ_MODE
	rw_msg = create_rw_msg(READ_TYPE, filename);
	
	((rw_msg_t*)buf)->msg_type = rw_msg->msg_type;
	strcpy(rw_msg->filename, ((rw_msg_t*)buf)->filename);
	((rw_msg_t*)buf)->byte_1 = rw_msg->byte_1;
	strcpy(rw_msg->mode, ((rw_msg_t*)buf)->mode);
	((rw_msg_t*)buf)->byte_2 = rw_msg->byte_2;
	
	if (send(socket_descriptor, buf, TAM_BUFFER, 0) != TAM_BUFFER) {
		fprintf(stderr, "%s: send rw_message: Connection aborted on error ", argv[0]);
		fprintf(stderr, "on send number %d\n", i);
		exit(1);
	}
     } else if (msg_type == WRITE_TYPE){ // 1 is WRITE_MODE
	rw_msg = create_rw_msg(WRITE_TYPE, filename);

	((rw_msg_t*)buf)->msg_type = rw_msg->msg_type;
	strcpy(rw_msg->filename, ((rw_msg_t*)buf)->filename);
	((rw_msg_t*)buf)->byte_1 = rw_msg->byte_1;
	strcpy(rw_msg->mode, ((rw_msg_t*)buf)->mode);
	((rw_msg_t*)buf)->byte_2 = rw_msg->byte_2;

	if (send(socket_descriptor, buf, TAM_BUFFER, 0) != TAM_BUFFER) {
		fprintf(stderr, "%s: send rw_message: Connection aborted on error ", argv[0]);
		fprintf(stderr, "on send number %d\n", i);
		exit(1);
	}
     }

	/* Now, shutdown the connection for further sends.
         * This will cause the server to receive an end-of-file
	 * condition after it has received all the requests that
	 * have just been sent, indicating that we will not be
	 * sending any further requests.
	 */
	if (shutdown(socket_descriptor, 1) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
		exit(1);
	}

	/* Now, start receiving all of the replys from the server.
	 * This loop will terminate when the recv returns zero,
	 * which is an end-of-file condition.  This will happen
	 * after the server has sent all of its replies, and closed
	 * its end of the connection.
	 */

	/* ----------------------- Aquí se envia el ACK --------------------------*/
	/*
	 * If data is not available for the socket socket, and socket is in blocking mode, 
	 * the recv() call blocks the caller until data arrives. 
	 * If data is not available and socket is in nonblocking mode, 
	 * recv() returns a -1 and sets the error code to EWOULDBLOCK.
	 */
	memset(buf, 0, sizeof(buf));
	while (i = recv(socket_descriptor, buf, TAM_BUFFER, 0)) {
		if (i == -1) {
	    	    perror(argv[0]);
		    fprintf(stderr, "%s: error reading result\n", argv[0]);
		    exit(1);
		} else {
		
		}
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
			 * the begining of the next reply.
			 */
		   while (i < TAM_BUFFER) {
			j = recv(socket_descriptor, &buf[i], TAM_BUFFER-i, 0);

			if (j == -1) {
	            		perror(argv[0]);
			 	fprintf(stderr, "%s: error reading result\n", argv[0]);
			 	exit(1);
	       		}

			i += j;
		    }
		
		/* Print out message indicating the identity of this reply. */
		printf("Received result number %d\n", *buf);
	}

	// Envia el ACK si es de lectura y el bloque de datos si es de escritura
	if (msg_type == READ_TYPE) {
	        ack_msg = create_ack_msg(1);
	        
		((ack_msg_t*)buf)->msg_type = ack_msg->msg_type;
		((ack_msg_t*)buf)->n_block = 1;

	        if (send(socket_descriptor, buf, TAM_BUFFER, 0) == -1) {
		    fprintf(stderr, "%s: send ACK_message: Connection aborted on error ", argv[0]);
		    fprintf(stderr, "on send number %d\n", i);
		    exit(1);
	        }
	} else if (msg_type == WRITE_TYPE) {
		data_msg = create_data_msg(1, NULL);
	        
		((data_msg_t*)buf)->msg_type = rw_msg->msg_type;
		((data_msg_t*)buf)->n_block = 1;
		strcpy(data_msg->data, ((data_msg_t*)buf)->data);

	        if (send(socket_descriptor, buf, TAM_BUFFER, 0) == -1) {
		    fprintf(stderr, "%s: send data_msg: Connection aborted on error ", argv[0]);
		    fprintf(stderr, "on send number %d\n", i);
		    exit(1);
	        }
	}

    /* Print message indicating completion of task. */
	time(&timevar);
	printf("All done at %s", (char *)ctime(&timevar));
}

rw_msg_t *create_rw_msg(int msg_type, char *filename)
{
    rw_msg_t *rw_msg;
    
    // Only two modes are available, read and write, as defined in tftp.h
    if (msg_type != READ_TYPE && msg_type != WRITE_TYPE)
    {
        printf("%s\n", "create_rw_msg: wrong message type");
        return NULL;
    } else if (filename == NULL) {
        printf("%s\n", "create_rw_msg: NULL pointer \'filename\'");
        return NULL;
    }
    
    if ((rw_msg = malloc(sizeof(rw_msg_t))) == NULL) {
        printf("%s\n", "create_rw_msg: could not allocate resource");
        return NULL;
    }
    
    rw_msg->msg_type = msg_type;
    strcpy(filename, rw_msg->filename);
    rw_msg->byte_1 = '\0';
    strcpy("octet", rw_msg->mode);
    rw_msg->byte_2 = '\0';
    
    return rw_msg;
}


data_msg_t *create_data_msg(int n_block, char *data)
{
    data_msg_t *data_msg;
    
    /*Borrar esto*/
    if (data == NULL)
	return NULL;

    if (n_block == 0) {
        printf("%s\n", "create_data_msg: number of block incorrect");
        return NULL;
    } else if (data == NULL) {
	printf("%s\n", "create_data_msg: NULL pointer data");
        return NULL;
    }
    
    if ((data_msg = malloc(sizeof(data_msg_t))) == NULL) {
        printf("%s\n", "create_data_msg: could not allocate resource");
        return NULL;
    }
    
    data_msg->msg_type = DATA_TYPE;
    data_msg->n_block = n_block + 1; // I dont really know if this parameter is neccesary as it sends data packets in a sequential order.
    strcpy(data, data_msg->data);
    
    return data_msg;
}

ack_msg_t *create_ack_msg(int n_block)
{
    ack_msg_t *ack_msg;
    
    // Only two modes are available, read and write, as defined in tftp.h
     if (n_block == 0) {
        printf("%s\n", "create_ack_msg: number of block incorrect");
        return NULL;
    }
    
    if ((ack_msg = malloc(sizeof(ack_msg_t))) == NULL) {
        printf("%s\n", "create_ack_msg: could not allocate resource");
        return NULL;
    }
    
    ack_msg->msg_type = ACK_TYPE;
    ack_msg->n_block = n_block + 1; // I dont really know if this parameter is neccesary as it sends data packets in a sequential order.
    
    return ack_msg;
}

error_msg_t *create_error_msg(int error_code)
{
    error_msg_t *error_msg;
    char error_output[200];
    
    // Only two modes are available, read and write, as defined in tftp.h
    if (error_code < 0) {
        printf("%s\n", "create_error_msg: invalid error_code ");
        return NULL;
    }
    
    if ((error_msg = malloc(sizeof(error_msg_t))) == NULL) {
        printf("%s\n", "create_error_msg: could not allocate resource");
        return NULL;
    }
    
    switch(error_code){
	case 0:
		strcpy("Not defined", error_output);
		break;
	case 1:
		strcpy("File not found", error_output);
		break;
	case 3:
		strcpy("Full disk", error_output);
		break;
	case 4:
		strcpy("Ilegal operation on TFTP protocol", error_output);
		break;
	case 6:
		strcpy("File already exists", error_output);
		break;
	default:
		strcpy("Out of bounds error > 6", error_output);
		break;
	
    }

    error_msg->msg_type = ERROR_TYPE;
    error_msg->error_code = error_code;
    strcpy(error_output, error_msg->error_msg);
    error_msg->byte = '\0';

    return error_msg;
}

char* strfmt(const char* format, ...)
{
	char *		str;
	va_list		args;

	if ((str = malloc(100 * sizeof(char))) == NULL)
		return NULL;


	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args); // do check return value
	va_end(args);

	return str;
}

int test_args(int argc, char *argv[])
{
    int i, j; // Indices

    /* Para almacenar las opciones D, PA y PD bitwise.
     * Así:
     *      100 -> D
     *      010 -> PA
     *      001 -> PD
     *      101 -> D y PD
     * etc.
     */
    int options = 0;

    if (argc < 2){

        invalid_option_msg("None");
        return -1;

    } else {

        if (argc == 2) {

            if (!strcmp(argv[1], "--help"))
                help_msg();
            else
                short_help_msg();

            return -1;

        } else if (argc >= 3){

	/*
            // Comprobamos si los primeros argumentos son numeros
            i = 0;
            while (argv[1][i] != '\0'){
                if (!isdigit(argv[1][i]))
                    return -1;
                ++i;
            }

            i = 0;
            while (argv[2][i] != '\0'){
                if (!isdigit(argv[2][i]))
                    return -1;
                ++i;
            }

            for (i = 3; i < argc; ++i){

                j = 0;
                while (argv[i][j] != '\0'){
                    argv[i][j] = toupper(argv[i][j]);
                    ++j;
                }

                if (!strcmp(argv[i], "D")){
                    if (options & 0x4){ // Test bitwise
                        invalid_option_msg(argv[i]);
                        return -1;
                    } else {
                        options |= 0x4; // Set bitwise
                    }
                } else if (!strcmp(argv[i], "PA")){
                    if ((options & 0x1) || (options & 0x2)) {
                        invalid_option_msg(argv[i]);
                        return -1;
                    } else {
                        options |= 0x2;
                    }
                } else if (!strcmp(argv[i], "PD")){
                    if ((options & 0x2) || (options & 0x1)) {
                        invalid_option_msg(argv[i]);
                        return -1;
                    } else {
                        options |= 0x1;
                    }
                } else {
                    invalid_option_msg(argv[i]);
                    return -1;
                }
            }
	*/
        }

        return 1;
    }
}

int invalid_option_msg(char * opt)
{
    char msg[1000] = "cliente: invalid option -- '%s'\n\
Try 'cliente --help' for more information.\n"; 

    return printf(msg, opt);
}

int short_help_msg(void)
{

    char msg[1000] = "Usage: cliente olivo P [r | l] file_name\n\
Try 'cliente --help' for more information.\n"; 
                         
    return printf("%s", msg);

}

int help_msg(void)
{

    char msg[1000] = "Usage:cliente olivo P [r | l] file_name\n\
Simulates a process  allocation  system with  an  interface  that  emulates a\n\
road with cars which must be parked in a ordered fashion.\n\
    P                   Transport Protocol executed in the communication,\n\
			it can be TCP or UDP\n\
    r                   Selects reading process\n\
    w                   Selects writing process\n\
    file_name           Name of the file that allocates the data to be written o read.\n"; 
                         
    return printf("%s", msg);
}



