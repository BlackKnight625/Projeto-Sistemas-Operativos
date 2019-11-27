#ifndef SOCKETS_H
#define SOCKETS_H

int newServer(int *sfd, char *address);
int getNewSocket(int *cfd, int sfd);
int newClient(int *sock, char *address);

#endif /* SOCKETS_H */