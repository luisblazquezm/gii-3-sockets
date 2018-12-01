#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[]) {

    struct addrinfo hints, *res;
    
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
      
    errcode = getaddrinfo(argv[1], NULL, &hints, &res); 
    
    printf("
}
