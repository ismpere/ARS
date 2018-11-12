// Practica tema 6, Pérez Martín Ismael

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>

#include <netdb.h>

#define MAXLENGTH 512

// Se declaran las variables globales para albergar los sockets
int sock;
int sock_connect;

//Se declara el manejador de señal para cerrar los socket
void signal_handler(int signal);


//Implementación del manejador de señal para el cierre de socket
void signal_handler(int signal){

    //Si se pulsa ctrl+c, se desconectan los socket
    if(signal == SIGINT){

        int err;

        //Se apaga el socket de de escucha
        err = shutdown(sock, SHUT_RDWR);

        if(err<0){
                perror("shutdown() sock");
                exit(EXIT_FAILURE);
        }

        //Se cierra el socket de escucha
        err = close(sock);

        if(err<0){
                perror("close() sock");
                exit(EXIT_FAILURE);
        }
        
        //Se apaga el socket de las conexiones aceptadas
        err = shutdown(sock_connect, SHUT_RDWR);

        if(err<0){
                perror("shutdown() sock_connect");
                exit(EXIT_FAILURE);
        }

        //Se apaga el socket de las conexiones aceptadas
        err = close(sock_connect);

        if(err<0){
                perror("close() sock_connect");
                exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char** argv){

    //Se vincula nuestro manejador de señal con la señal
    signal(SIGINT, signal_handler);

    //Se define la cabecera de la respuesta y los buffer para los mensajes
    char cabecera[100] = "Quote Of The Day from vm2509\n";

    char mensaje[512];

    static char buffQuote[MAXLENGTH];

    //Se comprueba el numero de argumentos introducidos
    if (argc != 3 && argc != 1) {
        printf("Missing argument\n");
        exit(EXIT_FAILURE);
    }

    //Se crean las estructuras para almacenar el puerto
    struct servent *serv;
    int puerto_introducido;
    int puerto;

    //Se comprueban las diferentes opciones de entrada
    if(argc == 3){
        //Se comprueba la opcion -p
        if(strcmp(argv[1], "-p")!=0){
            printf("Format not valid\n");
            exit(EXIT_FAILURE);
        //Se comprueba que es un puerto valido
        }else if(sscanf(argv[2],"%d",&puerto_introducido)!=1){
            printf("Port not valid\n");
            exit(EXIT_FAILURE);
        //Se pasa el puerto a network by order
        }else{
            puerto = htons(puerto_introducido);
        } 
    //Se extrae el puerto por nombre de servicio y protocolo
    }else{
        serv = getservbyname("qotd", "udp");
        if (serv == NULL) {
            printf("Service qotd not found for protocol udp\n");
            exit(EXIT_FAILURE);
        }else{
            puerto = serv->s_port;
        }
    }

    //Se crean las variables para almacenar el socket, errores y direccion destino
    int sock;
    int sock_connect;
    int err;
    struct sockaddr_in peer;

    //Se crea la variable para almacenar el tamaño de la direccion destino
    socklen_t addrlen = sizeof(peer);

    //Se crea e inicializa la estructura para almacenar nuestra direccion
    struct sockaddr_in myaddr;

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = puerto;
    myaddr.sin_addr.s_addr = INADDR_ANY;

    //Se crea el socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock<0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    //Se bindea el socket a mi direccion
    err = bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr));

    if(err<0){
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    //Se pone el servidor a escuchar
    err = listen(sock, 512);

    if(err<0){
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    //Se comienza el bucle del servidor iterativo
    while(1){

        //Se espera a una conexion
        sock_connect = accept(sock, (struct sockaddr *) &peer, &addrlen);

        if(sock_connect<0){
            perror("accept()");
            exit(EXIT_FAILURE);
        }

        //Se resetea el contenido del buffer de la cita
        memset(buffQuote, 0, sizeof(buffQuote));

        //Se extrae la cita del dia de fortune
        system("/usr/games/fortune -s > /tmp/tt.txt");
        FILE *fich = fopen("/tmp/tt.txt","r");
        int contador = 0;
        int nc = fgetc(fich);
        while(nc!=EOF){
            buffQuote[contador++] = nc;
            nc = fgetc(fich);
        }
        fclose(fich);

        //Se resetea el mensaje y se concatena con la cabecera y la cita del dia
        memset(mensaje, 0, sizeof(mensaje));
        strcat(mensaje,cabecera);
        strcat(mensaje, buffQuote);

        //Se envia el mensaje a la direccion de la que recibimos el mensaje con recvfrom
        err = send(sock_connect, mensaje, 512, 0);

        if(err<0){
            perror("send()");
            exit(EXIT_FAILURE);
        }
    }

    return(EXIT_SUCCESS);
}