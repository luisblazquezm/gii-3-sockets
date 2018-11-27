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
