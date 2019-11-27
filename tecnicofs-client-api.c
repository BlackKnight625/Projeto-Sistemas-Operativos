#include "tecnicofs-api-constants.h"
#include "sockets/sockets.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int sock;

int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions) {
    //lookup filename return -1 if it exists
    char buffer[100] = "c ";
    strcat(buffer, filename);
    strcat(buffer, " ");
    char perm[12];
    sprintf(perm, "%d", ownerPermissions);
    strcat(buffer, perm);
    sprintf(perm, "%d", othersPermissions);
    strcat(buffer, perm);
    if (write(sock, buffer, strlen(buffer)) == -1) {
        perror("Unable to send message");
        return -1; //probably get a new error num
    }
    memset(buffer, 0, 100);
    if (read(sock, buffer, 100) == -1) {
        perror("Unable to read");
    }
    if (*buffer != "0")
        return TECNICOFS_ERROR_FILE_ALREADY_EXISTS; //nao conseguiu criar o ficheiro
    return 0;
}

int tfsDelete(char *filename) {
    char buffer[100] = "d ";
    strcat(buffer, filename);
    if (write(sock, buffer, strlen(buffer)) == -1) {
        perror("Unable to send message");
        return -1; //probably get a new error num
    }
    return 0;
}

int tfsRename(char *filenameOld, char *filenameNew) {
    char buffer[100] = "r ";
    strcat(buffer, filenameOld);
    strcat(buffer, " ");
    strcat(buffer, filenameNew);
    if (write(sock, buffer, strlen(buffer)) == -1) {
        perror("Unable to send message");
        return -1; //probably get a new error num
    }
    return 0;
}

int tfsOpen(char *filename, permission mode) {

}

int tfsClose(int fd) {

}

int tfsRead(int fd, char *buffer, int len) {

}

int tfsWrite(int fd, char *buffer, int len) {

}

int tfsMount(char * address) {
    return newClient(&sock, address);
}

int tfsUnmount() {
    if (close(sock) == -1)
        return -1;
    return 0;
}