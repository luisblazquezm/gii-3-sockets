#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "log/log.h"

int main (int argc, char *argv[]) {

    char filename[1000] = "log.txt";
    time_t current_time;
    char hostname[1024];
    hostname[1023] = '\0';
    
    time(&current_time);
    gethostname(hostname, 1023);
    
    struct log_data *log_data = create_log_data(current_time,
                                           hostname,
                                           0x7f000001L,
                                           TCP_PROTOCOL_CODE,
                                           1069,
                                           "w(some_file)",
                                           "");
    
    init_log_file(filename);
    write_log_data(log_data, filename);
    
    return 0;
}
