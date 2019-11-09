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
#define LOCK_PROD()

#define LOCK_WRITE_ACCESS(bucket) doNothing(bucket)
#define LOCK_READ_ACCESS(bucket) doNothing(bucket)

#define WAIT_PODE_PROD() 
#define WAIT_PODE_CONS() 
#define POST_PODE_PROD() 
#define POST_PODE_CONS() 

#define UNLOCK_COMMAND()
#define UNLOCK_PROD()
#define UNLOCK_ACCESS(bucket) doNothing(bucket)

#define MULTITHREADING 0 /*false*/
#endif /*Teste de existência de macros*/

#if MULTITHREADING
#define LOCK_PROD() if(pthread_mutex_lock(&mutexProd)) perror("Unable to mutex lock in applyCommands()")
#define UNLOCK_PROD() if(pthread_mutex_unlock(&mutexProd)) perror("Unable to mutex unlock in applyCommands()")
#define WAIT_PODE_PROD() if(sem_wait(&pode_prod)) perror("Unable to sem_wait in produtor()")
#define WAIT_PODE_CONS() if(sem_wait(&pode_cons)) perror("Unable to sem_wait in consumidor()")
#define POST_PODE_PROD() if(sem_post(&pode_prod)) perror("Unable to sem_post in consumidor()")
#define POST_PODE_CONS() if(sem_post(&pode_cons)) perror("Unable to sem_post in produtor()")
#endif 


#define MAX_COMMANDS 10 /*Mudei este valor para comecar a execucao incremental*/
#define MAX_INPUT_SIZE 100
#define EXIT_COMMAND 'q'

/*Vetor de Strings que guarda os comandos*/
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int prodCommands = 0;
int consCommands = 0;
int headQueue = 0;

/*Variáveis globais*/
pthread_rwlock_t rwLock;
pthread_mutex_t mutexCommand;
pthread_mutex_t mutexProd;
/*pthread_mutex_t mutexAccess;*/
tecnicofs *fs;
int numMaxThreads;
int numBuckets;
int status = 1; /*status = 1 while a consumer hasn't yet read the EXIT_COMMAND*/
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
void insertCommand(char* data) {
    strcpy(inputCommands[prodCommands], data);
    prodCommands = (prodCommands+1)%MAX_COMMANDS;
}

/*Devolve o proximo comando em -inputCommands- */
char* removeCommand() {
    char *line = inputCommands[headQueue];
    headQueue = (headQueue+1)%MAX_COMMANDS;
    return line;
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
//se retornar 0 entao nao foi inserido nada e nao podemos 
//fazer sem_post, o consumidor ainda nao pode consumir
void processInput(char *line){
    //while (fgets(line, sizeof(line)/sizeof(char), fp)) {
        char token;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s", &token, name);
        //printf("numTokens: %d\n", numTokens);
        //printf("Command being processed: %c, arguments: %s\n", token, name);
        /* perform minimal validation */
        if (numTokens < 1) {
            perror("An empty line was processed in processInput(char* line)");
            return;
        }

        switch (token) {
            case 'c':
            case 'l':
            case 'd':
                if(numTokens != 2) {
                    errorParse();
                    return;
                }
                WAIT_PODE_PROD();
                LOCK_PROD();
                insertCommand(line);
                UNLOCK_PROD();
                POST_PODE_CONS();
                break;
            case 'r':
                if(numTokens != 3) {
                    errorParse();
                    return;
                }
                WAIT_PODE_PROD();
                LOCK_PROD();
                insertCommand(line);
                UNLOCK_PROD();
                POST_PODE_CONS();
                break;
            case EXIT_COMMAND:
                WAIT_PODE_PROD();
                LOCK_PROD();
                //printf("INSERTING QUIT COMMAND\n");
                insertCommand(line);
                UNLOCK_PROD();
                POST_PODE_CONS();
                break;
            case '#':
                break;
            default: { /* error */
                errorParse();
                break;
            }
        }
    //}
}

/*------------------------------------------------------------------
Funcao responsavel por ler os comandos do vetor InputComandos
e de aplicar os respestivos comandos
------------------------------------------------------------------*/
void applyCommands(){
    int searchResult;
    int iNumber;
    int bucket;
    char token;
    char name[MAX_INPUT_SIZE];
    char currentName[100];
    char newName[100];

    //while(!endOfFile){

        WAIT_PODE_CONS();
        //printf("Can consume. Going to consume\n");
        LOCK_COMMAND();
        //printf("Command currently on headQueue: %c\n", inputCommands[headQueue][0]);
        if(inputCommands[headQueue][0] == EXIT_COMMAND) {
            status = 0;
            UNLOCK_COMMAND();
            //printf("Command 'q' read, status changed to 0\n");
            POST_PODE_CONS();
            return;
        }

        const char* command = removeCommand();

        int numTokens = sscanf(command, "%c %s", &token, name);
        //printf("Command read- token: %c, name- %s\n", token, name);

        bucket = hash(name, numBuckets);

        if(token == 'c') {
            iNumber = obtainNewInumber(fs);
        }   

        UNLOCK_COMMAND();

        switch (token) {
            case 'c':
                if(numTokens != 2) {
                    fprintf(stderr, "Error: command 'c' can only have 1 argument\n");
                    break;
                }

                LOCK_WRITE_ACCESS(bucket);
                
                create(fs, name, iNumber);

                UNLOCK_ACCESS(bucket);
                POST_PODE_PROD();
                break;
            case 'l':
                if(numTokens != 2) {
                    fprintf(stderr, "Error: command 'l' can only have 1 argument\n");
                    break;
                }

                LOCK_READ_ACCESS(bucket);

                searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                
                UNLOCK_ACCESS(bucket);
                POST_PODE_PROD();
                break;
            case 'd':
                if(numTokens != 2) {
                    fprintf(stderr, "Error: command 'd' can only have 1 argument\n");
                    break;
                }

                LOCK_WRITE_ACCESS(bucket);

                delete(fs, name);
                
                UNLOCK_ACCESS(bucket);
                POST_PODE_PROD();       
                break;
            case 'r':
                if(numTokens != 3) {
                    fprintf(stderr, "Error: command 'r' can only have 2 argument\n");
                    break;
                }

                sscanf((command+2), "%s %s", currentName, newName);
                searchResult = lookup(fs, currentName);
                if (searchResult && !lookup(fs, newName)) {
                    delete(fs, currentName);
                    create(fs, newName, searchResult);
                }
                POST_PODE_PROD();
                break;
            default:  /* error */
                fprintf(stderr, "Error: command to apply\n");
                break;
        }
        //printf("Pode produzir\n");
    //}
    //return NULL;
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

void destroyThreads(pthread_t threadIds[], int numMaxThreads) {
    for (int i = 0; i < numMaxThreads; i++) {
        if(pthread_join(threadIds[i], NULL)) {
            perror("Unable to terminate thread");
        }
        //printf("A THREAD finished his job\n");
    }
}

//TODO
void produtor(char* const argv[], pthread_t threadIds[], int numMaxThreads) {
    FILE *fp;
    fp = fopen(argv[1], "r");
    char line[MAX_INPUT_SIZE];
    while (fgets(line, sizeof(line)/sizeof(char), fp)) {
        //printf("Going to process line: %s\n", line);
        processInput(line);
    }
    processInput("q");
    //printf("Going to destroy threads\n");
    destroyThreads(threadIds, numMaxThreads);
    fclose(fp);
}

//TODO
void *consumidor() {
    while (1) {
        LOCK_COMMAND();
        if (status) {
            UNLOCK_COMMAND();
            applyCommands();
        }
        else {
            //printf("Status false. Terminating thread\n");
            UNLOCK_COMMAND();
            return NULL;
        }
    }
}

/*------------------------------------------------------------------
Esta funcao e' responsavel por criar o numero de threads em 
numMaxThreads e de seguida devera destrui-las
--------------------------------------------------------------------*/
void createThreads(pthread_t threadIds[], int numMaxThreads) {
    for(int i = 0; i < numMaxThreads; i++) {
        if(pthread_create(&(threadIds[i]), NULL, consumidor, NULL)) {
            perror("Unable to create thread in createThreads(int numMaxThreads)");
        }
    }
}

void initLocks() {
    if(pthread_rwlock_init(&rwLock, NULL)) {
        perror("Unable to initialize rwLock");
    }
    if(pthread_mutex_init(&mutexCommand, NULL)) {
        perror("Unable to initialize mutexCommand");
    }
    /*if(pthread_mutex_init(&mutexAccess, NULL)) {
        perror("Unable to initialize mutexAccess");
    }*/

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

void destroyLocks() {
    if(pthread_rwlock_destroy(&rwLock)) {
        perror("Unable to destroy rwLock in main(int agrc, char* argv[])");
    }
    if(pthread_mutex_destroy(&mutexCommand)) {
        perror("Unable to destroy mutexCommand in main(int agrc, char* argv[])");
    }
    /*if(pthread_mutex_destroy(&mutexAccess)) {
        perror("Unable to destroy mutexAccess in main(int agrc, char* argv[])");
    }*/

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

/*------------------------------------------------------------------
Funcao main responsavel pela criacao de um tecnicofs e por chamar 
as funcoes processInput e createThreads
--------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
    parseArgs(argc, argv);
    FILE *fp;
    numMaxThreads = atoi(argv[3]);
    numBuckets = atoi(argv[4]);
    pthread_t threadIds[numMaxThreads];

    if(numMaxThreads <= 0) {
        perror("Number of threads must be a number greater than 0");
        exit(EXIT_FAILURE);
    }
    if(numBuckets <= 0) {
        perror("Number of buckets must be a number greater than 0");
        exit(EXIT_FAILURE);
    }

    if(!(fp = fopen(argv[2], "w"))) {
        perror("Unable to open file");
        exit(EXIT_FAILURE);
    }

    initLocks();

    fs = new_tecnicofs(numBuckets);
    
    double time_ini = getTime();

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

    print_tecnicofs_tree(fp, fs);
    
    free_tecnicofs(fs);
    

    if(fclose(fp)) {
        perror("Unable to close file");
    }

    destroyLocks();

    exit(EXIT_SUCCESS);
}
