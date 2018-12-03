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

//Pasa el puntero char a un entero
int bytesToInt(char *bytes){
    int n = 0;
    n += (unsigned char) bytes[0]*256;
    n += (unsigned char) bytes[1];
    return n;
}

//Pasa un entero a un puntero char de bytes
void intToBytes(int n, char *bytes){
    bytes[0] = n/256;
    bytes[1] = n%256;
}

//Se imprime el erro de la lista predefinida
void imprimeErrorPredefinido(char *error){
    fprintf (stderr, "%s\n", error);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv){

    //Se crean las variables para almacenar los datos necesarios
    char nombreFichero[TAMNOMBRE];
    char datagrama[4+TAMBLOQUE]; 
    char *modoTftp = "octet";  
    unsigned char opcode;

    //Se crean las variables para comprobar si se desea el informe en la operacion y el puerto
    int informe;
    int puerto;

    struct in_addr addr;  

    FILE *file;                                   
    
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

    //Se crean las variables para almacenar el socket, error, ask y tamanio
    int sock;
    int err;
    int ack;
    int tam;

    //Se crea la lista de errores
    char *codigoError[TAMNOMBRE] =  {&datagrama[4], 
                                "Fichero no encontrado.",
                                "Violación de acceso.",
                                "Espacio de almacenamiento lleno.",
                                "Operación TFTP ilegal.",
                                "Identificador de transferencia desconocido.",
                                "El fichero ya existe.",
                                "Usuario desconocido."}; 
    
    //Creo el socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if(sock<0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    //Se crea e inicializa la estructura que albergara los datos de  la direccion local
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
    
    //Se crea e inicializa la estructura de la dirección destino
    struct sockaddr_in dest_addr;

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = puerto;
    dest_addr.sin_addr = addr;

    //Se crea la variable para almacenar el tamaño de la direccion destino
    socklen_t addrlen = sizeof(dest_addr);
    
    
    //Se preparan los datos del datagrama
    intToBytes(opcode, datagrama);
    strcpy(&datagrama[2], nombreFichero); 
    int i = strlen(nombreFichero)+3;
    strcpy(&datagrama[i], modoTftp); 
    int tamEnvio = strlen(nombreFichero)+strlen(modoTftp)+4;

    //Se envia el datagrama para iniciar el protocolo
    err = sendto(sock, datagrama, tamEnvio, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));

    if(err<0){
        perror("sendto() de inicio");
        exit(EXIT_FAILURE);
    }
    
    //Se inicia la lectura del archivo
    if(opcode == 1){
        if(informe){
            printf("Enviada solicitud de lectura de %s a servidor tftp en %s .",nombreFichero,argv[1]);
        }

        //Se abre el fichero en modo escritura y se inicializa el codigo de ack
        file = fopen(nombreFichero, "wb");
        ack = 1;
        do{
            //Se recibe el paquete del servidor tftp
            tam = recvfrom(sock, datagrama, 4+TAMBLOQUE, 0, (struct sockaddr *) &dest_addr, &addrlen);

            if(tam<0){
                perror("recvfrom()");
                exit(EXIT_FAILURE);
            }

            //Se comprueba que se ha recibido el bloque
            if (bytesToInt(datagrama) == 3){

                if(informe){
                    printf("Recibido bloque del servidor tftp");
                    if(ack==1){
                        printf("Es el primer bloque (numero de bloque 1).\n");
                    }else{
                        printf("Es el bloque con codigo %d.",ack);
                    }
                }

                //Se comprueba si se recibe el ack, si se recibe se escribe se incrementa en ese caso
                intToBytes(4, datagrama);
                if(bytesToInt(&datagrama[2]) == ack){
                    fwrite(&datagrama[4], 1, tam-4, file);
                    intToBytes(ack, &datagrama[2]);
                    ack++;
                }else{
                    intToBytes(ack-1, &datagrama[2]);
                }
                    
                //Se envia el ack correspondiente al servidor
                err = sendto(sock, datagrama, 4, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));
                if(err<0){
                    perror("sendto()");
                    exit(EXIT_FAILURE);
                }

                if(informe){
                    printf("Enviado ACK del bloque %d.\n", ack-1);
                }
                
            }else if(bytesToInt(datagrama) == 5){
                imprimeErrorPredefinido(codigoError[bytesToInt(&datagrama[2])]);
            }
        }while(tam == 4+TAMBLOQUE); //Al procesar un paquete de menos de TAMBLOQUE bytes, se finaliza
        
    //Se inicia la escritura del fichero en el servidor
    }else if(opcode == 2){
        if(informe){
            printf("Enviada solicitud de escritura de %s a servidor tftp en %s .",nombreFichero,argv[1]);
        }

        //Se abre el fichero en modo escritura
        file = fopen(nombreFichero, "rb");

        //Se recive el primer paquete de confirmacion de escritura
        int tamRecv = 4 + TAMNOMBRE;
        err = recvfrom(sock, datagrama, tamRecv, 0, (struct sockaddr *) &dest_addr, &addrlen);

        if(err<0){
            perror("recvfrom() de inicio");
            exit(EXIT_FAILURE);
        }

        if (bytesToInt(datagrama) == 4){

            if(informe){
                printf("Recibido ACK del bloque %d.\n", bytesToInt(&datagrama[2]));
            }

            do{
                //Se prepara el envio al servidor
                ack = bytesToInt(&datagrama[2]);  
                //Se coloca el puntero en el fichero dependiendo del ack recibido               
                fseek(file, TAMBLOQUE*ack, SEEK_SET);       
                intToBytes(3, datagrama);                    
                intToBytes(ack+1, &datagrama[2]);      
                tam = fread(&datagrama[4], 1, TAMBLOQUE, file); 

                //Se envia el paquete al servidor
                err = sendto(sock, datagrama, 4+tam, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));

                if(err<0){
                    perror("sendto() de escritura");
                    exit(EXIT_FAILURE);
                }

                if(informe){
                    printf("Enviado bloque al servidor tftp");
                    if(ack==1){
                        printf("Es el primer bloque (numero de bloque 1).\n");
                    }else{
                        printf("Es el bloque con codigo %d.",ack);
                    }
                }
            
                //Se recibe el ack de confirmacion de llegada del paquete
                err = recvfrom(sock, datagrama, 4+TAMNOMBRE, 0, (struct sockaddr *) &dest_addr, &addrlen);

                if(err<0){
                    perror("recvfrom() de escritura");
                    exit(EXIT_FAILURE);
                }

                //Se comprueba que no exista ningun error de los definidos
                if (bytesToInt(datagrama) == 5){
                    imprimeErrorPredefinido(codigoError[bytesToInt(&datagrama[2])]);
                }

                if(informe){
                    printf("Recibido ACK del servidor tftp");
                    if(ack==1){
                        printf("Es el primer ACK (numero de bloque 1).\n");
                    }else{
                        printf("Es el ACK con codigo %d.",bytesToInt(&datagrama[2]));
                    }
                }
            
            //Se comprueba para controlar el caso de si es divisible por el tamanio de bloque o no
            }while ((tam == TAMBLOQUE) || (bytesToInt(&datagrama[2]) != ack+1));
        
        //Se comprueba si existe algun error al comienzo del protocolo
        }else if(bytesToInt(datagrama) == 5){
            imprimeErrorPredefinido(codigoError[bytesToInt(&datagrama[2])]);
        }
    }
        
    if(informe){
        printf("El bloque %d era el ultimo: cerramos el fichero.\n", ack-1);
    }

    //Se cierra el fichero 
    fclose(file);

    return(EXIT_SUCCESS);
}
