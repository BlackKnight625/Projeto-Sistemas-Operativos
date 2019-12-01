#include "tecnicofs-api-constants.h"
#include "tecnicofs-client-api.h"
#include "../Server/lib/sockets.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int sock;

int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions) {
    int success;
    char buffer[MAX_INPUT_SIZE] = "c";
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
    char buffer[MAX_INPUT_SIZE] = "d ";
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
    char buffer[MAX_INPUT_SIZE] = "r";
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
    char buffer[MAX_INPUT_SIZE] = "o";
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
    char buffer[MAX_INPUT_SIZE] = "x";
    sprintf(buffer, "%s %d", buffer, fd);
    if (write(sock, buffer, strlen(buffer)) == -1) {
        perror("Unable to send message");
    }
    if (read(sock, &success, sizeof(int)) == -1) {
        perror("Unable to read");
    }
    return success;
}

//esta funcao deve receber do server um int seguido de uma string 
int tfsRead(int fd, char *buffer, int len) {
    int success;
    char command[MAX_INPUT_SIZE] = "l";
    sprintf(command, "%s %d %d", command, fd, len);
    if (write(sock, command, strlen(command)) == -1) {
        perror("Unable to send message in tfsRead");
    }
    if (read(sock, &success, sizeof(int)) == -1) { 
        perror("Unable to read in tfsRead");
    }
    if (success > 0) {
        if (read(sock, buffer, len) == -1) { 
            perror("Unable to read in tfsRead");
        }
    }
    return success;
}

int tfsWrite(int fd, char *buffer, int len) {
    int success;
    char command[MAX_INPUT_SIZE] = "w";
    sprintf(command, "%s %d %s", command, fd, buffer);
    if (write(sock, command, strlen(command)) == -1) {
        perror("Unable to send message in tfsWrite");
    }
    if (read(sock, &success, sizeof(int)) == -1) {
        perror("Unable to read in tfsWrite");
    }
    return success;
}

int tfsMount(char * address) {
    return newClient(&sock, address);
}

int tfsUnmount() {
    char command = 's';
    if (write(sock, &command, sizeof(char)) == -1) {
        perror("Unable to send message");
    }
    if (close(sock) == -1)
        return TECNICOFS_ERROR_OTHER;
    return 0;
}