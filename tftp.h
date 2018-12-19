/*
** Fichero: tftp.h
** Autores:
** Luis Blázquez Miñambres DNI 70910465Q
** Samuel Gómez Sánchez    DNI 45136357F
*/


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
#include "utils.h"

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

#define TIMEOUT_N_TRIES 10

#define TFTP_DATA_SIZE 512
#define TAM_BUFFER 1024

#define HOSTNAME_LEN    100
#define IPADDR_LEN      16
#define PTCL_LEN        4
#define PORT_LEN        6
#define OP_DESC_LEN     1000
#define ERR_DESC_LEN    1000

#define LOG_FILENAME "peticiones.log"

// TFTP messages creation
char* create_r_msg(char *filename);
char* create_w_msg(char *filename);
char* create_data_msg(char* block, char *data);
char* create_ack_msg(char* block);
char* create_error_msg(char *errcode, char* errmsg);

// Functionality wrapping
int wait_ack(int s, char *buffer, struct sockaddr_in* clientaddr_in, int addr_len);
void inc_nblock(char *nblock);

// Domain-specific file-handling functions
int read_from_file(char *data_msg, char *filename, int pos);
int write_data_into_file(char *data_msg, char *filename, int pos);
FILE* open_file(char *filename, char *file_mode);
int locate_in_file_position(FILE *f, int pos);
void client_get_filepath(char *filename, char *path);
void server_get_filepath(char *filename, char *path);
int printmtof(char *msg, char *filename);

// Logfile-related functions
int init_log_file(char* filename);
int write_log_data(char *client_hostname,
                   char *ipaddr,
                   char *protocol,
                   char *port,
                   char *operation,
                   char *error_description,
                   char *filename);
void wrq_op(char *filename, char *msg);
void rrq_op(char *filename, char *msg);
void suc_op(char *filename, char *msg);
char *client_log_filename(uint16_t port);

#endif

