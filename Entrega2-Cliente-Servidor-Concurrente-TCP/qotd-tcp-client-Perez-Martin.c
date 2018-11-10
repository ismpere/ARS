// Practica tema 6, Pérez Martín Ismael

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>

int main(int argc, char** argv){

    //Se crea el array que albergara la respuesta
    char respuesta[512];

    //Se comprueba la cantidad de argumentos
    if (argc != 4 && argc != 2) {
        printf("Missing argument\n");
        exit(EXIT_FAILURE);
    }

    //Creo las variables para almacenar la direccion local
    struct in_addr addr;
    //Creo la estructura para almacenar el servicio por nombre
    struct servent *serv;
    //Se crea la variable que albergará el puerto
    int puerto;

    //Se comprueba los argumentos introducidos
    if(argc ==4 || argc == 2){
        //Se comprueba si la IP es valida
        if(inet_aton(argv[1], &addr)==0){
            printf("IP not valid\n");
            exit(EXIT_FAILURE);
        }
    }else{
        printf("Missing argument\n");
        exit(EXIT_FAILURE);
    }

    //Se comprueba si se ha introducido bien la opcion y el puerto
    if(argc == 4){
        if(strcmp(argv[2], "-p")!=0){
            printf("Format not valid\n");
            exit(EXIT_FAILURE);
        //Se comprueba el puerto
        }else if(sscanf(argv[3],"%d",&puerto)!=1){
            printf("Port not valid\n");
            exit(EXIT_FAILURE);
        }else{
            //Pasamos el puerto a order by network
            puerto = htons(puerto);
        } 
    }else{
        //Si no hay puerto, lo extraemos mediante el nombre del servicio y el protocolo
        serv = getservbyname("qotd", "udp");
        if (serv == NULL) {
            printf("Service qotd not found for protocol udp\n");
            exit(EXIT_FAILURE);
        }else{
            //Extraemos el numero de puerto de la estructura
            puerto = serv->s_port;
        }
    }


    //Se crean las variables para albergar el socket y los errores
    int sock;
    int err;

    //Se crea e inicializa la estructura que albergara los datos de  la direccion local
    struct sockaddr_in myaddr;

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = 0;
    myaddr.sin_addr.s_addr = INADDR_ANY;

    //Creo el socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock<0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    //Se bindea el socket
    err = bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr));

    if(err<0){
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    //Se crea e inicializa la estructura de la dirección destino
    struct sockaddr_in servaddr;

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = puerto;
    servaddr.sin_addr.s_addr = addr.s_addr;


    // Se conecta al servidor
    err = connect(sock, (struct sockaddr *) &servaddr, sizeof(servaddr));

    if(err<0){
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    //Se recibe la respuesta con recv ya que no nos hace falta saber los datos del remitente
    err = recv(sock, respuesta, 512, 0);

    if(err<0){
        perror("recv()");
        exit(EXIT_FAILURE);
    }

    //Se imprime la respuesta recibida del servidor
    printf("%s\n", respuesta);

    return(EXIT_SUCCESS);
}