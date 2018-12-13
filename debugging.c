#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debugging.h"

int printmtof(char *msg, char *filename)
{
    FILE *ptr = NULL;

    if (msg == NULL || filename == NULL){
        return -1;
    }
    
    if ((ptr = fopen(filename, "a")) == NULL) {
        perror("debugging.c: printmtof: could not open file");
        return -1;
    }
    
    if (1 != fwrite(msg, strlen(msg), 1, ptr)) {
        return -1;
    }
    
    if (1 != fwrite("\n", sizeof(char), 1, ptr)) {
        return -1;
    }
    
    if (fclose(ptr) == EOF) {
        return -1;
    }
    
    return 0;
}
