#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tftp.h"


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
    strcpy(rw_msg->filename, filename);
    rw_msg->byte_1 = '\0';
    strcpy(rw_msg->mode, "octet");
    rw_msg->byte_2 = '\0';
    
    return rw_msg;
}


data_msg_t *create_data_msg(int n_block)
{
    data_msg_t *data_msg;
    
    // Only two modes are available, read and write, as defined in tftp.h
    if (n_block <= 0) {
        printf("%s\n", "create_data_msg: number of block incorrect");
        return NULL;
    } 
    
    if ((data_msg = calloc(1, sizeof(data_msg_t))) == NULL) {
        printf("%s\n", "create_data_msg: could not allocate resource");
        return NULL;
    }
    
    data_msg->msg_type = DATA_TYPE;
    data_msg->n_block = n_block;
    
    return data_msg;
}

ack_msg_t *create_ack_msg(int n_block)
{
    ack_msg_t *ack_msg;
    
    // Only two modes are available, read and write, as defined in tftp.h
    if (n_block <= 0) {
        printf("%s\n", "create_ack_msg: number of block incorrect");
        return NULL;
    }
    
    if ((ack_msg = malloc(sizeof(ack_msg_t))) == NULL) {
        printf("%s\n", "create_ack_msg: could not allocate resource");
        return NULL;
    }
    
    ack_msg->msg_type = ACK_TYPE;
    ack_msg->n_block = n_block;
    
    return ack_msg;
}

error_msg_t *create_error_msg(int error_code)
{
    error_msg_t *error_msg;
    char error_str[200];
    
    // Only two modes are available, read and write, as defined in tftp.h
    if (error_code < 0 || error_code > 7) {
        printf("%s\n", "create_error_msg: invalid error_code ");
        return NULL;
    }
    
    if ((error_msg = malloc(sizeof(error_msg_t))) == NULL) {
        printf("%s\n", "create_error_msg: could not allocate resource");
        return NULL;
    }
    
    switch(error_code){
	case 0:
		strcpy(error_str, "Not defined");
		break;
	case 1:
		strcpy(error_str, "File not found");
		break;
	case 3:
		strcpy(error_str, "Full disk");
		break;
	case 4:
		strcpy(error_str, "Ilegal operation on TFTP protocol");
		break;
	case 5:
		strcpy(error_str, "Unknown transfer ID");
		break;
	case 6:
		strcpy(error_str, "File already exists");
		break;	
	case 7:
		strcpy(error_str, "No such user");
		break;
    }

    error_msg->msg_type = ERROR_TYPE;
    error_msg->error_code = error_code;
    strcpy(error_msg->error_msg, error_str);
    error_msg->byte = '\0';

    return error_msg;
}