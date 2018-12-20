// Practica tema 8, Perez Martin Ismael

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>

#include "ip-icmp-ping.h"

unsigned short int getChecksum(ECHORequest request){
    //Comienza el calculo del checksum

    //Parte 1 del checksum

    int numShorts = sizeof(request)/2;
    unsigned short int *puntero;
    unsigned int acumulador = 0;
    puntero = (unsigned short int*)&request;

    int i;
    for(i=0; i<numShorts-1; i++){
        acumulador = acumulador + (unsigned int) *puntero;
        puntero ++;
    }

    //Parte 2 del checksum

    acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
    acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
    acumulador = ~acumulador;

    //Se devuelven los ultimos 16 bits
    return (unsigned short int)acumulador;
}

int main(int argc, char **argv){

    //Se crean las variables para almacenar los campos principales de ICMP, control de errores y debug
    int debug;
    int err;
    int sock;
    char ip[30] = "";
    unsigned char type = 8;
    unsigned char code = 0;
    char *payload = "Este es el payload"; 

    //Se crean las variables para almacenar los datagramas
    ICMPHeader cabecera;
    ECHORequest request;
    ECHOResponse response;

    //Se crea la estructura para almacenar la dirección de envio
    struct in_addr addr;  

    //Se crean las listas de errores
    char *errorTipo3[16] =  {"Destination network unreachable", 
                                "Destination host unreachable",
                                "Destination protocol unreachable",
                                "Destination port unreachable",
                                "Fragmentation required, and DF flag set",
                                "Source route failed",
                                "Destination network unknown",
                                "Destination host unknown",
                                "Source host isolated",
                                "Network administratively prohibited",
                                "Host administratively prohibited",
                                "Network unreachable for ToS",
                                "Host unreachable for ToS",
                                "Communication administratively prohibited",
                                "Host Precedence Violation",
                                "Precedence cutoff in effect "}; 

    char *errorTipo5[4] = {"Redirect Datagram for the Network",
                            "Redirect Datagram for the Host",
                            "Redirect Datagram for the ToS & network",
                            "Redirect Datagram for the ToS & host"};

    char *errorTipo11[2] = {"TTL expired in transit",
                            "Fragment reassembly time exceeded"};

    char *errorTipo12[3] = {"Pointer indicates the error",
                            "Missing a required option",
                            "Bad length"};

    char *errorTipo43[5] = {"No error",
                            "Malformed Query",
                            "No Such Interface",
                            "No Such Table Entry",
                            "Multiple Interfaces Satisfy Query"};

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
    }else{
        //Se copia la ip introducida a una variable
        strcpy(ip, argv[1]);
    }

    //Se comprueba si se ha introducido la opcion de general el informe (debug)
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

    //Se comienza a preparar la cabecera del datagrama ICMP
    if(debug) printf("-> Generando cabecera ICMP.\n");

    cabecera.Type = type;
    cabecera.Code = code;
    cabecera.Checksum = 0;

    if(debug){
        printf("-> Type: %c\n", cabecera.Type);
        printf("-> Code: %c\n", cabecera.Code);
    }

    //Se añade la cabecera al datagrama y se rellena el resto de campos

    request.icmpHeader = cabecera;
    request.ID = getpid();
    request.SeqNumber = 0;
    strcpy(request.payload, payload);

    if(debug){
        printf("-> Identifier (pid): %d\n", request.ID);
        printf("-> Seq. number: %d\n", request.SeqNumber);
        printf("-> Cadena a enviar: %s\n", request.payload);
    }

    //Se calcula y guarda el valor del checksum
    request.icmpHeader.Checksum = getChecksum(request);

    if(debug){
        printf("-> Checksum: 0x%x\n", request.icmpHeader.Checksum);
        printf("-> Tamaño total del paquete ICMP: %d\n", (int)sizeof(request));
    }

    //Se prepara el envio del paquete

    //Creo el socket con tipo RAW y el protocolo ICMP
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

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
    dest_addr.sin_addr = addr;

    //Se crea la variable para almacenar el tamaño de la direccion destino
    socklen_t addrlen = sizeof(dest_addr);

    //Se envia el datagrama ICMP a la ip introducida
    err = sendto(sock, &request, sizeof(request), 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));

    if(err<0){
        perror("sendto()");
        exit(EXIT_FAILURE);         
    }

    printf("Paquete ICMP enviado a %s\n", ip);

    //Se recibe el datagrama de respuesta de ICMP en la variable response
    err = recvfrom(sock, &response, 512, 0, (struct sockaddr *) &dest_addr, &addrlen);

    if(err<0){
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    }

    printf("Respuesta recibida desde %s\n", ip);

    if(debug){
        printf("-> Tamaño de la respuesta: %d\n", (int)sizeof(response));
        printf("-> Cadena recibida: %s\n", response.payload);
        printf("-> Identifier (pid): %d\n", response.ID);
        printf("-> TTL: %d\n", response.ipHeader.TTL);
    }

    //Se comprueba que los campos de la respuesta son correctos

    int errorType = response.icmpHeader.Type;
    int errorCode = response.icmpHeader.Code;

    if(errorType==0 && errorCode==0){
        printf("Descripción de la respuesta: respuesta correcta (type 0, code 0)\n");
        exit(EXIT_SUCCESS);

    //Si no son correctas, se realiza el control de errores para imprimir por pantalla el motivo del error
    }else if(errorType>0 && errorCode>0){

        switch(errorType){
            case 3:
                if(errorCode<16){
                    printf("Destination Unreachable: %s\n", errorTipo3[errorCode]);
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 4:
                if(errorCode == 0){
                    printf("Source quench (congestion control)\n");
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);
            
            case 5:
                if(errorCode<4){
                    printf("Redirect Message: %s\n", errorTipo5[errorCode]);
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 6:
                printf("Alternate Host Address\n");

            case 8:
                if(errorCode == 0){
                    printf("Echo request (used to ping)\n");
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 9:
                if(errorCode == 0){
                    printf("Router Advertisement\n");
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 10:
                if(errorCode == 0){
                    printf("Router discovery/selection/solicitation\n");
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);
            
            case 11:
                if(errorCode<2){
                    printf("Time Exceeded: %s\n", errorTipo11[errorCode]);
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 12:
                if(errorCode<3){
                    printf("Parameter Problem: Bad IP header: %s\n", errorTipo12[errorCode]);
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 13:
                if(errorCode == 0){
                    printf("Timestamp\n");
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 14:
                if(errorCode == 0){
                    printf("Timestamp Reply\n");
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 15:
                if(errorCode == 0){
                    printf("Information Request\n");
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 16:
                if(errorCode == 0){
                    printf("Information Reply\n");
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 17:
                if(errorCode == 0){
                    printf("Address Mask Request\n");
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 18:
                if(errorCode == 0){
                    printf("Address Mask Reply\n");
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 30:
                if(errorCode == 0){
                    printf("Traceroute: Information Request\n");
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 31:
                printf("Datagram Conversion Error\n");
                exit(EXIT_FAILURE);

            case 32:
                printf("Mobile Host Redirect\n");
                exit(EXIT_FAILURE);

            case 33:
                printf("Where-Are-You (originally meant for IPv6)\n");
                exit(EXIT_FAILURE);

            case 34:
                printf("Here-I-Am (originally meant for IPv6)\n");
                exit(EXIT_FAILURE);

            case 35:
                printf("Mobile Registration Request\n");
                exit(EXIT_FAILURE);

            case 36:
                printf("Mobile Registration Reply\n");
                exit(EXIT_FAILURE);

            case 37:
                printf("Domain Name Request\n");
                exit(EXIT_FAILURE);

            case 38:
                printf("Domain Name Reply\n");
                exit(EXIT_FAILURE);

            case 39:
                printf("SKIP Algorithm Discovery Protocol, Simple Key-Management for Internet Protocol\n");
                exit(EXIT_FAILURE);

            case 40:
                printf("Photuris, Security failures\n");
                exit(EXIT_FAILURE);
        
            case 41:
                printf("ICMP for experimental mobility protocols such as Seamoby [RFC4065]\n");
                exit(EXIT_FAILURE);

            case 42:
                if(errorCode == 0){
                    printf("Extended Echo Request: No Error\n");
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 43:
                if(errorCode<5){
                    printf("Extended Echo Reply: %s\n", errorTipo43[errorCode]);
                }else{
                    printf("Code %d nor exists with type %d\n", errorCode, errorType);
                }
                exit(EXIT_FAILURE);

            case 253:
                printf("RFC3692-style Experiment 1 (RFC 4727)\n");
                exit(EXIT_FAILURE);

            case 254:
                printf("RFC3692-style Experiment 2 (RFC 4727)\n");
                exit(EXIT_FAILURE);
        }

        if(errorType<45 || errorType==255){
            printf("Reserved Type code\n");
        }else{
            printf("Type error not exists\n");
        }
    }else{
        printf("Invalid type or code\n");
    }
    
    exit(EXIT_FAILURE);
}