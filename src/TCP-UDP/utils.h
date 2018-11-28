#ifndef __UTILS_H
#define __UTILS_H

#include <stdio.h>
#include <stdlib.h>

#define NULL_PTR_ERROR          -1

#define LOG_EMPTY_FILENAME_ERR 	1
#define OPEN_FILE_ERROR 	2
#define WRITE_FILE_ERROR 	3
#define READ_FILE_ERROR 	4
#define CLOSE_FILE_ERROR        5

#define TCP_PROTOCOL_CODE       0
#define UDP_PROTOCOL_CODE       1

char* strfmt(const char* format, ...);
int test_args(int argc, char *argv[]);
int invalid_option_msg(char * opt);
int short_help_msg(void);
int help_msg(void);

#endif
