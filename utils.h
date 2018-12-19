/*
** Fichero: utils.h
** Autores:
** Luis Blázquez Miñambres DNI 70910465Q
** Samuel Gómez Sánchez    DNI 45136357F
*/

#ifndef __UTILS_H
#define __UTILS_H

#include <stdio.h>
#include <stdlib.h>

int test_args(int argc, char *argv[]);
int invalid_option_msg(char * opt);
int short_help_msg(void);
int help_msg(void);

#endif
