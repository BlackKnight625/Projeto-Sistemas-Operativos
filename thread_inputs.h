#ifndef THREAD_INPUTS
#define THREAD_INPUTS

#include "lib/bst.h"
#include "fs.h"

/*Structs que guarda inputs para funcoes*/
typedef struct Tecnicofs_char_int {
    tecnicofs *fs;
    char *name;
    int iNumber;
} tecnicofs_char_int;


tecnicofs_char_int *createThreadInputTecnicofsCharInt(tecnicofs *fs, char *name, int iNumber);
void destroyThreadInputTecnicofsCharInt(tecnicofs_char_int *input);


#endif /* THREAD_INPUTS */