#include "fs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


tecnicofs* new_tecnicofs(int numRoots){
	tecnicofs*fs = (tecnicofs*) malloc(sizeof(tecnicofs));
	inode_table_init();
	if (!fs) {
		perror("failed to allocate tecnicofs");
		exit(EXIT_FAILURE);
	}
	fs->bstRoots = (node**) malloc(sizeof(node*) * numRoots);
	fs->mutexs = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) * numRoots);
	fs->rwlocks = (pthread_rwlock_t*) malloc(sizeof(pthread_rwlock_t) * numRoots);

	for(int i = 0; i < numRoots; i++) {
    	if(pthread_mutex_init(&(fs->mutexs[i]), NULL)) {
    	    perror("Unable to initialize mutex in new_tecnicofs");
    	}
	    if(pthread_rwlock_init(&(fs->rwlocks[i]), NULL)) {
        	perror("Unable to initialize rwLock in new_tecnicofs");
    	}
		fs->bstRoots[i] = NULL;
	}
	
	fs->numRoots = numRoots;
	return fs;
}

void free_tecnicofs(tecnicofs* fs){
	int numRoots = fs->numRoots;
	for(int i = 0; i < numRoots; i++) {
		free_tree(fs->bstRoots[i]);
	}

	for(int i = 0; i < numRoots; i++) {
    	if(pthread_mutex_destroy(&(fs->mutexs[i]))) {
    	    perror("Unable to destroy mutex in new_tecnicofs");
    	}
	    if(pthread_rwlock_destroy(&(fs->rwlocks[i]))) {
        	perror("Unable to destroy rwLock in new_tecnicofs");
    	}
	}
	inode_table_destroy();
	free(fs->mutexs);
	free(fs->rwlocks);
	free(fs->bstRoots);
	free(fs);
}

void create(tecnicofs *fs, char *name, int iNumber){
	int bucket = hash(name, fs->numRoots);
    fs->bstRoots[bucket] = insert(fs->bstRoots[bucket], name, iNumber);
}

void delete(tecnicofs* fs, char *name){
	int bucket = hash(name, fs->numRoots);
	fs->bstRoots[bucket] = remove_item(fs->bstRoots[bucket], name);
}

int lookup(tecnicofs* fs, char *name){
	int bucket = hash(name, fs->numRoots);
	node* searchNode = search(fs->bstRoots[bucket], name);
	if ( searchNode ) return searchNode->inumber;
	return 0;
}

void print_tecnicofs_tree(FILE * fp, tecnicofs *fs){
	int numRoots = fs->numRoots;
	for(int i = 0; i < numRoots; i++) {
		print_tree(fp, fs->bstRoots[i]);
	}
}
