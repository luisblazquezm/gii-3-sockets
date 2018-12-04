#ifndef __TFTP_H
#define __TFTP_H

#include <inttypes.h>
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

#define TFTP_DATA_SIZE 512

typedef uint16_t msg_type_t;
typedef uint16_t blockn_t;
typedef uint16_t error_code_t;

typedef struct rw_msg
{
    msg_type_t msg_type;
    char filename[1000];
    char byte_1;
    char mode[10];
    char byte_2;
    
} rw_msg_t;

typedef struct data_msg
{
    msg_type_t msg_type;
    blockn_t n_block;
    char data[TFTP_DATA_SIZE];
    
} data_msg_t;

typedef struct ack_msg
{
    msg_type_t msg_type;
    blockn_t n_block;
    
} ack_msg_t;

typedef struct error_msg
{
    msg_type_t msg_type;
    error_code_t error_code;
    char error_msg[1000];
    char byte;
    
} error_msg_t;

rw_msg_t *create_rw_msg(msg_type_t msg_type, char *filename);
ack_msg_t *create_ack_msg(blockn_t n_block);
data_msg_t *create_data_msg(blockn_t n_block);
error_msg_t *create_error_msg(error_code_t error_code);

int read_from_file(data_msg_t *data_msg, char *filename, int pos);
int write_data_into_file(data_msg_t data_msg, char *filename, int pos);
FILE* open_file(char *filename, char *file_mode);
int locate_in_file_position(FILE *f, int pos);

#endif

