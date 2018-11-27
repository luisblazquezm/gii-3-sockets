#include <stdarg.h>
#include "utils.h"

char* strfmt(const char* format, ...)
{
	char *		str;
	va_list		args;

	if ((str = malloc(100 * sizeof(char))) == NULL)
		return NULL;


	va_start(args, format);
	vsnprintf(str, sizeof(str), format, args); // do check return value
	va_end(args);

	return str;
}

int test_args(int argc, char *argv[])
{
    int i, j; // Indices

    /* Para almacenar las opciones D, PA y PD bitwise.
     * Así:
     *      100 -> D
     *      010 -> PA
     *      001 -> PD
     *      101 -> D y PD
     * etc.
     */
    int options = 0;

    if (argc < 2){

        invalid_option_msg("None");
        return -1;

    } else {

        if (argc == 2) {

            if (!strcmp(argv[1], "--help"))
                help_msg();
            else
                short_help_msg();

            return -1;

        } else if (argc >= 3){

            // Comprobamos si los primeros argumentos son numeros
            i = 0;
            while (argv[1][i] != '\0'){
                if (!isdigit(argv[1][i]))
                    return -1;
                ++i;
            }

            i = 0;
            while (argv[2][i] != '\0'){
                if (!isdigit(argv[2][i]))
                    return -1;
                ++i;
            }

            /* Comprobamos que el resto de argumentos no producen combinaciones
             * no validas:
             *      - No se repiten opciones
             *      - No se dan PA y PD a la vez
             */
            for (i = 3; i < argc; ++i){

                j = 0;
                while (argv[i][j] != '\0'){
                    argv[i][j] = toupper(argv[i][j]);
                    ++j;
                }

                if (!strcmp(argv[i], "D")){
                    if (options & 0x4){ // Test bitwise
                        invalid_option_msg(argv[i]);
                        return -1;
                    } else {
                        options |= 0x4; // Set bitwise
                    }
                } else if (!strcmp(argv[i], "PA")){
                    if ((options & 0x1) || (options & 0x2)) {
                        invalid_option_msg(argv[i]);
                        return -1;
                    } else {
                        options |= 0x2;
                    }
                } else if (!strcmp(argv[i], "PD")){
                    if ((options & 0x2) || (options & 0x1)) {
                        invalid_option_msg(argv[i]);
                        return -1;
                    } else {
                        options |= 0x1;
                    }
                } else {
                    invalid_option_msg(argv[i]);
                    return -1;
                }
            }
        }

        return options;
    }
}

int invalid_option_msg(char * opt)
{
    char msg[1000] = "cliente: invalid option -- '%s'\n\
Try 'cliente --help' for more information.\n"; 

    return printf(msg, opt);
}

int short_help_msg(void)
{

    char msg[1000] = "Usage: cliente olivo P [r | l] file_name\n\
Try 'cliente --help' for more information.\n"; 
                         
    return printf("%s", msg);

}

int help_msg(void)
{

    char msg[1000] = "Usage:cliente olivo P [r | l] file_name\n\
Simulates a process  allocation  system with  an  interface  that  emulates a\n\
road with cars which must be parked in a ordered fashion.\n\
    P                   Transport Protocol executed in the communication,\n\
			it can be TCP or UDP\n\
    r                   Selects reading process\n\
    w                   Selects writing process\n\
    file_name           Name of the file that allocates the data to be written o read.\n"; 
                         
    return printf("%s", msg);
}





