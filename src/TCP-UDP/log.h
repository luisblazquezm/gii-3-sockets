#ifndef __LOG_H
#define __LOG_H

#include <time.h>
#include <stdint.h>
#include "utils.h"

typedef int protocol_code_t;
typedef uint_least64_t ipadd_t;
typedef int_least16_t port_number_t;

typedef struct log_data
{
    time_t time;
    char client_host[100];
    ipadd_t ip_address;
    protocol_code_t protocol_code;
    port_number_t port;
    char operation[1000];
    char error_description[1000];
    
} log_data_t;

char *protocol_tostring(int protocol_code);
int init_log_file(char* filename);
int write_log_data(log_data_t *log_data, char *filename);


#endif
