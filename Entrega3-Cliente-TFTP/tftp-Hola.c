// Practica tema 7, Berruezo Franco Alvaro
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#define SIZENAME 100 //Tamaño máximo del nombreFichero del archivo y de errstring
#define BLOCKSIZE 512 //Tamaño máximo de datos que se enviarán en cada paquete

void error(char *errstr){
    /*Imprime errstr y finaliza el programa*/
    fprintf (stderr, "%s\n", errstr);
    exit(-1);
}

int bytesToInt(char *bytes){
    /*Toma un puntero (de char) a un entero de dos bytes y devuelve una variable de tipo int que coniene el entero*/
    int num = 0;
    num += (unsigned char) bytes[0]*256;
    num += (unsigned char) bytes[1];
    return num;
}

void intToBytes(int num, char *bytes){
    /*Toma una variable de tipo int y la almacena en un puntero (de char) como un entero de dos bytes*/
    bytes[0] = num/256;
    bytes[1] = num%256;
}

int main(int args, char **argv){
    struct in_addr addr;              //Estructura donde se almacena la addr del servidor (pasada como argumento)
    unsigned char opcode;           //Segundo byte del opcode
    char nombreFichero[SIZENAME];          //nombreFichero del archivo a enviar o recibir
    int informe;                      //1 = Imprimir informe de pasos; 0 = No
    
    int sock;                       //Identificador del socket
    struct sockaddr_in myaddr;    //Estructura que contiene la direcc de origen (cliente)
    struct sockaddr_in dest_addr;    //Estructura que contiene la direcc de destino (servidor)
    socklen_t len;                  //Longitud de la estructura dest_addr
    int puerto;
    
    char datagram[4+BLOCKSIZE];     //Datagrama para enviar y recibir datos 
    int size;                       //Tamaño del datagrama
    int ack;                        //Número de paquete esperado
    char *modo = "octet";           //Modo de lectura/escritura
    FILE *file;                     //Puntero al archivo para leer/escribir
    char *errcode[SIZENAME] =  {&datagram[4], //Si errcode es 0 se lee errstring, a partir del byte 4 del datagrama
                                "Fichero no encontrado.",
                                "Violación de acceso.",
                                "Espacio de almacenamiento lleno.",
                                "Operación TFTP ilegal.",
                                "Identificador de transferencia desconocido.",
                                "El fichero ya existe.",
                                "Usuario desconocido."}; 
    
    //LECTURA DE PARAMETROS
    if ((args == 4) || (args == 5)){
        inet_aton(argv[1], &addr); //Se almacena la dirección addr en la estructura
        sscanf(argv[3], "%s", nombreFichero); //Se almacena el nombreFichero del archivo
        puerto = getservbyname("tftp", "udp")->s_port; //El puerto se obtiene del sistema operativo
        
        if (! strcmp(argv[2], "-r")) //Se guarda el opcode
            opcode = 1;
        else if (! strcmp(argv[2], "-w"))
            opcode = 2;
        else
            error("Parámetro desconocido");
        
        if ((args == 5) && (! strcmp(argv[4], "-v"))) //Se comprueba si hay un cuarto argumento
            informe = 1;
        else
            informe = 0;
    } else
        error("Número de argumentos incorrecto");
    
    //APERTURA DEL SOCKET
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) error(strerror(errno));

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = 0; //Se asigna el puerto 0 para que el sistema operativo lo complete
    myaddr.sin_addr.s_addr = INADDR_ANY; //No se asigna addr, para que el sistema operativo elija tarjeta de red
    
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = puerto;
    dest_addr.sin_addr = addr;
    len = sizeof dest_addr;
    
    if(bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) error(strerror(errno));
        
    //DATAGRAMA INICIAL
    intToBytes(opcode, datagram); //Opcode (lectura o escritura)
    strcpy(&datagram[2], nombreFichero); //nombreFichero del archivo, con el EOS incluido
    strcpy(&datagram[2+strlen(nombreFichero)+1], modo); //Modo (octet o netascii), con el  EOS incluido
    if(sendto(sock, datagram, 2+strlen(nombreFichero)+1+strlen(modo)+1, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr)) == -1) error(strerror(errno));
    
    //LECTURA DE DATOS DEL SERVIDOR
    if (opcode == 1){
        if(informe) printf("Enviada solicitud de lectura de %s a servidor TFTP en %s.\n", argv[3], argv[1]);
        ack = 1;
        file = fopen(nombreFichero, "wb");
        do{
            tam = recvfrom(sock, datagrama, 4+TAMBLOQUE, 0, (struct sockaddr *) &dest_addr, &addrlen);

            if(tam<0){
                perror("recvfrom()");
                exit(EXIT_FAILURE);
            }

            if (bytesToInt(datagrama) == 3){

                if(informe){
                    printf("Recibido bloque del servidor tftp");
                }

                intToBytes(4, datagrama);
                if(bytesToInt(&datagrama[2]) == ack){
                    fwrite(&datagrama[4], 1, tam-4, file);
                    intToBytes(ack, &datagrama[2]);
                    ack++;
                }else{
                    intToBytes(ack-1, &datagrama[2]);
                }
                    
                err = sendto(sock, datagrama, 4, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));
                if(err<0){
                    perror("sendto()");
                    exit(EXIT_FAILURE);
                }

                if(informe){
                    printf("Enviado ACK del bloque %d.\n", ack-1);
                }
                
            }else if(bytesToInt(datagrama) == 5){
                //error(errcode[bytesToInt(&datagram[2])]);
                printf("ERORRRR")
            }
        }while(size == 4+BLOCKSIZE); //Al procesar un paquete de menos de BLOCKSIZE bytes, se finaliza
        
    //ESCRITURA DE DATOS EN EL SERVIDOR
    }else if (opcode == 2){
        if(informe) printf("Enviada solicitud de escritura en %s a servidor TFTP en %s.\n", argv[3], argv[1]);
        file = fopen(nombreFichero, "rb");
        
        if(recvfrom(sock, datagram, 4+SIZENAME, 0, (struct sockaddr *) &dest_addr, &len) == -1) error(strerror(errno));
        if (bytesToInt(datagram) == 4){ //El datagrama es un ACK
            if(informe) printf("Recibido ACK del bloque %d.\n", bytesToInt(&datagram[2]));
            do{
                ack = bytesToInt(&datagram[2]);                 //Se guarda el número de paquete que quiere el servidor
                fseek(file, BLOCKSIZE*ack, SEEK_SET);           //Se coloca el puntero al fichero en la posición correspondiente
                intToBytes(3, datagram);                        //Se crea un datagrama de datos 
                intToBytes(ack+1, &datagram[2]);                //Se escribe el número de bloque que se va a enviar
                size = fread(&datagram[4], 1, BLOCKSIZE, file); //Se escriben los datos
                if(sendto(sock, datagram, 4+size, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr)) == -1) error(strerror(errno));
                if(informe) printf("Enviado bloque %d.\n", ack+1);
            
                if(recvfrom(sock, datagram, 4+SIZENAME, 0, (struct sockaddr *) &dest_addr, &len) == -1) error(strerror(errno));
                if (bytesToInt(datagram) == 5) error(errcode[bytesToInt(&datagram[2])]);
                if(informe) printf("Recibido ACK del bloque %d.\n", bytesToInt(&datagram[2]));
            }while ((size == BLOCKSIZE) || (bytesToInt(&datagram[2]) != ack+1)); //Se finaliza cuando se lee el último bloque y se recibe su ack
        
        } else if (bytesToInt(datagram) == 5) //El datagrama contiene un mensaje de error
            error(errcode[bytesToInt(&datagram[2])]);
    }
        
    if(informe) printf("Era el último bloque. Cerramos el fichero.\n");
    fclose(file);
    return 0;
}
