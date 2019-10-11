#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include "fs.h"

/*Testa existência de macros no compilador*/
#if defined MUTEX
#define LOCK_COMMAND() pthread_mutex_lock(&mutexCommand) ? perror("Unable to mutex lock in applyCommands()") : NULL
#define LOCK_WRITE_ACCESS() pthread_mutex_lock(&mutexAccess) ? perror("Unable to mutex lock in applyCommands()") : NULL
#define LOCK_READ_ACCESS() pthread_mutex_lock(&mutexAccess) ? perror("Unable to mutex lock in applyCommands()") : NULL

#define UNLOCK_COMMAND() pthread_mutex_unlock(&mutexCommand) ? perror("Unable to mutex unlock in applyCommands()") : NULL
#define UNLOCK_ACCESS() pthread_mutex_unlock(&mutexAccess) ? perror("Unable to mutex unlock in applyCommands()") : NULL

#define MULTITHREADING 1 /*true*/


#elif defined RWLOCK
#define LOCK_COMMAND() pthread_mutex_lock(&mutexCommand) ? perror("Unable to mutex lock in applyCommands()") : NULL
#define LOCK_WRITE_ACCESS() pthread_rwlock_wrlock(&rwLock) ? perror("Unable to rwlock in applyCommands()") : NULL
#define LOCK_READ_ACCESS() pthread_rwlock_rdlock(&rwLock) ? perror("Unable to rwlock in applyCommands()") : NULL

#define UNLOCK_COMMAND() pthread_mutex_unlock(&mutexCommand) ? perror("Unable to mutex lock in applyCommands()") : NULL
#define UNLOCK_ACCESS() pthread_rwlock_unlock(&rwLock) ? perror("Unable to rwlock in applyCommands()") : NULL

#define MULTITHREADING 1 /*true*/

/*Nenhuma macro previamente definida*/
#else
#define LOCK_COMMAND()
#define LOCK_WRITE_ACCESS()
#define LOCK_READ_ACCESS()

#define UNLOCK_COMMAND()
#define UNLOCK_ACCESS()

#define MULTITHREADING 0 /*false*/
#endif /*Teste de existência de macros*/


#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

/*Vetor de Strings que guarda os comandos*/
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

/*Variáveis globais*/
pthread_rwlock_t rwLock;
pthread_mutex_t mutexCommand;
pthread_mutex_t mutexAccess;
int numberThreads = 0;
tecnicofs* fs;

/*Mostra como se chama corretamente o programa*/
static void displayUsage (const char* appName){
    printf("Usage: %s inputfile outputfile numThreads\n", appName);
    exit(EXIT_FAILURE);
}

/*Verifica que a funcao recebeu todos os argumentos*/
static void parseArgs (long argc, char* const argv[]){
    if (argc != 4) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
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
------------------------------------------------------------------*/
void processInput(char* const argv[]){
    FILE *fp;
    fp = fopen(argv[1], "r");
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
    fclose(fp);
}

/*------------------------------------------------------------------
Funcao responsavel por ler os comandos do vetor InputComandos
e de aplicar os respestivos comandos
------------------------------------------------------------------*/
void *applyCommands(){
    int searchResult;
    int iNumber;
    char token;
    char name[MAX_INPUT_SIZE];

    while(numberCommands > 0){

        LOCK_COMMAND();

        const char* command = removeCommand();
        if(command == NULL){
            continue;
        }

        int numTokens = sscanf(command, "%c %s", &token, name);

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
                LOCK_WRITE_ACCESS();
                
                create(fs, name, iNumber);

                UNLOCK_ACCESS();
                break;
            case 'l':
                LOCK_READ_ACCESS();

                searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                
                UNLOCK_ACCESS();
                break;
            case 'd':
                LOCK_WRITE_ACCESS();

                delete(fs, name);
                
                UNLOCK_ACCESS();       
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
void createThreads(int numMaxThreads) {
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

/*------------------------------------------------------------------
Funcao main responsavel pela criacao de um tecnicofs e por chamar 
as funcoes processInput e createThreads
--------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
    FILE *fp;
    int numMaxThreads = atoi(argv[3]);

    fp = fopen(argv[2], "w");

    if(pthread_rwlock_init(&rwLock, NULL)) {
        perror("Unable to initialize rwLock");
    }
    if(pthread_mutex_init(&mutexCommand, NULL)) {
        perror("Unable to initialize mutexCommand");
    }
    if(pthread_mutex_init(&mutexAccess, NULL)) {
        perror("Unable to initialize mutexAccess");
    }

    parseArgs(argc, argv);

    fs = new_tecnicofs();
    processInput(argv);

    
    double time_ini = getTime();

    if(MULTITHREADING) {
        createThreads(numMaxThreads);  
    }
    else {
        applyCommands();
    }


    double time_f = getTime();

    printf("TecnicoFs completed in %0.4f seconds.\n", time_f-time_ini);
    

    print_tecnicofs_tree(fp, fs);

    fclose(fp);
    free_tecnicofs(fs);

    if(pthread_rwlock_destroy(&rwLock)) {
        perror("Unable to destroy rwLock in main(int agrc, char* argv[])");
    }
    if(pthread_mutex_destroy(&mutexCommand)) {
        perror("Unable to destroy mutexCommand in main(int agrc, char* argv[])");
    }
    if(pthread_mutex_destroy(&mutexAccess)) {
        perror("Unable to destroy mutexAccess in main(int agrc, char* argv[])");
    }

    exit(EXIT_SUCCESS);
}
