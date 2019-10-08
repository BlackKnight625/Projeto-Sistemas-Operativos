#include "fs.h"
#include "thread_inputs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*Retorna o -nextINumber- do -fs- e incrementa-o*/
int obtainNewInumber(tecnicofs* fs) {
	int newInumber = ++(fs->nextINumber);
	return newInumber;
}

/*Construtor da struct -tecnicofs-*/
tecnicofs* new_tecnicofs(){
	tecnicofs*fs = malloc(sizeof(tecnicofs));
	if (!fs) {
		perror("failed to allocate tecnicofs");
		exit(EXIT_FAILURE);
	}
	fs->bstRoot = NULL;
	fs->nextINumber = 0;
	return fs;
}

/*Liberta -fs- e seus atributos da memoria*/
void free_tecnicofs(tecnicofs* fs){
	free_tree(fs->bstRoot);
	free(fs);
}


void create(void *input){
	tecnicofs_char_int *inputs = (tecnicofs_char_int*) input;
	inputs->fs->bstRoot = insert(inputs->fs->bstRoot, inputs->name, inputs->iNumber);
}

void delete(tecnicofs* fs, char *name){
	fs->bstRoot = remove_item(fs->bstRoot, name);
}

int lookup(tecnicofs* fs, char *name){
	node* searchNode = search(fs->bstRoot, name);
	if ( searchNode ) return searchNode->inumber;
	return 0;
}

void print_tecnicofs_tree(FILE * fp, tecnicofs *fs){
	print_tree(fp, fs->bstRoot);
}
