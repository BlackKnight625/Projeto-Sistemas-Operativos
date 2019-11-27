#include "sockets.h"

int newServer(int *sfd, char *address) {
    struct sockaddr_un serv_addr;

    //Cria socket stream
    *sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (*sfd == -1)
        return -1;

    //Elimina o nome para o caso de ja existir
    unlink(address);
    
    //Limpeza preventiva
    bzero((char *)&serv_addr, sizeof(serv_addr)); 
    //Dados para o sock: tipo e nome que identifica o server
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, address, sizeof(serv_addr.sun_path)-1);
    int servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family); 

    //O nome para que os clientes possam identificar o server
    if (bind(*sfd, (struct sockaddr *) &serv_addr, servlen) == -1) 
        return -1;

    //Marca o socket como o listening port
    if (listen(*sfd, 5) == -1)
        return -1;

    return 0;    
}

int getNewSocket(int *cfd, int sfd) {
    struct sockaddr_un cli_addr;
    int clilen = sizeof(cli_addr); 
    *cfd = accept(sfd, (struct sockaddr *) &cli_addr, &clilen);

    if (*cfd < 0)
        return -1;

    return 0;
}

int newClient(int *sock, char *address) {
    struct sockaddr_un serv_addr;

    //Cria socket stream
    *sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (*sock == -1)
        return -1;

    //Limpeza preventiva
    bzero((char *) &serv_addr, sizeof(serv_addr));
    //Dados para o sock: tipo e nome que identifica o server
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, address, sizeof(serv_addr.sun_path)-1);
    int servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family); 

    //Estabelece uma ligacao
    if (connect(*sock, (struct sockaddr *) &serv_addr, servlen) < 0)
        return -1;

    return 0;
}