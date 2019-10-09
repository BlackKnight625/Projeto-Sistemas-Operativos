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

/*Definicao de structs*/

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

/*Verifica e lida com os argumentos passados na consola para o programa. */
static void parseArgs (long argc, char* const argv[]){
    if (argc != 4) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
}

/*Insere o comando e seu argumento em -data- no vetor -inputCommands-*/
int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

/*Devolve o proximo comando em -inputCommands-. Caso nao existe, devolve NULL*/
char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

/*Imprime "Error: command invalid"*/
void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    //exit(EXIT_FAILURE);
}

/*Converte o input do ficheiro fornecido para o vetor -inputCommands-*/
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

/*Corre os comandos presentes em -inputCommands-*/
void *applyCommands(){
    int searchResult;
    int iNumber;
    char token;
    char name[MAX_INPUT_SIZE];

    while(numberCommands > 0){
        const char* command = removeCommand();
        if(command == NULL){
            continue;
        }

        /*Lock*/
        if(pthread_mutex_lock(&mutex)) {
            perror("Unable to mutex lock in applyCommands()");
        }

        int numTokens = sscanf(command, "%c %s", &token, name);

        if(token == 'c') {
            iNumber = obtainNewInumber(fs);
        }

        /*Unlock*/
        if(pthread_mutex_unlock(&mutex)) {
            perror("Unable to mutex unlock in applyCommands()");
        }

        if(numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        printf("Thread running %c command on directory %s\n", token, name);
       
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
                break;

                /*Unlock*/
                if(pthread_rwlock_unlock(&rwLock)) {
                    perror("Unable to rwunlock in applyCommands()");
                }  
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

void createThreads(int numMaxThreads) {
    pthread_t threadIds[numMaxThreads];
    for(int i = 0; i < numMaxThreads; i++) {
        if(pthread_create(&(threadIds[i]), NULL, applyCommands, NULL)) {
            perror("Unable to create thread in createThreads(int numMaxThreads)");
        }
    }
    for (int i = 0; i < numMaxThreads; i++) {
        if(pthread_join(threadIds[i],NULL)) {
            perror("Unable to terminate thread");
        }
    }
}

/* Main FUNKKKKKK */
int main(int argc, char* argv[]) {
    struct timeval tv;
    time_t i_secs;
    suseconds_t i_msecs;
    time_t f_secs;
    suseconds_t f_msecs;
    
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

    
    gettimeofday(&tv, NULL);
    i_secs = tv.tv_sec;
    i_msecs = tv.tv_usec;

    createThreads(numMaxThreads);

    gettimeofday(&tv, NULL);
    f_secs = tv.tv_sec;
    f_msecs = tv.tv_usec;
    printf("%ld.00%ld\n", f_secs - i_secs, f_msecs - i_msecs);

    print_tecnicofs_tree(fp, fs);

    fclose(fp);
    free_tecnicofs(fs);

    if(pthread_rwlock_destroy(&rwLock)) {
        perror("Unable to detroy rwLock in main(int agrc, char* argv[]");
    }
    if(pthread_mutex_destroy(&mutex)) {
        perror("Unable to detroy mutex in main(int agrc, char* argv[]");
    }

    exit(EXIT_SUCCESS);
}
