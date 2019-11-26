#include "tecnicofs-api-constants.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
static int sock;
struct sockaddr_un morada;

int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions) {
    write(sock, filename, strlen(filename));
}
int tfsDelete(char *filename) {

}
int tfsRename(char *filenameOld, char *filenameNew) {

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
    char path[200] = "/tmp/";
    strcat(path, address);
    if((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Unable to create socket");
    }

    strncpy(morada.sun_path, path, strlen(path) - 1);
    morada.sun_family = AF_UNIX;
    if(connect(sock, (struct sockaddr*) &morada, sizeof(struct sockaddr_un)) == -1) {
        perror("Unable to connect");
    }
    return 0;
}
int tfsUnmount() {

}