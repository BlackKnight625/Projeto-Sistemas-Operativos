#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include "fs.h"
#include "lib/hash.h"
#include "lib/sockets.h"

void doNothing(int bucket);

/*Testa existência de macros no compilador*/
#if defined RWLOCK
#define LOCK_WRITE_ACCESS(bucket) if (pthread_rwlock_wrlock(&fs->rwlocks[bucket])) perror("Unable to rwlock LOCK_WRITE_ACCESS") 
#define LOCK_READ_ACCESS(bucket) if (pthread_rwlock_rdlock(&fs->rwlocks[bucket])) perror("Unable to rwlock LOCK_READ_ACCESS") 

#define UNLOCK_ACCESS(bucket) if (pthread_rwlock_unlock(&fs->rwlocks[bucket])) perror("Unable to rwlock UNLOCK_ACCESS")

#define MULTITHREADING 1 /*true*/

/*Nenhuma macro previamente definida*/
#else
#define LOCK_WRITE_ACCESS(bucket) doNothing(bucket)
#define LOCK_READ_ACCESS(bucket) doNothing(bucket)

#define UNLOCK_ACCESS(bucket) doNothing(bucket)

#define MULTITHREADING 0 /*false*/
#endif /*Teste de existência de macros*/


#define MAX_COMMANDS 10 /*Mudei este valor para comecar a execucao incremental*/
#define MAX_INPUT_SIZE 100
#define MAX_CONTENT_SIZE 100
#define NUM_MAX_THREADS 64

/*Variáveis globais*/
tecnicofs *fs;
int numThreads;
int numBuckets;
int sfd;
pthread_t threadIds[NUM_MAX_THREADS];
FILE *fp;
double time_ini;

/*Mostra como se chama corretamente o programa*/
static void displayUsage (const char* appName){
    printf("Usage: %s nomesocket outputfile numBuckets\n", appName);
    exit(EXIT_FAILURE);
}

/*Verifica que a funcao recebeu todos os argumentos*/
static void parseArgs (long argc, char* const argv[]){
    if (argc != 4) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
}

/*Faz nada. Absolutamente nada*/
void doNothing(int bucket) {
    (void)bucket;
}

int hasPermissionToWrite(uid_t owner, uid_t person, permission ownerPerm, permission othersPerm) {
    if(owner == person) {
        if(ownerPerm == WRITE || ownerPerm == RW) return 1;
    }
    if(othersPerm == WRITE || othersPerm == RW) return 1;
    return 0;
}

int hasPermissionToRead(uid_t owner, uid_t person, permission ownerPerm, permission othersPerm) {
    if(owner == person) {
        if(ownerPerm == READ || ownerPerm == RW) return 1;
    }
    if(othersPerm == READ || othersPerm == RW) return 1;
    return 0;
}

/*------------------------------------------------------------------
Funcao auxiliar para tratar do caso rename. De forma a evitar 
interblocagem, decidimos fazer lock aos respetivos mutexs/rwlocks
pela seguinte ordem: Deve-se lockar o menor bucket primeiro
------------------------------------------------------------------*/
void multipleLock(int currentBucket, int newBucket) {
    if (currentBucket == newBucket) {
        LOCK_WRITE_ACCESS(currentBucket);
    }
    else if (currentBucket < newBucket) {
        LOCK_WRITE_ACCESS(currentBucket);
        LOCK_WRITE_ACCESS(newBucket);
    }
    else {
        LOCK_WRITE_ACCESS(newBucket);
        LOCK_WRITE_ACCESS(currentBucket);
    }
}

void multipleUnlock(int currentBucket, int newBucket) {
    UNLOCK_ACCESS(currentBucket);
    if (currentBucket != newBucket) {
        UNLOCK_ACCESS(newBucket);
    }
}

/*------------------------------------------------------------------
Funcao responsavel por ler um comando do vetor -inputCommands-
e de o executar. Esta funcao e' chamada pelos consumidores
------------------------------------------------------------------*/
int applyCommands(char command, char arg1[], char arg2[], uid_t commandSender, int sock, char* content, open_file_table* fileTable){
    int searchResult;
    int iNumber;
    int bucket;
    int currentBucket;
    int newBucket;
    int isOpen;
    int result = 0; //Value returned by this function
    int fd;
    uid_t owner;
    char mode;
    permission ownerPerm;
    permission othersPerm;
    int len;

    content[0] = '\0'; /*Indicates that the content's empty*/
    
    switch(command) { /*Calculates the hash only for the commands that need it*/
        case 'c':
        case 'd':
        case 'r':
        case 'o':
            bucket = hash(arg1, numBuckets);
            break;
    }
    

    switch (command) {
        case 'c':
            if(!(arg2[0] >= '0' && arg2[0] <= '3') || !(arg2[1] >= '0' && arg2[1] <= '3')) {
                return TECNICOFS_ERROR_INVALID_MODE;
            }

            LOCK_WRITE_ACCESS(bucket);

            searchResult = lookup(fs, arg1);
            if(searchResult != -1) { 
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
            }

            if((iNumber = inode_create(commandSender, arg2[0] -'0', arg2[1] -'0')) == -1) {
                return TECNICOFS_ERROR_OTHER;
            }
            
            create(fs, arg1, iNumber);

            UNLOCK_ACCESS(bucket);
            break;
        case 'l':
            if(((fd = atoi(arg1)) == 0) && arg1[0] != '0') { /*If arg1 differs from "0" and atoi return 0, then arg1 contains a non-numeric string*/
                return TECNICOFS_ERROR_OTHER;
            }

            iNumber = fileTable->iNumbers[fd];

            if(iNumber == -1) {
                return TECNICOFS_ERROR_FILE_NOT_OPEN;
            } 

            mode = fileTable->modes[fd];

            if(inode_get(iNumber, &owner, &ownerPerm, &othersPerm, content, MAX_CONTENT_SIZE, &isOpen) == -1) {
                return TECNICOFS_ERROR_OTHER;
            }

            else if(!hasPermissionToRead(owner, commandSender, ownerPerm, othersPerm)) {
                return TECNICOFS_ERROR_PERMISSION_DENIED;
            }
            else if (mode - '0' != READ && mode - '0' != RW) {
                return TECNICOFS_ERROR_INVALID_MODE;
            }
            else if (!isOpen) {
                return TECNICOFS_ERROR_FILE_NOT_OPEN;
            }


            len = atoi(arg2)-1;
            if (len > strlen(content))
                result = strlen(content);
            else { result = len; }

            break;
        case 'o': /*A nossa solucao permite que varios clientes possam abrir o mesmo ficheiro em modos diferentes*/
            LOCK_READ_ACCESS(bucket);

            searchResult = lookup(fs, arg1);

            if(searchResult == -1) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_FILE_NOT_FOUND;
            }

            if(!(arg2[0] > '0' && arg2[0] <= '3')) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_INVALID_MODE;
            }
            else if(inode_get(searchResult, &owner, &ownerPerm, &othersPerm, NULL, 0, NULL) == -1) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_OTHER;
            }
            else if ((arg2[0] == '2' || arg2[0] == '3') && !hasPermissionToRead(owner, commandSender, ownerPerm, othersPerm)) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_PERMISSION_DENIED;
            }
            else if ((arg2[0] == '1' || arg2[0] == '3') && !hasPermissionToWrite(owner, commandSender, ownerPerm, othersPerm)) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_PERMISSION_DENIED;
            }

            if(fileTable->nOpenedFiles == MAX_OPEN_FILES) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_MAXED_OPEN_FILES;
            }
            UNLOCK_ACCESS(bucket);

            inode_open(searchResult);

            for(int i = 0; i < MAX_OPEN_FILES; i++) {
                if(fileTable->iNumbers[i] == -1) {
                    fileTable->iNumbers[i] = searchResult;
                    fileTable->modes[i] = arg2[0];
                    fileTable->nOpenedFiles++;
                    break;
                }
            }
            
            result = searchResult;
            break;
        case 'x':
            if(((fd = atoi(arg1)) == 0) && arg1[0] != '0') { /*If arg1 differs from "0" and atoi return 0, then arg1 contains a non-numeric string*/
                return TECNICOFS_ERROR_OTHER;
            }

            iNumber = fileTable->iNumbers[fd];
            
            if(iNumber == -1) {
                return TECNICOFS_ERROR_FILE_NOT_OPEN;
            } 

            if(inode_get(iNumber, NULL, NULL, NULL, NULL, 0, &isOpen) == -1) {
                return TECNICOFS_ERROR_OTHER;
            }
            else if(!isOpen) {
                return TECNICOFS_ERROR_FILE_NOT_OPEN;
            }

            inode_close(iNumber);
            fileTable->iNumbers[fd] = -1;
            fileTable->modes[fd] = 0;
            fileTable->nOpenedFiles--;

            break;
        case 'w':
            if(((fd = atoi(arg1)) == 0) && arg1[0] != '0') { /*If arg1 differs from "0" and atoi return 0, then arg1 contains a non-numeric string*/
                return TECNICOFS_ERROR_OTHER;
            }

            iNumber = fileTable->iNumbers[fd];
            if(iNumber == -1) {
                return TECNICOFS_ERROR_FILE_NOT_OPEN;
            }
            mode = fileTable->modes[fd];

            if(inode_get(iNumber, &owner, &ownerPerm, &othersPerm, NULL, 0, &isOpen) == -1) {
                return TECNICOFS_ERROR_OTHER;
            }
            else if(!hasPermissionToWrite(owner, commandSender, ownerPerm, othersPerm)) {
                return TECNICOFS_ERROR_PERMISSION_DENIED;
            }
            if (mode - '0' != WRITE && mode - '0' != RW) {
                return TECNICOFS_ERROR_INVALID_MODE;
            }
            else if (!isOpen) {
                return TECNICOFS_ERROR_FILE_NOT_OPEN;
            }

            else if(inode_set(iNumber, arg2, strlen(arg2))) {
                return TECNICOFS_ERROR_OTHER;
            }

            
            break;
        case 'd':
            LOCK_WRITE_ACCESS(bucket);
            searchResult = lookup(fs, arg1);

            if(searchResult == -1) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_FILE_NOT_FOUND;
            }

            else if(inode_get(searchResult, &owner, NULL, NULL, NULL, 0, &isOpen) == -1) {
                return TECNICOFS_ERROR_OTHER;
            }
            else if (commandSender != owner) {
                return TECNICOFS_ERROR_PERMISSION_DENIED;
            }
            else if(isOpen) {
                return TECNICOFS_ERROR_FILE_IS_OPEN;
            }
            else if(inode_delete(searchResult) == -1) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_OTHER;
            }

            delete(fs, arg1);
            
            UNLOCK_ACCESS(bucket);       
            break;
        case 'r':

            currentBucket = bucket;
            newBucket = hash(arg2, numBuckets);

            multipleLock(currentBucket, newBucket);

            searchResult = lookup(fs, arg1);

            if(searchResult == -1) {
                multipleUnlock(currentBucket, newBucket);
                return TECNICOFS_ERROR_FILE_NOT_FOUND;
            }

            else if(inode_get(searchResult, &owner, NULL, NULL, NULL, 0, &isOpen) == -1) {
                multipleUnlock(currentBucket, newBucket);
                return TECNICOFS_ERROR_OTHER;
            }
            else if(commandSender != owner) {
                multipleUnlock(currentBucket, newBucket);
                return TECNICOFS_ERROR_PERMISSION_DENIED;
            }
            else if(isOpen) {
                multipleUnlock(currentBucket, newBucket);
                return TECNICOFS_ERROR_FILE_IS_OPEN;
            }

            if (lookup(fs, arg2) == -1) {
                delete(fs, arg1);
                create(fs, arg2, searchResult);
            }
            else {
                multipleUnlock(currentBucket, newBucket);
                return TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
            }

            multipleUnlock(currentBucket, newBucket);
            break;
        case 's':
            for(int i = 0; i < MAX_OPEN_FILES; i++) {
                if (fileTable->iNumbers[i] != -1) {
                    inode_close(fileTable->iNumbers[i]);
                }              
            }

            result = 1;
            break;
        default: { /* error */
            result = TECNICOFS_ERROR_OTHER;
            break;
        }
    }
    return result;
}

/* -----------------------------------------------------------------
Esta funcao devolve um double cujo valor e' o tempo em segundos e 
microsegundos desde o inicio dos tempos
------------------------------------------------------------------*/
double getTime() {
    struct timeval tv;
    double secs;
    double msecs;
    if(gettimeofday(&tv, NULL)) {
        perror("Unable to get time of day");
        exit(EXIT_FAILURE);
    }
    secs = (double) tv.tv_sec;
    msecs = (double) tv.tv_usec;
    return secs + msecs/1000000;
}

/*------------------------------------------------------------------
Funcao chamada pelas thread escravas. Ouve por pedidos do cliente respetivo e executa-os
--------------------------------------------------------------------*/
void *threadFunc(void *cfd) {
    char buffer[MAX_INPUT_SIZE];
    char command;
    char arg1[MAX_INPUT_SIZE];
    char arg2[MAX_INPUT_SIZE];
    char content[MAX_CONTENT_SIZE];
    int sock = *((int *) cfd);
    open_file_table fileTable;
    uid_t owner;
    struct ucred info;
    int infolen = sizeof(info);
    sigset_t set;

    if (sigemptyset(&set) == -1) {
        perror("Unable to empty set");
        close(sock);
        return NULL;
    }
    if (sigaddset(&set, SIGINT) == -1) {
        perror("Unable to delete SIGINT");
        close(sock);
        return NULL;
    }
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("Unable to pthread_sigmask");
        close(sock);
        return NULL;
    }

    open_file_table_init(&fileTable);

    if (getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &info, (socklen_t *) &infolen) != 0) {
        perror("Unable to get options");
        close(sock);
        return NULL;
    } 

    owner = info.uid;

    while(1) {
        memset(buffer, 0, MAX_INPUT_SIZE);
        if(read(sock, buffer, MAX_INPUT_SIZE) == -1) {
            perror("Error on read");
            break;   
        }
        sscanf(buffer, "%c %s %s", &command, arg1, arg2);
        int success = applyCommands(command, arg1, arg2, owner, sock, content, &fileTable);
        if (success == 1)
            break;
        if (write(sock, &success, sizeof(int)) == -1) {
            perror("Error on write");
            break;
        }
        if (*content != '\0' && success > 0) {
            if (write(sock, content, success) == -1) {
                perror("Error on write");
                break;
            }
        }
    }
    close(sock);
    return NULL;
}

/*------------------------------------------------------------------
Funcao responsavel pelo tratamento do signal recebido causado por 
Ctrl+C. Devendo terminar ordeiramente o servidor.
------------------------------------------------------------------*/
void apanhaCTRLC(int s) {
    close(sfd);
    for (int i = 0; i < numThreads; i++) {
        if (pthread_join(threadIds[i], NULL))
            perror("Unable to join");
    }

    double time_f = getTime();

    printf("TecnicoFs completed in %0.4f seconds.\n", time_f-time_ini);

    print_tecnicofs_tree(fp, fs);

    free_tecnicofs(fs);

    if(fclose(fp)) {
        perror("Unable to close file");
    }
    
    exit(EXIT_SUCCESS);
}

/*------------------------------------------------------------------
Funcao main
--------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
    parseArgs(argc, argv);
    numBuckets = atoi(argv[3]);

    if(numBuckets <= 0) {
        perror("Number of buckets must be a number greater than 0");
        exit(EXIT_FAILURE);
    }

    if(!(fp = fopen(argv[2], "w"))) {
        perror("Unable to open file for writing");
        exit(EXIT_FAILURE);
    }

    fs = new_tecnicofs(numBuckets);
    
    time_ini = getTime();

    if (signal(SIGINT, apanhaCTRLC) == SIG_ERR) {
        perror("Unable to set signal");
        exit(EXIT_FAILURE);
    }

    if (newServer(&sfd, argv[1]) < 0) {
        perror("Unable to create server");
        exit(EXIT_FAILURE);
    }

    int ix = 0;

    while (numThreads < NUM_MAX_THREADS) {
        int new_sock;
        if (getNewSocket(&new_sock, sfd) < 0) {
            perror("Unable to create socket");
            break;
        }
        if (pthread_create(&threadIds[ix], 0, threadFunc, (void *) &new_sock) != 0) {
            perror("Unable to create thread");
            break;
        }
        numThreads += 1;
        ix += 1;
    }
    apanhaCTRLC(0);
    return 0;
}
