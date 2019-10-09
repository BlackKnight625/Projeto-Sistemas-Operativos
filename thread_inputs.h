#ifndef THREAD_INPUTS_H
#define THREAD_INPUTS_H

//#include "lib/bst.h" ja esta incluido em fs.h
#include "fs.h"
#include <stdlib.h>

/*Structs que guarda inputs para funcoes*/
typedef struct Tecnicofs_char_int {
    tecnicofs *fs;
    char *name;
    int iNumber;
} tecnicofs_char_int;


tecnicofs_char_int *createThreadInputTecnicofsCharInt(tecnicofs *fs, char *name, int iNumber);
void destroyThreadInputTecnicofsCharInt(tecnicofs_char_int *input);
void *create(void *input);

#endif /* THREAD_INPUTS_H */