#ifndef __TFTP_H
#define __TFTP_H

#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#define WRONG_MSG_TYPE  -1

#define NETASCII_MODE	0
#define OCTET_MODE	1
#define MAIL_MODE	2

#define READ_TYPE       1
#define WRITE_TYPE      2
#define DATA_TYPE       3
#define ACK_TYPE        4
#define ERROR_TYPE      5

#define TCP_TYPE        1
#define UDP_TYPE        2

#define TIMEOUT_N_TRIES 5

#define TFTP_DATA_SIZE 512
#define TAM_BUFFER 1024

void nblock_to_str(char *str, int n);
char* create_r_msg(char *filename);
char* create_w_msg(char *filename);
char* create_data_msg(int block, char *data);
char* create_ack_msg(char* block);
char* create_error_msg(char *errcode, char* errmsg);

int wait_ack(int s, char *buffer, struct sockaddr_in clientaddr_in, int addr_len);


int read_from_file(char *data_msg, char *filename, int pos);
int write_data_into_file(char *data_msg, char *filename, int pos);
FILE* open_file(char *filename, char *file_mode);
int locate_in_file_position(FILE *f, int pos);

#endif

