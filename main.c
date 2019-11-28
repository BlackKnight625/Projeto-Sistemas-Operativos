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
#include "sockets/sockets.h"

void doNothing(int bucket);

/*Testa existência de macros no compilador*/
#if defined MUTEX
#define LOCK_COMMAND() if (pthread_mutex_lock(&mutexCommand)) perror("Unable to mutex lock LOCK_COMMAND")
#define LOCK_WRITE_ACCESS(bucket) if (pthread_mutex_lock(&fs->mutexs[bucket])) perror("Unable to mutex lock LOCK_WRITE_ACCESS") 
#define LOCK_READ_ACCESS(bucket) if (pthread_mutex_lock(&fs->mutexs[bucket])) perror("Unable to mutex lock LOCK_READ_ACCESS") 

#define UNLOCK_COMMAND() if (pthread_mutex_unlock(&mutexCommand)) perror("Unable to mutex unlock UNLOCK_COMMAND") 
#define UNLOCK_ACCESS(bucket) if (pthread_mutex_unlock(&fs->mutexs[bucket])) perror("Unable to mutex unlock UNLOCK_ACCESS") 

#define MULTITHREADING 1 /*true*/


#elif defined RWLOCK
#define LOCK_COMMAND() if (pthread_mutex_lock(&mutexCommand)) perror("Unable to mutex lock LOCK_COMMAND") 
#define LOCK_WRITE_ACCESS(bucket) if (pthread_rwlock_wrlock(&fs->rwlocks[bucket])) perror("Unable to rwlock LOCK_WRITE_ACCESS") 
#define LOCK_READ_ACCESS(bucket) if (pthread_rwlock_rdlock(&fs->rwlocks[bucket])) perror("Unable to rwlock LOCK_READ_ACCESS") 

#define UNLOCK_COMMAND() if (pthread_mutex_unlock(&mutexCommand)) perror("Unable to mutex lock UNLOCK_COMMAND") 
#define UNLOCK_ACCESS(bucket) if (pthread_rwlock_unlock(&fs->rwlocks[bucket])) perror("Unable to rwlock UNLOCK_ACCESS")

#define MULTITHREADING 1 /*true*/

/*Nenhuma macro previamente definida*/
#else
#define LOCK_COMMAND()
#define LOCK_PROD()

#define LOCK_WRITE_ACCESS(bucket) doNothing(bucket)
#define LOCK_READ_ACCESS(bucket) doNothing(bucket)

#define UNLOCK_COMMAND()
#define UNLOCK_PROD()
#define UNLOCK_ACCESS(bucket) doNothing(bucket)

#define MULTITHREADING 0 /*false*/
#endif /*Teste de existência de macros*/

#if MULTITHREADING
#define LOCK_PROD() if(pthread_mutex_lock(&mutexProd)) perror("Unable to mutex lock LOCK_PROD")
#define UNLOCK_PROD() if(pthread_mutex_unlock(&mutexProd)) perror("Unable to mutex unlock UNLOCK_PROD")
#endif 

#define WAIT_PODE_PROD() if(sem_wait(&pode_prod)) perror("Unable to WAIT_PODE_PROD")
#define WAIT_PODE_CONS() if(sem_wait(&pode_cons)) perror("Unable to WAIT_PODE_CONS")
#define POST_PODE_PROD() if(sem_post(&pode_prod)) perror("Unable to POST_PODE_PROD")
#define POST_PODE_CONS() if(sem_post(&pode_cons)) perror("Unable to POST_PODE_CONS")


#define MAX_COMMANDS 10 /*Mudei este valor para comecar a execucao incremental*/
#define MAX_INPUT_SIZE 100
#define MAX_CONTENT_SIZE 100
#define NUM_MAX_THREADS 16


/*Vetor de Strings que guarda os comandos*/
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int prodCommands = 0;
int consCommands = 0;
int headQueue = 0;
int nThreads = 0;

/*Variáveis globais*/
pthread_mutex_t mutexCommand;
pthread_mutex_t mutexProd;
tecnicofs *fs;
int numMaxThreads;
int numBuckets;
sem_t pode_prod;
sem_t pode_cons;
int sfd;
pthread_t threadIds[NUM_MAX_THREADS];
FILE *fp;

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
    if(othersPerm == WRITE || othersPerm == RW) return 1;
    else if(owner == person) {
        if(ownerPerm == WRITE || ownerPerm == RW) return 1;
    }
    return 0;
}

int hasPermissionToRead(uid_t owner, uid_t person, permission ownerPerm, permission othersPerm) {
    if(othersPerm == READ || othersPerm == RW) return 1;
    else if(owner == person) {
        if(ownerPerm == READ || ownerPerm == RW) return 1;
    }
    return 0;
}

/*Insere o comando fornecido por -data- no vetor -inputCommands-*/
void insertCommand(char* data) {
    strcpy(inputCommands[prodCommands], data);
    prodCommands = (prodCommands+1)%MAX_COMMANDS;
}

/*Devolve o proximo comando em -inputCommands- */
char* removeCommand() {
    if(strcmp(inputCommands[headQueue], "q!")){
        char *line = inputCommands[headQueue];
        headQueue = (headQueue+1)%MAX_COMMANDS;
        return line;
    }
    return NULL;
}

/*Funcao de erro caso os inputs nao sejam corretos*/
void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/*------------------------------------------------------------------
Converte o input fornecido por line para o vetor -inputCommands-
Devolve 0 se nao tiver sido inserido nenhum comando,
devolve 1 caso contrario. Esta funcao e' chamada pelo produtor
------------------------------------------------------------------*/
int processInput(char *line){
    char token;
    char name[MAX_INPUT_SIZE];
    int numTokens = sscanf(line, "%c %s", &token, name);

    if (numTokens < 1) {
        return 0;
    }
    switch (token) {
        case 'c':
        case 'l':
        case 'd':
        case 'r':
            if(numTokens != 2)
                errorParse();
            LOCK_PROD();
            insertCommand(line);
            UNLOCK_PROD();
                break;
        case 'q':
            LOCK_PROD();
            strcpy(inputCommands[prodCommands], line);
            UNLOCK_PROD();
            break;
        case '#':
            return 0;
        default: { /* error */
            errorParse();
        }
    }
    return 1;
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
    char* mode = NULL;
    permission ownerPerm;
    permission othersPerm;
    int len;

    content[0] = 0; /*Indicates that the content's empty*/
    
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
            if((iNumber = inode_create(commandSender, arg2[0], arg2[1])) == -1) {
                return TECNICOFS_ERROR_OTHER;
            }

            LOCK_WRITE_ACCESS(bucket);

            searchResult = lookup(fs, arg1);
            if(searchResult != -1) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
            }
            
            create(fs, arg1, iNumber);

            UNLOCK_ACCESS(bucket);
            break;
        case 'l':
            if(arg1[0] != '0' && (fd = atoi(arg1)) == 0) { /*If arg1 differs from "0" and atoi return 0, then arg1 contains a non-numeric string*/
                return TECNICOFS_ERROR_OTHER;
            }

            iNumber = fileTable->iNumbers[fd];

            if(iNumber == -1) {
                return TECNICOFS_ERROR_FILE_NOT_OPEN;
            } 
            else if((len = inode_get(iNumber, &owner, &ownerPerm, &othersPerm, content, MAX_CONTENT_SIZE, mode, &isOpen)) == -1) {
                return TECNICOFS_ERROR_OTHER;
            }

            else if(!hasPermissionToRead(owner, commandSender, ownerPerm, othersPerm)) {
                return TECNICOFS_ERROR_PERMISSION_DENIED;
            }
            else if (mode[0] != 'r') {
                return TECNICOFS_ERROR_INVALID_MODE;
            }
            else if (isOpen == 0) {
                return TECNICOFS_ERROR_FILE_NOT_OPEN;
            }

            break;
        case 'o':
            LOCK_READ_ACCESS(bucket);

            searchResult = lookup(fs, arg1);

            if(searchResult != -1) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_FILE_NOT_FOUND;
            }

            else if(inode_get(searchResult, NULL, NULL, NULL, NULL, 0, NULL, &isOpen) == -1) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_OTHER;
            }

            else if(isOpen) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_FILE_IS_OPEN;
            }

            if(fileTable->nOpenedFiles == MAX_OPEN_FILES) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_MAXED_OPEN_FILES;
            }

            inode_open(searchResult, arg2[0]);

            for(int i = 0; i < MAX_OPEN_FILES; i++) {
                if(fileTable->iNumbers[i] == -1) {
                    fileTable->iNumbers[i] = searchResult;
                    fileTable->nOpenedFiles++;
                    break;
                }
            }

            UNLOCK_ACCESS(bucket);
            break;
        case 'x':
            if(arg1[0] != '0' && (fd = atoi(arg1)) == 0) { /*If arg1 differs from "0" and atoi return 0, then arg1 contains a non-numeric string*/
                return TECNICOFS_ERROR_OTHER;
            }

            iNumber = fileTable->iNumbers[fd];

            if(inode_get(iNumber, NULL, NULL, NULL, NULL, 0, NULL, &isOpen) == -1) {
                return TECNICOFS_ERROR_OTHER;
            }
            else if(!isOpen) {
                return TECNICOFS_ERROR_FILE_NOT_OPEN;
            }

            inode_close(iNumber);
            fileTable->iNumbers[fd] = -1;

            break;
        case 'w':
            if(arg1[0] != '0' && (fd = atoi(arg1)) == 0) { /*If arg1 differs from "0" and atoi return 0, then arg1 contains a non-numeric string*/
                return TECNICOFS_ERROR_OTHER;
            }

            
            break;
        case 'd':
            LOCK_WRITE_ACCESS(bucket);
            searchResult = lookup(fs, arg1);

            if(searchResult != -1) {
                UNLOCK_ACCESS(bucket);
                return TECNICOFS_ERROR_FILE_NOT_FOUND;
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

            if(searchResult != -1) {
                multipleUnlock(currentBucket, newBucket);
                return TECNICOFS_ERROR_FILE_NOT_FOUND;
            }

            if (lookup(fs, arg2) == -1) {
                delete(fs, arg1);
                create(fs, arg2, searchResult);
            }

            multipleUnlock(currentBucket, newBucket);
            break;
        case 's':
            result = 1;
            break;
        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
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
Funcao responsavel por destruir as tarefas inicializadas 
no vetor -threadIds-
------------------------------------------------------------------*/
void destroyThreads(pthread_t threadIds[], int numMaxThreads) {
    for (int i = 0; i < numMaxThreads; i++) {
        if(pthread_join(threadIds[i], NULL)) {
            perror("Unable to terminate thread");
        }
    }
}

/*------------------------------------------------------------------
Funcao chamada pela tarefa inicial para carregar os comandos do
ficheiro e inserir no vetor -inputCommands-
------------------------------------------------------------------*/
void produtor(char* const argv[], pthread_t threadIds[], int numMaxThreads) {
    FILE *fp;
    if(!(fp = fopen(argv[1], "r"))) {
        perror("Unable to open file for reading");
        exit(EXIT_FAILURE);
    }
    char line[MAX_INPUT_SIZE];
    int status;
    while (fgets(line, sizeof(line)/sizeof(char), fp)) {
        WAIT_PODE_PROD();
        status = processInput(line);
        if (status) { POST_PODE_CONS(); }
    }
    WAIT_PODE_PROD();
    processInput("q!");
    POST_PODE_CONS();
    destroyThreads(threadIds, numMaxThreads);
    fclose(fp);
}

/*------------------------------------------------------------------
Funcao executada pelas tarefas escravas. Assim que estas estejam 
livres, deverao remover os comandos do vetor -inputCommands- 
e executa-los
------------------------------------------------------------------*/
/*void *consumidor() {
    while (1) {
        LOCK_COMMAND();
        if (strcmp(inputCommands[headQueue], "q!")) { //Mudar isto. Verificação constante desnecessária
            UNLOCK_COMMAND();
            WAIT_PODE_CONS();
            applyCommands();
            POST_PODE_PROD(); //Assim que se copia o comando, é preciso dizer ao produtor para produzir
        }
        else {
            UNLOCK_COMMAND();
            POST_PODE_CONS();
            return NULL;
        }
    }
}*/

/*------------------------------------------------------------------
Esta funcao e' responsavel por criar o numero de threads indicadas 
por numMaxThreads
--------------------------------------------------------------------*/
/*void createThreads(pthread_t threadIds[], int numMaxThreads) {
    for(int i = 0; i < numMaxThreads; i++) {
        if(pthread_create(&(threadIds[i]), NULL, consumidor, NULL)) {
            perror("Unable to create thread in createThreads(int numMaxThreads)");
        }
    }
}*/

/*Funcao responsavel por inicializar os locks e semaforos*/
void initLocks() {
    if(pthread_mutex_init(&mutexCommand, NULL)) {
        perror("Unable to initialize mutexCommand");
    }
    if(pthread_mutex_init(&mutexProd, NULL)) {
        perror("Unable to initialize mutexProd");
    }

    if (sem_init(&pode_prod, 0, MAX_COMMANDS)) {
        perror("Unable to iniatialize pode_prod");
    }
    if (sem_init(&pode_cons, 0, 0)) {
        perror("Unable to iniatialize pode_cons");
    }
}

/*Funcao responsavel por destruir os locks e semaforos*/
void destroyLocks() {
    if(pthread_mutex_destroy(&mutexCommand)) {
        perror("Unable to destroy mutexCommand in main(int agrc, char* argv[])");
    }
    if(pthread_mutex_destroy(&mutexProd)) {
        perror("Unable to destroy mutexProd in main(int agrc, char* argv[])");
    }

    if (sem_destroy(&pode_prod)) {
        perror("Unable to destroy pode_prod");
    }
    if (sem_destroy(&pode_cons)) {
        perror("Unable to destroy pode_cons");
    }
}

void *threadFunc(void *cfd) {
    char buffer[MAX_INPUT_SIZE];
    char command;
    char filename[MAX_INPUT_SIZE];
    char perm[2];
    char content[MAX_CONTENT_SIZE];
    int sock = *((int *) cfd);
    open_file_table fileTable;
    uid_t owner;
    struct ucred info;

    open_file_table_init(&fileTable);

    getsockopt(sock, 0, 0, &info, NULL);

    owner = info.uid;

    while(1) {
        memset(buffer, 0, MAX_INPUT_SIZE);
        read(sock, buffer, MAX_INPUT_SIZE);        
        sscanf(buffer, "%c %s %s", &command, filename, perm);
        int success = applyCommands(command, filename, perm, owner, sock, content, &fileTable);
        if (success == 1)
            break;
        write(sock, &success, sizeof(int));
        if (success == 0 && *content != '\0') {
            write(sock, content, strlen(content));
        }
    }
    close(sock);
    return NULL;
}

void apanhaCTRLC(int s) {
    close(sfd);
    for (int i = 0; i < nThreads; i++) {
        if (pthread_join(threadIds[i], NULL))
            perror("Unable to join");
    }
    print_tecnicofs_tree(fp, fs);

    free_tecnicofs(fs);

    destroyLocks();
    
    exit(EXIT_SUCCESS);
}

/*------------------------------------------------------------------
Funcao main
--------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
    parseArgs(argc, argv);
    numBuckets = atoi(argv[3]);

    
    /*if(numMaxThreads <= 0) {
        perror("Number of threads must be a number greater than 0");
        exit(EXIT_FAILURE);
    }*/
    if(numBuckets <= 0) {
        perror("Number of buckets must be a number greater than 0");
        exit(EXIT_FAILURE);
    }

    if(!(fp = fopen(argv[2], "w"))) {
        perror("Unable to open file for writing");
        exit(EXIT_FAILURE);
    }

    initLocks();

    fs = new_tecnicofs(numBuckets);
    
    /*double time_ini = getTime();

    if(MULTITHREADING) {
        createThreads(threadIds, numMaxThreads);  
    }
    else {
        numMaxThreads = 1;
        createThreads(threadIds, numMaxThreads);
    }
    produtor(argv, threadIds, numMaxThreads);

    double time_f = getTime();

    printf("TecnicoFs completed in %0.4f seconds.\n", time_f-time_ini);*/

    /*if(fclose(fp)) {
        perror("Unable to close file");
    }*/

    signal(SIGINT, apanhaCTRLC);

    newServer(&sfd, argv[1]);

    int ix = 0;

    while (1) {
        int new_sock;
        getNewSocket(&new_sock, sfd);
        pthread_create(&threadIds[ix], 0, threadFunc, (void *) &new_sock);
        nThreads += 1;
        ix += 1;
    }

    print_tecnicofs_tree(fp, fs);

    free_tecnicofs(fs);

    destroyLocks();
    
    exit(EXIT_SUCCESS);
}
