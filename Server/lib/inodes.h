#ifndef INODES_H
#define INODES_H

#include <stdio.h>
#include <stdlib.h>
#include "../../Client/tecnicofs-api-constants.h"

#define FREE_INODE -1
#define INODE_TABLE_SIZE 50
#define MAX_OPEN_FILES 5

typedef struct inode_t {
    uid_t owner;
    permission ownerPermissions;
    permission othersPermissions;
    char* fileContent;
    int isOpen; /*Holds the number of clients that curretly have this file open. Is 0 when it's closed*/
} inode_t;

typedef struct open_file_table {
    int iNumbers[MAX_OPEN_FILES];
    char modes[MAX_OPEN_FILES];
    int nOpenedFiles;
} open_file_table;

void inode_table_init();
void inode_table_destroy();
int inode_create(uid_t owner, permission ownerPerm, permission othersPerm);
int inode_delete(int inumber);
int inode_get(int inumber,uid_t *owner, permission *ownerPerm, permission *othersPerm,
                     char* fileContents, int len, int* isOpen);
int inode_set(int inumber, char *contents, int len);
int inode_open(int inumber);
int inode_close(int inumber);

void open_file_table_init(open_file_table* fileTable);


#endif /* INODES_H */