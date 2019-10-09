#include "thread_inputs.h"
//#include "fs.h" ja esta no thread_inputs


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

void *create(void *input){
	tecnicofs_char_int *inputs = (tecnicofs_char_int*) input;
	inputs->fs->bstRoot = insert(inputs->fs->bstRoot, inputs->name, inputs->iNumber);
	destroyThreadInputTecnicofsCharInt(inputs);
}