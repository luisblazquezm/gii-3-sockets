#ifndef __TFTP_H
#define __TFTP_H

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

#endif

