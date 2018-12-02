// Practica tema 7, Berruezo Franco Alvaro
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#define TAMNOMBRE 100 //Tamaño máximo del nombreFichero del archivo y de errstring
#define TAMBLOQUE 512 //Tamaño máximo de datos que se enviarán en cada paquete

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

int main(int argc, char **argv){
    struct in_addr addr;              //Estructura donde se almacena la addr del servidor (pasada como argumento)
    unsigned char opcode;           //Segundo byte del opcode
    char nombreFichero[TAMNOMBRE];          //nombreFichero del archivo a enviar o recibir
    int informe;                      //1 = Imprimir informe de pasos; 0 = No
    
    int sock;                       //Identificador del socket
        //Estructura que contiene la direcc de origen (cliente)
    struct sockaddr_in dest_addr;    //Estructura que contiene la direcc de destino (servidor)
                  //Longitud de la estructura dest_addr
    int puerto;
    int err;
    
    char datagrama[4+TAMBLOQUE];     //datagrama para enviar y recibir datos 
    int tam;                       //Tamaño del datagrama
    int ack;                        //Número de paquete esperado
    char *modoTftp = "octet";           //modoTftp de lectura/escritura
    FILE *file;                     //Puntero al archivo para leer/escribir
    char *codigoError[TAMNOMBRE] =  {&datagrama[4], //Si codigoError es 0 se lee errstring, a partir del byte 4 del datagrama
                                "Fichero no encontrado.",
                                "Violación de acceso.",
                                "Espacio de almacenamiento lleno.",
                                "Operación TFTP ilegal.",
                                "Identificador de transferencia desconocido.",
                                "El fichero ya existe.",
                                "Usuario desconocido."}; 
    
    //Se comprueba la cantidad de argumentos
    if (argc != 4 && argc != 5) {
        printf("Missing argument\n");
        exit(EXIT_FAILURE);
    }
    
    //Se comprueba si la IP es valida
    if(inet_aton(argv[1], &addr)==0){
        printf("IP not valid\n");
        exit(EXIT_FAILURE);
    }else{
        puerto = getservbyname("tftp", "udp")->s_port;
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
            informe = 1;
            printf("Cinco argumentos\n");
        }
    }else{
        informe = 0;
        printf("Cuatro argumentos\n");
    }
    
    //Creo el socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if(sock<0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in myaddr;
       
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = 0; //Se asigna el puerto 0 para que el sistema operativo lo complete
    myaddr.sin_addr.s_addr = INADDR_ANY; //No se asigna addr, para que el sistema operativo elija tarjeta de red

    //Se bindea el socket
    err = bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr));

    if(err<0){
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = puerto;
    dest_addr.sin_addr = addr;
    socklen_t addrlen = sizeof dest_addr;
    
    
    //datagrama INICIAL
    intToBytes(opcode, datagrama); //Opcode (lectura o escritura)
    strcpy(&datagrama[2], nombreFichero); //nombreFichero del archivo, con el EOS incluido
    strcpy(&datagrama[2+strlen(nombreFichero)+1], modoTftp); //modoTftp (octet o netascii), con el  EOS incluido
    if(sendto(sock, datagrama, 2+strlen(nombreFichero)+1+strlen(modoTftp)+1, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr)) == -1) error(strerror(errno));
    
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
                //error(codigoError[bytesToInt(&datagrama[2])]);
                printf("ERORRRR");
            }
        }while(tam == 4+TAMBLOQUE); //Al procesar un paquete de menos de TAMBLOQUE bytes, se finaliza
        
    //ESCRITURA DE DATOS EN EL SERVIDOR
    }else if (opcode == 2){
        if(informe) printf("Enviada solicitud de escritura en %s a servidor TFTP en %s.\n", argv[3], argv[1]);
        file = fopen(nombreFichero, "rb");
        
        if(recvfrom(sock, datagrama, 4+TAMNOMBRE, 0, (struct sockaddr *) &dest_addr, &addrlen) == -1) error(strerror(errno));
        if (bytesToInt(datagrama) == 4){ //El datagrama es un ACK
            if(informe) printf("Recibido ACK del bloque %d.\n", bytesToInt(&datagrama[2]));
            do{
                ack = bytesToInt(&datagrama[2]);                 //Se guarda el número de paquete que quiere el servidor
                fseek(file, TAMBLOQUE*ack, SEEK_SET);           //Se coloca el puntero al fichero en la posición correspondiente
                intToBytes(3, datagrama);                        //Se crea un datagrama de datos 
                intToBytes(ack+1, &datagrama[2]);                //Se escribe el número de bloque que se va a enviar
                tam = fread(&datagrama[4], 1, TAMBLOQUE, file); //Se escriben los datos
                if(sendto(sock, datagrama, 4+tam, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr)) == -1) error(strerror(errno));
                if(informe) printf("Enviado bloque %d.\n", ack+1);
            
                if(recvfrom(sock, datagrama, 4+TAMNOMBRE, 0, (struct sockaddr *) &dest_addr, &addrlen) == -1) error(strerror(errno));
                if (bytesToInt(datagrama) == 5) error(codigoError[bytesToInt(&datagrama[2])]);
                if(informe) printf("Recibido ACK del bloque %d.\n", bytesToInt(&datagrama[2]));
            }while ((tam == TAMBLOQUE) || (bytesToInt(&datagrama[2]) != ack+1)); //Se finaliza cuando se lee el último bloque y se recibe su ack
        
        } else if (bytesToInt(datagrama) == 5) //El datagrama contiene un mensaje de error
            error(codigoError[bytesToInt(&datagrama[2])]);
    }
        
    if(informe) printf("Era el último bloque. Cerramos el fichero.\n");
    fclose(file);
    return 0;
}
