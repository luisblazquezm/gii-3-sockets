#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "../utils/utils.h"

char log_format[] = "%-30s%-15s%-15s%-5s%-10s%-20s%-20s\n";

char TCP_PROTOCOL_STR_DES[] = "TCP";
char UDP_PROTOCOL_STR_DES[] = "UDP";

char LOG_HEADER_TIME[] = "DATE";
char LOG_HEADER_CLIENT_HOST[] = "HOST";
char LOG_HEADER_IP_ADDRESS[] = "IP ADDRESS";
char LOG_HEADER_PROTOCOL[] = "PROTOCOL";
char LOG_HEADER_PORT[] = "PORT";
char LOG_HEADER_OPERATION[] = "OPERATION";
char LOG_HEADER_ERROR_DESCRIPTION[] = "ERROR";

char *protocol_tostring(int protocol_code)
{
    switch(protocol_code){
    case TCP_PROTOCOL_CODE:
        return TCP_PROTOCOL_STR_DES;
    break;
    case UDP_PROTOCOL_CODE:
        return UDP_PROTOCOL_STR_DES;
    break;
    default:
        return "UNKNOWN";
    break;
    }
}

struct log_data *create_log_data(time_t time,
                            char *client_host,
                            ipadd_t ip_address,
                            protocol_code_t protocol_code,
                            port_number_t port,
                            char *operation,
                            char *error_description)
{
    struct log_data *log_data = NULL;
    if ((log_data = malloc(sizeof(struct log_data))) == NULL) {
        perror("log.c: create_log_data: alloc error");
        return NULL;
    }
    
    log_data->time = time;
    if (client_host != NULL){
        strcpy(client_host, log_data->client_host);
    } else {
        strcpy("", log_data->client_host);
    }
    log_data->ip_address = ip_address;
    log_data->protocol_code = protocol_code;
    log_data->port = port;
    if (operation != NULL){
        strcpy(operation, log_data->operation);
    } else {
        strcpy("", log_data->operation);
    }
    if (client_host != NULL){
        strcpy(error_description, log_data->error_description);
    } else {
        strcpy("", log_data->error_description);
    }
    
    return log_data;
}

int init_log_file(char* filename)
{
    
    char buf[1000];
    FILE *ptr;
    
    if (filename == NULL){
        return LOG_EMPTY_FILENAME_ERR;
    }
    
    if ((ptr = fopen(filename, "w")) == NULL){
        perror("init_log_file: error opening file");
        return OPEN_FILE_ERROR;
    }
    
    snprintf(buf,
             sizeof(buf),
             log_format, LOG_HEADER_TIME,
             LOG_HEADER_CLIENT_HOST,
             LOG_HEADER_IP_ADDRESS,
             LOG_HEADER_PROTOCOL,
             LOG_HEADER_PORT,
             LOG_HEADER_OPERATION,
             LOG_HEADER_ERROR_DESCRIPTION);
    
    if (fwrite(buf, strlen(buf), 1, ptr) != 1){
        printf("init_log_file: error writing to file \'%s\'\n", filename);
        return WRITE_FILE_ERROR;
    }
    
    if (fclose(ptr) == EOF){
        perror("init_log_file: error closing file");
        return CLOSE_FILE_ERROR;
    }

}

int write_log_data(struct log_data *log_data, char *filename)
{
    char buf[1000];
    FILE *ptr;

    if (log_data == NULL || filename == NULL){
        return NULL_PTR_ERROR;
    }
    
    if ((ptr = fopen(filename, "w")) == NULL){
        perror("write_log_data: error opening file");
        return OPEN_FILE_ERROR;
    }
    
    snprintf(buf,
             sizeof(buf),
             log_format,
             ctime(&(log_data->time)),
             log_data->client_host,
             "127.0.0.1",
             protocol_tostring(log_data->protocol_code),
             strfmt("%s", log_data->port),
             log_data->operation,
             log_data->error_description);
                      
    if (fwrite(buf, strlen(buf), 1, ptr) != 1){
        printf("write_log_data: error writing to file \'%s\'\n", filename);
        return WRITE_FILE_ERROR;
    }
    
    if (fclose(ptr) == EOF){
        perror("write_log_data: error closing file");
        return CLOSE_FILE_ERROR;
    }
}
