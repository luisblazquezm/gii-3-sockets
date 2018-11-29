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
    strcpy(filename, rw_msg->filename);
    rw_msg->byte_1 = '\0';
    strcpy("octet", rw_msg->mode);
    rw_msg->byte_2 = '\0';
    
    return rw_msg;
}


data_msg_t *create_data_msg(int n_block, char *data)
{
    data_msg_t *data_msg;
    
    // Only two modes are available, read and write, as defined in tftp.h
    if (msg_type != DATA_TYPE)
    {
        printf("%s\n", "create_data_msg: wrong message type");
        return NULL;
    } else if (n_block == 0) {
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
    if (msg_type != ACK_TYPE)
    {
        printf("%s\n", "create_ack_msg: wrong message type");
        return NULL;
    } else if (n_block == 0) {
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
    char error_msg[200];
    
    // Only two modes are available, read and write, as defined in tftp.h
    if (msg_type != ERROR_TYPE)
    {
        printf("%s\n", "create_error_msg: wrong message type");
        return NULL;
    } else if (error_code < 0) {
        printf("%s\n", "create_error_msg: invalid error_code ");
        return NULL;
    }
    
    if ((error_msg = malloc(sizeof(error_msg_t))) == NULL) {
        printf("%s\n", "create_error_msg: could not allocate resource");
        return NULL;
    }
    
    switch(error_code){
	case 0:
		strcpy("Not defined", error_msg);
		break;
	case 1:
		strcpy("File not found", error_msg);
		break;
	case 3:
		strcpy("Full disk", error_msg);
		break;
	case 4:
		strcpy("Ilegal operation on TFTP protocol", error_msg);
		break;
	case 6:
		strcpy("File already exists", error_msg);
		break;
	default:
		strcpy("Out of bounds error > 6", error_msg);
		break;
	
    }

    error_msg->msg_type = ERROR_TYPE;
    error_msg->error_code = error_code;
    strcpy(error_msg, error_msg->error_msg);
    error_msg->byte = '\0';

    return error_msg;
}
