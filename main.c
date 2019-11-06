#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include "fs.h"
#include "lib/hash.h"

void doNothing(int bucket);

/*Testa existência de macros no compilador*/
#if defined MUTEX
#define LOCK_COMMAND() if (pthread_mutex_lock(&mutexCommand)) perror("Unable to mutex lock in applyCommands()")
#define LOCK_WRITE_ACCESS(bucket) if (pthread_mutex_lock(&fs->mutexs[bucket])) perror("Unable to mutex lock in applyCommands()") 
#define LOCK_READ_ACCESS(bucket) if (pthread_mutex_lock(&fs->mutexs[bucket])) perror("Unable to mutex lock in applyCommands()") 

#define UNLOCK_COMMAND() if (pthread_mutex_unlock(&mutexCommand)) perror("Unable to mutex unlock in applyCommands()") 
#define UNLOCK_ACCESS(bucket) if (pthread_mutex_unlock(&fs->mutexs[bucket])) perror("Unable to mutex unlock in applyCommands()") 

#define MULTITHREADING 1 /*true*/


#elif defined RWLOCK
#define LOCK_COMMAND() if (pthread_mutex_lock(&mutexCommand)) perror("Unable to mutex lock in applyCommands()") 
#define LOCK_WRITE_ACCESS(bucket) if (pthread_rwlock_wrlock(&fs->rwlocks[bucket])) perror("Unable to rwlock in applyCommands()") 
#define LOCK_READ_ACCESS(bucket) if (pthread_rwlock_rdlock(&fs->rwlocks[bucket])) perror("Unable to rwlock in applyCommands()") 

#define UNLOCK_COMMAND() if (pthread_mutex_unlock(&mutexCommand)) perror("Unable to mutex lock in applyCommands()") 
#define UNLOCK_ACCESS(bucket) if (pthread_rwlock_unlock(&fs->rwlocks[bucket])) perror("Unable to rwlock in applyCommands()")

#define MULTITHREADING 1 /*true*/

/*Nenhuma macro previamente definida*/
#else
#define LOCK_COMMAND()
#define LOCK_WRITE_ACCESS(bucket) doNothing(bucket)
#define LOCK_READ_ACCESS(bucket) doNothing(bucket)

#define UNLOCK_COMMAND()
#define UNLOCK_ACCESS(bucket) doNothing(bucket)

#define MULTITHREADING 0 /*false*/
#endif /*Teste de existência de macros*/


#define MAX_COMMANDS 10 /*Mudei este valor para comecar a execucao incremental*/
#define MAX_INPUT_SIZE 100

/*Vetor de Strings que guarda os comandos*/
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

/*Variáveis globais*/
pthread_rwlock_t rwLock;
pthread_mutex_t mutexCommand;
/*pthread_mutex_t mutexAccess;*/
tecnicofs *fs;
int numMaxThreads;
int numBuckets;
sem_t pode_prod;
sem_t pode_cons;

/*Mostra como se chama corretamente o programa*/
static void displayUsage (const char* appName){
    printf("Usage: %s inputfile outputfile numThreads numBuckets\n", appName);
    exit(EXIT_FAILURE);
}

/*Verifica que a funcao recebeu todos os argumentos*/
static void parseArgs (long argc, char* const argv[]){
    if (argc != 5) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
}

void doNothing(int bucket) {
    (void)bucket;
}

/*Insere o comando fornecido por -data- no vetor -inputCommands-*/
int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

/*Devolve o proximo comando em -inputCommands- */
char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

/*Funcao de erro caso os inputs nao sejam corretos*/
void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/*------------------------------------------------------------------
Converte o input do ficheiro fornecido para o vetor -inputCommands-
Termina se nao conseguir abrir o ficheiro dado em argv
------------------------------------------------------------------*/
void processInput(char* const argv[]){
    FILE *fp;
    if(!(fp = fopen(argv[1], "r"))) {
        perror("Unable to open file");
        exit(EXIT_FAILURE);
    }
    char line[MAX_INPUT_SIZE];
    while (fgets(line, sizeof(line)/sizeof(char), fp)) {
        char token;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s", &token, name);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
            case 'l':
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            case '#':
                break;
            default: { /* error */
                errorParse();
            }
        }
    }
    if(fclose(fp)) {
        perror("Unable to close file");
    }
}

/*------------------------------------------------------------------
Funcao responsavel por ler os comandos do vetor InputComandos
e de aplicar os respestivos comandos
------------------------------------------------------------------*/
void *applyCommands(){
    int searchResult;
    int iNumber;
    int bucket;
    char token;
    char name[MAX_INPUT_SIZE];

    while(numberCommands > 0){
        LOCK_COMMAND();

        const char* command = removeCommand();
        if(command == NULL){
            UNLOCK_COMMAND();
            continue;
        }

        int numTokens = sscanf(command, "%c %s", &token, name);

        bucket = hash(name, numBuckets);

        if(token == 'c') {
            iNumber = obtainNewInumber(fs);
        }   

        if(numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        UNLOCK_COMMAND();

        switch (token) {
            case 'c':
                LOCK_WRITE_ACCESS(bucket);
                
                create(fs, name, iNumber);

                UNLOCK_ACCESS(bucket);
                break;
            case 'l':
                LOCK_READ_ACCESS(bucket);

                searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                
                UNLOCK_ACCESS(bucket);
                break;
            case 'd':
                LOCK_WRITE_ACCESS(bucket);

                delete(fs, name);
                
                UNLOCK_ACCESS(bucket);       
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

/*------------------------------------------------------------------
Esta funcao e' responsavel por criar o numero de threads em 
numMaxThreads e de seguida devera destrui-las
--------------------------------------------------------------------*/
void createThreads() {
    pthread_t threadIds[numMaxThreads];
    for(int i = 0; i < numMaxThreads; i++) {
        if(pthread_create(&(threadIds[i]), NULL, applyCommands, NULL)) {
            perror("Unable to create thread in createThreads(int numMaxThreads)");
        }
    }
    for (int i = 0; i < numMaxThreads; i++) {
        if(pthread_join(threadIds[i], NULL)) {
            perror("Unable to terminate thread");
        }
    }
}

/* -----------------------------------------------------------------
Esta funcao devolve um double cujo valor e' o tempo em segundos e 
microsegundos desde o inicio dos tempos
------------------------------------------------------------------*/
double getTime() {
    struct timeval tv;
    double secs;
    double msecs;
    gettimeofday(&tv, NULL);
    secs = (double) tv.tv_sec;
    msecs = (double) tv.tv_usec;
    return secs + msecs/1000000;
}

void produtor() {
    sem_wait(&pode_prod);
}

void consumidor() {
    sem_wait(&pode_cons);
}

/*------------------------------------------------------------------
Funcao main responsavel pela criacao de um tecnicofs e por chamar 
as funcoes processInput e createThreads
--------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
    parseArgs(argc, argv);
    FILE *fp;


    if(pthread_rwlock_init(&rwLock, NULL)) {
        perror("Unable to initialize rwLock");
    }
    if(pthread_mutex_init(&mutexCommand, NULL)) {
        perror("Unable to initialize mutexCommand");
    }
    /*if(pthread_mutex_init(&mutexAccess, NULL)) {
        perror("Unable to initialize mutexAccess");
    }*/

    if(!(fp = fopen(argv[2], "w"))) {
        perror("Unable to open file");
        exit(EXIT_FAILURE);
    }
    
    if (sem_init(&pode_prod, 0, 10)) {
        perror("Unable to iniatialize semInit");
    }
    if (sem_init(&pode_cons, 0, 0)) {
        perror("Unable to iniatialize semInit");
    }



    processInput(argv);

    numMaxThreads = atoi(argv[3]);
    numBuckets = atoi(argv[4]);

    fs = new_tecnicofs(numBuckets);

    if(numMaxThreads <= 0) {
        perror("Number of threads must be a number greater than 0");
        exit(EXIT_FAILURE);
    }
    if(numBuckets <= 0) {
        perror("Number of buckets must be a number greater than 0");
        exit(EXIT_FAILURE);
    }
    
    double time_ini = getTime();

    if(MULTITHREADING) {
        createThreads();  
    }
    else {
        applyCommands();
    }


    double time_f = getTime();

    printf("TecnicoFs completed in %0.4f seconds.\n", time_f-time_ini);

    print_tecnicofs_tree(fp, fs);
    
    free_tecnicofs(fs);
    

    if(fclose(fp)) {
        perror("Unable to close file");
    }

    if(pthread_rwlock_destroy(&rwLock)) {
        perror("Unable to destroy rwLock in main(int agrc, char* argv[])");
    }
    if(pthread_mutex_destroy(&mutexCommand)) {
        perror("Unable to destroy mutexCommand in main(int agrc, char* argv[])");
    }
    /*if(pthread_mutex_destroy(&mutexAccess)) {
        perror("Unable to destroy mutexAccess in main(int agrc, char* argv[])");
    }*/
    
    if (sem_destroy(&pode_prod)) {
        perror("Unable to destroy semDestroy");
    }
    if (sem_destroy(&pode_cons)) {
        perror("Unable to destroy semDestroy");
    }

    exit(EXIT_SUCCESS);
}
