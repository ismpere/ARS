#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>

int main(int argc, char** argv){

    //char name[10] = "QOTD";
    //char proto[10] = "UDP";

    //Se crean los array que albergaran los mensajes y respuestas
    char mensaje[100] = "Mensaje de prueba";
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
    //int puerto_introducido;
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
    sock = socket(AF_INET, SOCK_DGRAM, 0);

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
    struct sockaddr_in dest_addr;

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = puerto;
    dest_addr.sin_addr.s_addr = addr.s_addr;

    //Se envia el mensaje
    err = sendto(sock, mensaje, 512, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));

    if(err<0){
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

    //Se recibe la respuesta
    err = recv(sock, respuesta, 512, 0);

    if(err<0){
        perror("recv()");
        exit(EXIT_FAILURE);
    }

    //Se imprime la respuesta recibida del servidor
    printf("Respuesta: %s\n", respuesta);
}