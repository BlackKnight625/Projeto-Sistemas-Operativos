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
    }
    if (read(sock, &success, sizeof(int)) == -1) {
        perror("Unable to read");
    }
    return success;
}

int tfsDelete(char *filename) {
    int success;
    char buffer[100] = "d ";
    strcat(buffer, filename);
    if (write(sock, buffer, strlen(buffer)) == -1) {
        perror("Unable to send message");
    }
    if (read(sock, &success, sizeof(int)) == -1) {
        perror("Unable to read");
    }
    return success;
}

int tfsRename(char *filenameOld, char *filenameNew) {
    int success;
    char buffer[100] = "r";
    sprintf(buffer, "%s %s %s", buffer, filenameOld, filenameNew);
    if (write(sock, buffer, strlen(buffer)) == -1) {
        perror("Unable to send message");
    }
    if (read(sock, &success, sizeof(int)) == -1) {
        perror("Unable to read");
    }
    return success;
}

int tfsOpen(char *filename, permission mode) {
    int success;
    char buffer[100] = "o";
    sprintf(buffer, "%s %s %d", buffer, filename, mode);
    if (write(sock, buffer, strlen(buffer)) == -1) {
        perror("Unable to send message");
    }
    if (read(sock, &success, sizeof(int)) == -1) {
        perror("Unable to read");
    }
    return success;
}

int tfsClose(int fd) {
    int success;
    char buffer[100] = "x";
    sprintf(buffer, "%s %d", buffer, fd);
    if (write(sock, buffer, strlen(buffer)) == -1) {
        perror("Unable to send message");
    }
    if (read(sock, &success, sizeof(int)) == -1) {
        perror("Unable to read");
    }
    return success;
}

int tfsRead(int fd, char *buffer, int len) {
    int success;
    char command[100] = "l";
    sprintf(command, "%s %d %d", command, fd, len);
    if (write(sock, command, strlen(command)) == -1) {
        perror("Unable to send message");
    }
    //esta funcao deve retornar um int seguido de uma string 
    if (read(sock, &success, sizeof(int)) == -1) { 
        perror("Unable to read");
    }
    if (read(sock, buffer, len) == -1) { 
        perror("Unable to read");
    }
    if (success != 0)
        return success;
    return len-1; 
}

int tfsWrite(int fd, char *buffer, int len) {
    int success;
    char command[100] = "w";
    sprintf(command, "%s %d %d", command, fd, buffer);
    if (write(sock, command, strlen(command)) == -1) {
        perror("Unable to send message");
    }
    if (read(sock, &success, sizeof(int)) == -1) {
        perror("Unable to read");
    }
    return success;
}

int tfsMount(char * address) {
    return newClient(&sock, address);
}

int tfsUnmount() {
    char command = 's';
    if (write(sock, command, sizeof(char)) == -1) {
        perror("Unable to send message");
    }
    if (close(sock) == -1)
        return -1;
    return 0;
}