#ifndef INODES_H
#define INODES_H

#include <stdio.h>
#include <stdlib.h>
#include "../tecnicofs-api-constants.h"

#define FREE_INODE -1
#define INODE_TABLE_SIZE 50

typedef struct inode_t {
    uid_t owner;
    permission ownerPermissions;
    permission othersPermissions;
    char* fileContent;
    char mode[2];
    int isOpen;
} inode_t;


void inode_table_init();
void inode_table_destroy();
int inode_create(uid_t owner, permission ownerPerm, permission othersPerm);
int inode_delete(int inumber);
int inode_get(int inumber,uid_t *owner, permission *ownerPerm, permission *othersPerm,
                     char* fileContents, int len, char* mode, int* isOpen);
int inode_set(int inumber, char *contents, int len);
int inode_open(int inumber, char* mode);
int inode_close(int inumber);


#endif /* INODES_H */