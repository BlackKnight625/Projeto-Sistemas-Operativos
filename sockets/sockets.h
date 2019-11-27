#ifndef SOCKETS_H
#define SOCKETS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int newServer(int *sfd, char *address);
int getNewSocket(int *cfd, int sfd);
int newClient(int *sock, char *address);

#endif /* SOCKETS_H */