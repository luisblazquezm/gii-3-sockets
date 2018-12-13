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

// Converts number to string
void nblock_to_str(char *str, int n)
{

	if (n == 0){

		str[0] = '0';
		str[1] = '0';
		str[2] = '\0';

	} else if (n % 10 > 0 && n/10 == 0){

		char c = n +'0'; // Int to ASCII representation

		str[0] = '0';
		str[1] = c;
		str[2] = '\0';

	} else if (n % 100 > 0 && n/100 == 0){

		char c2 = (n % 10) + '0'; // Int to ASCII representation
		char c1 = (n/10) + '0';   //

		str[0] = c1;
		str[1] = c2;
		str[2] = '\0';

	} else {

		str[0] = '9';
		str[1] = '9';
		str[2] = '\0';
	}
}

char* create_r_msg(char *filename)
{
	char *packet;
    char mode[] = "octet";
	packet = calloc(1, 4 + strlen(filename) + strlen(mode));
	strcat(packet, "01");
	strcat(packet, filename);
	strcpy(packet + 3 + strlen(filename), mode); // Type + filename + \0
	return packet;
}

char* create_w_msg(char *filename)
{
	char *packet;
	char mode[] = "octet";
	packet = calloc(1, 2 + strlen(filename));
	strcat(packet, "02");
	strcat(packet, filename);
	strcpy(packet + 3 + strlen(filename) , mode); // Type + filename + \0
	return packet;
}

char* create_data_msg(int block, char *data)
{
	char *packet;
	char temp[3];
	nblock_to_str(temp, block);
	packet = calloc(1, 4 + strlen(data));
	strcat(packet, "03");
	strcat(packet, temp);
	strcat(packet, data);
	return packet;
}

char* create_ack_msg(char* block)
{
	char *packet;
	packet = calloc(1, 2 + strlen(block));
	strcat(packet, "04");
	strcat(packet, block);
	return packet;
}

char* create_error_msg(char *errcode, char* errmsg)
{
	char *packet;
	packet = calloc(1, 4 + strlen(errmsg));
	strcat(packet, "05");
	strcat(packet, errcode);
	strcat(packet, errmsg);
	return packet;
}

int wait_ack(int s, char *buffer, struct sockaddr_in clientaddr_in, int addr_len) 
{
    fd_set socket_mask;
    int num_fds;
    struct timeval timeout_tv;
    
    FD_ZERO(&socket_mask);
    FD_SET(s, &socket_mask);
    
    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< WHAT IS THIS??
    timeout_tv.tv_sec = TIMEOUT_N_TRIES;
    timeout_tv.tv_usec = 0;
    
    num_fds= select(s+1, &socket_mask, NULL, NULL, &timeout_tv);
    if (num_fds == 0) {
        printf("tftp: wait_ack: Timeout \n");
        return -2;
    } else if (num_fds == -1) {
        printf("tftp: wait_ack: Error \n");
        return -1;
    }
    
    return recvfrom(s, buffer, TAM_BUFFER-1, 0, (struct sockaddr *)&clientaddr_in, &addr_len);
}


int read_from_file(char *data, char *filename, int pos)
{
    FILE *f = open_file(filename, "r");
	
    if (f == NULL) return -1;
     
    if ((locate_in_file_position(f, pos)) == -1) return -1; //nwrittenbytes
    
    printf("READ:El fichero es %s y el tamaño es %ld\n", filename, sizeof(data));
    
     if (1 != fread(data, TFTP_DATA_SIZE, 1, f)){
        if (ferror(f)) {
            fprintf(stderr,"tftp.c: read_from_file: error in fread");
            return -1;
        } else if (feof(f)) {
            return 1;
        }
     }
     
     if (0 != fclose(f)){
         fprintf(stderr,"tftp.c: read_from_file: error in fclose");
         return -1;
     }

     return 0;
}

int write_data_into_file(char* data_msg, char *filename, int pos)
{
    FILE *f = open_file(filename, "a");
    
    if (f == NULL) return -1;
    
    if ((locate_in_file_position(f, pos)) == -1) return -1; //nwrittenbytes
    
    printf("WRITE:El fichero es %s y el tamaño es %ld\n", filename, sizeof(data_msg));
    
    if (1 != fwrite(data_msg, strlen(data_msg), 1, f)){
        fprintf(stderr,"tftp.c: write_data_into_file: error in fwrite\n");
        return -1;
    }

    if (0 != fclose(f)){
        fprintf(stderr,"tftp.c: write_data_into_file: error in fclose\n");
        return -1;
    }
    
    return 0; // No pongas 1 porque luego no funciona porque coincide con el ACK mandado 
}

FILE* open_file(char *filename, char *file_mode)
{
    FILE *f;
    
    if ((f = fopen(filename, file_mode)) == NULL) {
        fprintf(stderr,"tftp.c: create_file: could not open file to read\n");
        return NULL;
    }
    
    return f;
    
}

int locate_in_file_position(FILE *f, int pos)
{
    if (0 != fseek(f, pos, SEEK_SET)) {
        fprintf(stderr,"tftp.c: locate_in_file_position: error in fseek\n");
        return -1;
    }
    
    return 1;
}


