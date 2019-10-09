#include "thread_inputs.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
//#include "fs.h" ja esta no thread_inputs

pthread_rwlock_t lock;


tecnicofs_char_int *createThreadInputTecnicofsCharInt(tecnicofs *fs, char *name, int iNumber) {
    tecnicofs_char_int *input = malloc(sizeof(tecnicofs_char_int));

    input->fs = fs;
    strcpy(input->name, name);
    input->iNumber = iNumber;

    return input;
}

void initLock() {
    pthread_rwlock_init(&lock, NULL);
}

void destroyLock() {
    pthread_rwlock_destroy(&lock);
}

void destroyThreadInputTecnicofsCharInt(tecnicofs_char_int *input) {
    free(input);
}

void *create(void *input){
    if(pthread_rwlock_wrlock(&lock)) {
        perror("Unable to lock on function create(void *input)");
    }
	tecnicofs_char_int *inputs = (tecnicofs_char_int*) input;

    printf("Node sendo criada com nome %s\n", inputs->name);

	inputs->fs->bstRoot = insert(inputs->fs->bstRoot, inputs->name, inputs->iNumber);
	destroyThreadInputTecnicofsCharInt(inputs);
    
    if(pthread_rwlock_unlock(&lock)) {
        perror("Unable to unlock on function create(void *input)");
    }

    return NULL;
}