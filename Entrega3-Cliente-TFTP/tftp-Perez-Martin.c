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

int main(int argc, char** argv){

    char nombreFichero[TAMNOMBRE];
    unsigned char opcode;
    int informe;

    //Se comprueba la cantidad de argumentos
    if (argc != 4 && argc != 5) {
        printf("Missing argument\n");
        exit(EXIT_FAILURE);
    }

    //Creo las variables para almacenar la direccion local
    struct in_addr addr;
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
            informe = 1;
            printf("Cinco argumentos\n");
        }
    }else{
        informe = 0;
        printf("Cuatro argumentos\n");
    }

    puerto = getservbyname("tftp", "udp")->s_port;
    puerto = htons(puerto);

    //Se crean las variables para albergar el socket y los errores
    int sock;
    int err;
    int tam;
    int ack;
    char datagrama[4+TAMBLOQUE];
    FILE *file; 
    char *modoTftp = "octet";

    char *codigoError[TAMNOMBRE] =  {&datagrama[4],
                                "Fichero no encontrado.",
                                "Violación de acceso.",
                                "Espacio de almacenamiento lleno.",
                                "Operación TFTP ilegal.",
                                "Identificador de transferencia desconocido.",
                                "El fichero ya existe.",
                                "Usuario desconocido."}; 


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

    //Se crea la variable para almacenar el tamaño de la direccion destino
    socklen_t addrlen = sizeof(dest_addr);

    puerto = getservbyname("tftp", "udp")->s_port;
    puerto = htons(puerto);

    //Preparacion del envio
    intToBytes(opcode, datagrama);
    strcpy(&datagrama[2], nombreFichero);
    tam = 2+strlen(nombreFichero)+1;
    strcpy(&datagrama[tam],modoTftp);

    //Se envia el mensaje
    tam = 2+strlen(nombreFichero)+1+strlen(modoTftp)+1;
    err = sendto(sock, datagrama, tam, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));

    if(err<0){
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

        //LECTURA DE DATOS DEL SERVIDOR
    if (opcode == 1){
        if(informe){
            printf("Enviada solicitud de lectura de %s a servidor tftp en %s .",nombreFichero,argv[1]);
        }

        file = fopen(nombreFichero, "wb");
        ack = 1;
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
            }
                
        }while(tam == 4+TAMBLOQUE); //Al procesar un paquete de menos de BLOCKSIZE bytes, se finaliza
        
    //ESCRITURA DE DATOS EN EL SERVIDOR
    }
        
    if(informe){
        printf("Era el último bloque. Cerramos el fichero.\n");
    }

    fclose(file);
    return(EXIT_SUCCESS);
}