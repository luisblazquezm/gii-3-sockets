#ifndef __CLIENT_TCP_H
#define __CLIENT_TCP_H

#define PUERTO 17278
#define TAM_BUFFER 512

/* Esta todo en el utils */

int test_args(int argc, char *argv[]);
int invalid_option_msg(char * opt);
int short_help_msg(void);
int help_msg(void);

#endif
