#ifndef FS_H
#define FS_H
#include "lib/bst.h"
#include "lib/hash.h"
#include "lib/inodes.h"
#include <pthread.h>

typedef struct tecnicofs {
    node** bstRoots;
    int numRoots;
    pthread_mutex_t *mutexs;
    pthread_rwlock_t *rwlocks;
} tecnicofs;

tecnicofs* new_tecnicofs(int numRoots);
void free_tecnicofs(tecnicofs* fs);
void create(tecnicofs *fs, char *name, int iNumber);
void delete(tecnicofs* fs, char *name);
int lookup(tecnicofs* fs, char *name);
void print_tecnicofs_tree(FILE * fp, tecnicofs *fs);

#endif /* FS_H */
