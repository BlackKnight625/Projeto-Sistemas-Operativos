#include "thread_inputs.h"
#include <pthread.h>
#include <stdlib.h>
//#include "fs.h" ja esta no thread_inputs

pthread_rwlock_t lock;

tecnicofs_char_int *createThreadInputTecnicofsCharInt(tecnicofs *fs, char *name, int iNumber) {
    tecnicofs_char_int *input = malloc(sizeof(tecnicofs_char_int));

    input->fs = fs;
    input->name = name;
    input->iNumber = iNumber;
    input->fp = NULL;

    return input;
}

void destroyThreadInputTecnicofsCharInt(tecnicofs_char_int *input) {
    free(input);
}

void *create(void *input) {
    if(pthread_rwlock_wrlock(&lock)) {
        perror("Unable to lock on function create(void *input)");
    }

	tecnicofs_char_int *inputs = (tecnicofs_char_int*) input;
	inputs->fs->bstRoot = insert(inputs->fs->bstRoot, inputs->name, inputs->iNumber);
	destroyThreadInputTecnicofsCharInt(inputs);
    
    if(pthread_rwlock_unlock(&lock)) {
        perror("Unable to unlock on function create(void *input)");
    }

    return NULL;
}

void *look(void *input) {
    if(pthread_rwlock_rdlock(&lock)) {
        perror("Unable to lock on function look(void *input)");
    }
    int searchResult;
    tecnicofs_char_int *inputs = (tecnicofs_char_int*) input;
    searchResult = lookup(inputs->fs, inputs->name);
    if(!searchResult)
        fprintf(inputs->fp, "%s not found\n", inputs->name);
    else
        fprintf(inputs->fp, "%s found with inumber %d\n", inputs->name, searchResult);
    if(pthread_rwlock_unlock(&lock)) {
        perror("Unable to unlock on function look(void *input)");
    }
    return NULL;
}