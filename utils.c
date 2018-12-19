/*
** Fichero: utils.c
** Autores:
** Luis Blázquez Miñambres DNI 70910465Q
** Samuel Gómez Sánchez    DNI 45136357F
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"

/***************************************
*  test_args                          *
***************************************
*                                     *
*  Comprueba que los argumentos       *
*  recibidos por CLI tienen el        *
*  formato apropiado.                 *
*                                     *
***************************************/
int test_args(int argc, char * argv[])
{
	
	// There might be necessary to do some checking on host's name
	if (argv == NULL) {
		return -1;
	} else if (argc != 5) {
		
		if (argc == 2 && !strcmp(argv[1], "--help")) {
			help_msg();
			return 1;
		} else {
			short_help_msg();
			return -1;
		}
	} else if (strcmp(argv[2], "TCP") && strcmp(argv[2], "UDP")) {
		invalid_option_msg(argv[2]);
		return -1;
	} else if (strcmp(argv[3], "r") && strcmp(argv[3], "w")) { // Invalid
		invalid_option_msg(argv[3]);
		return -1;
	}
	
	return 0;
}

/************************************
*  invalid_option_msg              *
************************************
*                                  *
*  Imprime un mensaje de error     *
************************************/
int invalid_option_msg(char * opt)
{
	char msg[] = "cliente: invalid option -- '%s'\n\
Try 'cliente --help' for more information.\n";

	return printf(msg, opt);
}

/************************************
*  short_help_msg                  *
************************************
*                                  *
*  Imprime un mensaje de ayuda     *
*  corto                           *
************************************/
int short_help_msg(void)
{

	char msg[] = "Usage: cliente <HOST> <PROTOCOL> r|w filename\n\
Try 'cliente --help' for more information.\n";

	return printf("%s", msg);

}

/************************************
*  help_msg                        *
************************************
*                                  *
*  Imprime un mensaje de ayuda     *
************************************/
int help_msg(void)
{

	char msg[] = "Usage: cliente <HOST> <PROTOCOL> r|w filename\n\n\
Launches TFTP client in host <HOST> for protocol <PROTOCOL> in order\n\
to send file with name 'filename'\n\n\
    PROTOCOL            Transport Protocol executed in the communication,\n\
                        either 'TCP' or 'UDP'\n\n\
    HOST                IP address of the machine in which the client\n\
                        program will run\n\n\
    r                   Client's reading mode\n\n\
    w                   Client's writing mode\n\n\
    filename            Name of the file to be transmitted\n";

	return printf("%s", msg);
}
