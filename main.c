#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
tecnicofs* fs;

/*Vetor de Strings que guarda os comandos*/
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;


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
void applyCommands(char* const argv[]){
    FILE *fp;
    fp = fopen(argv[2], "w");
    while(numberCommands > 0){
        const char* command = removeCommand();
        if (command == NULL){
            continue;
        }

        char token;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s", &token, name);
        if (numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                iNumber = obtainNewInumber(fs);
                create(fs, name, iNumber);
                break;
            case 'l':
                searchResult = lookup(fs, name);
                if(!searchResult)
                    fprintf(fp, "%s not found\n", name);
                else
                    fprintf(fp, "%s found with inumber %d\n", name, searchResult);
                break;
            case 'd':
                delete(fs, name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    fclose(fp);
}



/* Main FUNKKKKKK */
int main(int argc, char* argv[]) {
    FILE *fp;
    fp = fopen(argv[2], "a");
    parseArgs(argc, argv);

    fs = new_tecnicofs();
    processInput(argv);
    //criar pool de tarefas
    applyCommands(argv);
    print_tecnicofs_tree(fp, fs);

    fclose(fp);
    free_tecnicofs(fs);
    exit(EXIT_SUCCESS);
}
