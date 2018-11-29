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
    
    return rw_msg_t;
}
