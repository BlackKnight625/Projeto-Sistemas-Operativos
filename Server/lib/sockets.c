#include "../../Client/tecnicofs-api-constants.h"
#include "sockets.h"

#define MAX_LOCATION_SIZE 128

int newServer(int *sfd, char *address) {
    struct sockaddr_un serv_addr;
    char location[MAX_LOCATION_SIZE] = "/tmp/";

    strcat(location, address);

    //Cria socket stream
    *sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (*sfd == -1)
        return TECNICOFS_ERROR_CONNECTION_ERROR;

    //Elimina o nome para o caso de ja existir
    unlink(location);
    
    //Limpeza preventiva
    bzero((char *)&serv_addr, sizeof(serv_addr)); 
    //Dados para o sock: tipo e nome que identifica o server
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, location, sizeof(serv_addr.sun_path)-1);
    int servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family); 

    //O nome para que os clientes possam identificar o server
    if (bind(*sfd, (struct sockaddr *) &serv_addr, servlen) == -1) 
        return TECNICOFS_ERROR_CONNECTION_ERROR;

    //Marca o socket como o listening port
    if (listen(*sfd, 5) == -1)
        return TECNICOFS_ERROR_CONNECTION_ERROR;

    return 0;    
}

int getNewSocket(int *cfd, int sfd) {
    struct sockaddr_un cli_addr;
    int clilen = sizeof(cli_addr); 
    *cfd = accept(sfd, (struct sockaddr *) &cli_addr, (socklen_t *) &clilen);

    if (*cfd < 0)
        return TECNICOFS_ERROR_CONNECTION_ERROR;

    return 0;
}

int newClient(int *sock, char *address) {
    int errno;
    struct sockaddr_un serv_addr;
    char location[MAX_LOCATION_SIZE] = "/tmp/";

    strcat(location, address);

    //Cria socket stream
    *sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (*sock == -1)
        return TECNICOFS_ERROR_CONNECTION_ERROR;

    //Limpeza preventiva
    bzero((char *) &serv_addr, sizeof(serv_addr));
    //Dados para o sock: tipo e nome que identifica o server
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, location, sizeof(serv_addr.sun_path)-1);
    int servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family); 

    //Estabelece uma ligacao
    if ((errno = connect(*sock, (struct sockaddr *) &serv_addr, servlen)) < 0) {
        switch(errno) {
            case (EISCONN):
                return TECNICOFS_ERROR_OPEN_SESSION;
                break;
            default:
                return TECNICOFS_ERROR_CONNECTION_ERROR;
                break;
        }
    }

    return 0;
}
