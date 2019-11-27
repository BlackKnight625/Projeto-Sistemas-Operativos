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


/*Vetor de Strings que guarda os comandos*/
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int prodCommands = 0;
int consCommands = 0;
int headQueue = 0;

/*Variáveis globais*/
pthread_mutex_t mutexCommand;
pthread_mutex_t mutexProd;
tecnicofs *fs;
int numMaxThreads;
int numBuckets;
sem_t pode_prod;
sem_t pode_cons;

/*Mostra como se chama corretamente o programa*/
/*static void displayUsage (const char* appName){
    printf("Usage: %s inputfile outputfile numThreads numBuckets\n", appName);
    exit(EXIT_FAILURE);
}*/

/*Verifica que a funcao recebeu todos os argumentos*/
/*static void parseArgs (long argc, char* const argv[]){
    if (argc != 5) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
}*/

/*Faz nada. Absolutamente nada*/
void doNothing(int bucket) {
    (void)bucket;
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

/*------------------------------------------------------------------
Funcao responsavel por ler um comando do vetor -inputCommands-
e de o executar. Esta funcao e' chamada pelos consumidores
------------------------------------------------------------------*/
int applyCommands(char command, char arg1[], char arg2[], uid_t owner){
    int searchResult;
    int iNumber;
    int bucket;
    int currentBucket;
    int newBucket;

    if(command == 'c') {
        iNumber = obtainNewInumber(fs);
    }   

    switch (command) { /*Generates the hash for the commands that need it*/
        case 'c':
        case 'l':
        case 'd':
        case 'r':
            bucket = hash(arg1, numBuckets);
            break;
    }

    switch (command) {
        case 'c':
            LOCK_WRITE_ACCESS(bucket);


            
            create(fs, arg1, iNumber);

            UNLOCK_ACCESS(bucket);
            break;
        case 'l':
            LOCK_READ_ACCESS(bucket);

            searchResult = lookup(fs, arg1);
            if(!searchResult)
                printf("%s not found\n", arg1);
            else
                printf("%s found with inumber %d\n", arg1, searchResult);
            
            UNLOCK_ACCESS(bucket);
            break;
        case 'd':
            LOCK_WRITE_ACCESS(bucket);

            delete(fs, arg1);
            
            UNLOCK_ACCESS(bucket);       
            break;
        case 'r':

            currentBucket = bucket;
            newBucket = hash(arg2, numBuckets);

            multipleLock(currentBucket, newBucket);

            searchResult = lookup(fs, arg1);

            if (searchResult && !lookup(fs, arg2)) {
                delete(fs, arg1);
                create(fs, arg2, searchResult);
            }

            UNLOCK_ACCESS(currentBucket);
            if (currentBucket != newBucket) {
                UNLOCK_ACCESS(newBucket);
            }
            break;
        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
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
    char buffer[100];
    char command;
    char filename[100];
    char perm[2];
    int sock = *((int *) cfd);
    
    uid_t owner;
    struct ucred info;

    getsockopt(sock, 0, 0, &info, NULL);

    owner = info.uid;

    while(1) {
        read(sock, buffer, 100);
        sscanf(buffer, "%c %s %s", &command, filename, perm);
        int success = applyCommands(command, filename, perm, owner);
        write(sock, &success, sizeof(int));
        close(sock);
    }
    return NULL;
}

/*------------------------------------------------------------------
Funcao main
--------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
    /*parseArgs(argc, argv);
    FILE *fp;
    numMaxThreads = atoi(argv[3]);*/
    numBuckets = atoi(argv[4]);
    /*pthread_t threadIds[numMaxThreads];*/

    
    /*if(numMaxThreads <= 0) {
        perror("Number of threads must be a number greater than 0");
        exit(EXIT_FAILURE);
    }*/
    if(numBuckets <= 0) {
        perror("Number of buckets must be a number greater than 0");
        exit(EXIT_FAILURE);
    }

    /*if(!(fp = fopen(argv[2], "w"))) {
        perror("Unable to open file for writing");
        exit(EXIT_FAILURE);
    }*/

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

    printf("TecnicoFs completed in %0.4f seconds.\n", time_f-time_ini);

    print_tecnicofs_tree(fp, fs);*/
    
    free_tecnicofs(fs);

    /*if(fclose(fp)) {
        perror("Unable to close file");
    }*/

    destroyLocks();

    int sfd;
    newServer(&sfd, argv[1]);

    while (1) {
        int new_sock;
        pthread_t tid;
        getNewSocket(&new_sock, sfd);
        pthread_create(&tid, 0, threadFunc, (void *) &new_sock);
    }

    close(sfd);

    destroyLocks();
    
    exit(EXIT_SUCCESS);
}
