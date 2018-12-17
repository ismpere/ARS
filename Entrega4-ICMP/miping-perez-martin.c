// Practica tema 8, Perez Martin Ismael

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>

#include <arpa/inet.h>

int main(int argc, char **argv){
    int debug;

    struct in_addr addr;  

    //Se comprueba la cantidad de argumentos
    if(argc < 2 ) {
        printf("Missing argument\n");
        exit(EXIT_FAILURE);
    }else if(argc > 3){
        printf("So many arguments\n");
        exit(EXIT_FAILURE);
    }
    
    //Se comprueba si la IP es valida
    if(inet_aton(argv[1], &addr)==0){
        printf("IP not valid\n");
        exit(EXIT_FAILURE);
    }

    //Se comprueba si se ha introducido la opcion de general el informe
    if(argc==3){
        if(strcmp(argv[2], "-v")!=0){
            printf("Option not valid\n");
            exit(EXIT_FAILURE);
        }else{
            debug = 1;
        }
    }else{
        debug = 0;
    }
}