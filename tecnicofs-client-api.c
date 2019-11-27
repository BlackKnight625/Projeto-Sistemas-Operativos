#include "tecnicofs-api-constants.h"
#include "sockets/sockets.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int sock;

int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions) {
    //lookup filename return -1 if it exists
    int success;
    char buffer[100] = "c";
    sprintf(buffer, "%s %s %d%d", buffer, filename, ownerPermissions, othersPermissions);
    if (write(sock, buffer, strlen(buffer)) == -1) {
        perror("Unable to send message");
        return -1; //probably get a new error num
    }
    if (read(sock, &success, sizeof(int)) == -1) {
        perror("Unable to read");
    }
    return success;
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
    char buffer[100] = "r";
    sprintf(buffer, "%s %s %s", buffer, filenameOld, filenameNew);
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