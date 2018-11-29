#ifndef __LOG_H
#define __LOG_H

#include <time.h>
#include <stdint.h>
#include "../utils/utils.h"

typedef int protocol_code_t;
typedef long ipadd_t;
typedef int port_number_t;

struct log_data
{
    time_t time;
    char client_host[100];
    ipadd_t ip_address;
    protocol_code_t protocol_code;
    port_number_t port;
    char operation[1000];
    char error_description[1000];
    
};

char *protocol_tostring(int protocol_code);
struct log_data *create_log_data(time_t time,
                            char *client_host,
                            ipadd_t ip_address,
                            protocol_code_t protocol_code,
                            port_number_t port,
                            char *operation,
                            char *error_description);
int init_log_file(char* filename);
int write_log_data(struct log_data *log_data, char *filename);


#endif
