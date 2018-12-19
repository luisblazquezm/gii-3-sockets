/*
** Fichero: tftp.c
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

char log_format[] = "%-24s %-15s %-15s %-8s %-5s %-25s %-40s\n";

char TCP_PROTOCOL_STR_DES[] = "TCP";
char UDP_PROTOCOL_STR_DES[] = "UDP";

char LOG_HEADER_TIME[] = "DATE";
char LOG_HEADER_CLIENT_HOST[] = "HOST";
char LOG_HEADER_IP_ADDRESS[] = "IP ADDRESS";
char LOG_HEADER_PROTOCOL[] = "PROTOCOL";
char LOG_HEADER_PORT[] = "PORT";
char LOG_HEADER_OPERATION[] = "OPERATION";
char LOG_HEADER_ERROR_DESCRIPTION[] = "ERROR";

void wrq_op(char *filename, char *msg)
{
    if (filename == NULL || msg == NULL) return;
    
    sprintf(msg, "wrq: '%s'", filename);
}

void rrq_op(char *filename, char *msg)
{
    if (filename == NULL || msg == NULL) return;
    
    sprintf(msg, "rrq: '%s'", filename);
}

void suc_op(char *filename, char *msg)
{
    if (filename == NULL || msg == NULL) return;
    
    sprintf(msg, "%s: all done", filename);
}

void client_get_filepath(char *filename, char *path)
{
    char default_folder[] = "ficherosTFTPclient";
    
    if (filename == NULL || path == NULL) return;
    
    sprintf(path, "%s/%s", default_folder, filename);
}

void server_get_filepath(char *filename, char *path)
{
    char default_folder[] = "ficherosTFTPserver";
    
    if (filename == NULL || path == NULL) return;
    
    sprintf(path, "%s/%s", default_folder, filename);
}

char *client_log_filename(uint16_t port)
{
    char *name;
    
    if ((name = malloc(sizeof(10*sizeof(char)))) == NULL){
        return NULL;
    }
    
    sprintf(name,"%d.txt", port);
    
    return name;
}

void inc_nblock(char *nblock)
{
    if (nblock == NULL || strlen(nblock) < 2 || strlen(nblock) > 2){
        return;
    } else {
        if (nblock[0] == '9' && nblock[1] == '9') {
            nblock[0] = '0', nblock[1] = '0';
        } else if (nblock[1] < '9') {
            ++nblock[1];
        } else if (nblock[1] == '9') {
            ++nblock[0];
            nblock[1] = '0';
        }
    }
}

int printmtof(char *msg, char *filename)
{
    FILE *ptr = NULL;

    if (msg == NULL || filename == NULL){
        return -1;
    }
    
    if ((ptr = fopen(filename, "a")) == NULL) {
        perror("printmtof: could not open file");
        return -1;
    }
    
    if (1 != fwrite(msg, strlen(msg), 1, ptr)) {
        return -1;
    }
    
    if (1 != fwrite("\n", sizeof(char), 1, ptr)) {
        return -1;
    }
    
    if (fclose(ptr) == EOF) {
        return -1;
    }
    
    return 0;
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

char* create_data_msg(char* block, char *data)
{
	char *packet;
	char temp[3];
	packet = calloc(1, 4 + strlen(data));
	strcat(packet, "03");
	strcat(packet, block);
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

int wait_ack(int s, char *buffer, struct sockaddr_in* clientaddr_in, int addr_len) 
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
    
    return recvfrom(s, buffer, TAM_BUFFER-1, 0, (struct sockaddr *)clientaddr_in, &addr_len);
}


int read_from_file(char *data, char *filename, int pos)
{
    FILE *f = open_file(filename, "r");
    int bytes_left = 0;
	
    if (f == NULL) return -1;
    
    fseek(f, 0, SEEK_END);
    bytes_left = ftell(f);
     
    if ((locate_in_file_position(f, pos)) == -1) return -1; //nwrittenbytes
    
    bytes_left -= ftell(f);
    
     if (1 != fread(data, TFTP_DATA_SIZE, 1, f)){
        if (ferror(f)) {
            fprintf(stderr,"tftp.c: read_from_file: error in fread");
            return -1;
        } else if (feof(f)) { // In this case bytes_left < 512 bytes
            data[bytes_left] = '\0';
            return 1;
        }
     }
     
     data[TFTP_DATA_SIZE] = '\0';
     
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

int init_log_file(char* filename)
{
    
    char buf[1000];
    FILE *ptr;
    
    if (filename == NULL){
        return -1;
    }
    
    // Check if file exists; create otherwise
    if(access(filename, F_OK) == -1) {
        if ((ptr = fopen(filename, "w")) == NULL){
            perror("init_log_file: error opening file");
            return -1;
        }
        
        snprintf(buf, sizeof(buf), log_format,
                 LOG_HEADER_TIME,
                 LOG_HEADER_CLIENT_HOST,
                 LOG_HEADER_IP_ADDRESS,
                 LOG_HEADER_PROTOCOL,
                 LOG_HEADER_PORT,
                 LOG_HEADER_OPERATION,
                 LOG_HEADER_ERROR_DESCRIPTION);
        
        if (fwrite(buf, strlen(buf), 1, ptr) != 1){
            printf("init_log_file: error writing to file \'%s\'\n", filename);
            return -1;
        }
        
        if (fclose(ptr) == EOF){
            perror("init_log_file: error closing file");
            return -1;
        }
    }
    
    return 0;
}

int write_log_data(char *client_hostname,
                   char *ipaddr,
                   char *protocol,
                   char *port,
                   char *operation,
                   char *error_description,
                   char *filename)
{
    int i;
    char buf[1000];
    time_t currtime;
    char s_currtime[100];
    FILE *ptr;

    if (client_hostname == NULL
    || ipaddr == NULL
    || protocol == NULL
    || port == NULL
    || operation == NULL
    || error_description == NULL
    || filename == NULL){
        return -1;
    }

    if ((ptr = fopen(filename, "a")) == NULL){
        perror("write_log_data: error opening file");
        return -1;
    }
    
    currtime = time(NULL);
    
    // Get rid of ctime's '\n'
    strncpy(s_currtime, ctime(&currtime), sizeof(s_currtime));
    for (i = 0; i < strlen(s_currtime); ++i){
        if (s_currtime[i] == '\n')
            s_currtime[i] = '\0';
        else if (s_currtime[i] == '\0')
            break;
    }

    snprintf(buf, sizeof(buf), log_format,
             s_currtime,
             client_hostname,
             ipaddr,
             protocol,
             port,
             operation,
             error_description);
             
    if (fwrite(buf, strlen(buf), 1, ptr) != 1){
        printf("write_log_data: error writing to file \'%s\'\n", filename);
        return -1;
    }
    
    if (fclose(ptr) == EOF){
        perror("write_log_data: error closing file");
        return -1;
    }
}


