// Practica tema 7, Perez Martin Ismael

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>

#define TAMNOMBRE 100
#define TAMBLOQUE 512

int main(int argc, char** argv){

    char nombreFichero[TAMNOMBRE];
    unsigned char opcode;
    int debug;

    //Se comprueba la cantidad de argumentos
    if (argc != 4 && argc != 5) {
        printf("Missing argument\n");
        exit(EXIT_FAILURE);
    }

    //Creo las variables para almacenar la direccion local
    struct in_addr addr;
    //Creo la estructura para almacenar el servicio por nombre
    struct servent *serv;
    //Se crea la variable que albergará el puerto
    int puerto;
    
    //Se comprueba si la IP es valida
    if(inet_aton(argv[1], &addr)==0){
        printf("IP not valid\n");
        exit(EXIT_FAILURE);
    }

    //Se comprueba si se ha introducido bien la opcion de lectura/escritura
    if(strcmp(argv[2], "-r")==0){
        opcode = 1;
    }else if(strcmp(argv[2], "-w")==0){
        opcode = 2;
    }else{
        printf("Select opcion between -w or -r\n");
        exit(EXIT_FAILURE);
    }

    sscanf(argv[3],"%s",nombreFichero);

    //Se comprueba si se ha introducido la opcion de general el informe
    if(argc==5){
        if(strcmp(argv[4], "-v")!=0){
            printf("Option not valid\n");
            exit(EXIT_FAILURE);
        }else{
            debug = 1;
            printf("Cinco argumentos\n");
        }
    }else{
        debug = 0;
        printf("Cuatro argumentos\n");
    }
}