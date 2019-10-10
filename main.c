#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include "fs.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

/*Vetor de Strings que guarda os comandos*/
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

/*Variáveis globais*/
pthread_rwlock_t rwLock;
pthread_mutex_t mutex;
int numberThreads = 0;
tecnicofs* fs;


static void displayUsage (const char* appName){
    printf("Usage: %s\n", appName);
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

        /*Lock*/
        if(pthread_mutex_lock(&mutex)) {
            perror("Unable to mutex lock in applyCommands()");
        }

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

        /*Unlock*/
        if(pthread_mutex_unlock(&mutex)) {
            perror("Unable to mutex unlock in applyCommands()");
        }

        switch (token) {
            case 'c':
                /*Lock*/
                if(pthread_rwlock_wrlock(&rwLock)) {
                    perror("Unable to rwlock in applyCommands()");
                }
                
                create(fs, name, iNumber);

                /*Unlock*/
                if(pthread_rwlock_unlock(&rwLock)) {
                    perror("Unable to rwunlock in applyCommands()");
                }
                break;
            case 'l':
                /*Lock*/
                if(pthread_rwlock_rdlock(&rwLock)) {
                    perror("Unable to rwlock in applyCommands()");
                }

                searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                
                /*Unlock*/
                if(pthread_rwlock_unlock(&rwLock)) {
                    perror("Unable to rwunlock in applyCommands()");
                } 
                break;
            case 'd':
                /*Lock*/
                if(pthread_rwlock_wrlock(&rwLock)) {
                    perror("Unable to rwlock in applyCommands()");
                }

                delete(fs, name);
                
                /*Unlock*/
                if(pthread_rwlock_unlock(&rwLock)) {
                    perror("Unable to rwunlock in applyCommands()");
                }         
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
    if(pthread_mutex_init(&mutex, NULL)) {
        perror("Unable to initialize mutex");
    }

    parseArgs(argc, argv);

    fs = new_tecnicofs();
    processInput(argv);

    
    double time_ini = getTime();

    createThreads(numMaxThreads);

    double time_f = getTime();

    printf("TecnicoFs completed in %0.4f seconds\n", time_f-time_ini);
    

    print_tecnicofs_tree(fp, fs);

    fclose(fp);
    free_tecnicofs(fs);

    if(pthread_rwlock_destroy(&rwLock)) {
        perror("Unable to destroy rwLock in main(int agrc, char* argv[])");
    }
    if(pthread_mutex_destroy(&mutex)) {
        perror("Unable to destroy mutex in main(int agrc, char* argv[])");
    }

    exit(EXIT_SUCCESS);
}
