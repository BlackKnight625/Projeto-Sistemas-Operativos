#include "thread_inputs.h"
#include "fs.h"


tecnicofs_char_int *createThreadInputTecnicofsCharInt(tecnicofs *fs, char *name, int iNumber) {
    tecnicofs_char_int *input = malloc(sizeof(tecnicofs_char_int));
    input->fs = fs;
    input->name = name;
    input->iNumber;

    return input;
}

void destroyThreadInputTecnicofsCharInt(tecnicofs_char_int *input) {
    free(input);
}