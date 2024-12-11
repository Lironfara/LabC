#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>  
#include <stdbool.h>
#include "LineParser.h"
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <stdbool.h>   



#define MAX_LENGTH 2048
#define MAX_PATH_SIZE 4096
#define TERMINATED  -1
#define RUNNING 1
#define SUSPENDED 0
char input[MAX_LENGTH]; //string is array of chars
bool debug = false;
int HISTLEN = 10;


typedef struct history {
    struct historyLinkStruct* first; //The first link in history, is the first link tat was inserted, meaning its the first one to go out
    struct historyLinkStruct* last;   //The last link in history, is the last link that was inserted, meaning its the newes
} history;

typedef struct historyLinkStruct {
    char* cmd;
    struct historyLinkStruct* next;
} historyLinkStruct;

typedef struct process{
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid; 		                  /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	                  /* next process in chain */
} process;

process* processList = NULL;
historyLinkStruct* historyLink = NULL;
history* historyList = NULL;


bool isFull(){
    return HISTLEN == 0;
}

void cleanFirst(history** history_List){
    printf("Cleaning first\n");
    history* historyList = *history_List;
    if (historyList->first == NULL || historyList==NULL){
        return;
    }
    historyLinkStruct* first = historyList->first;
    historyList->first = historyList->first->next;
    free(first->cmd);
    free(first);
    HISTLEN++; //beacuse we cleaned 1 link

}

void insertToHistory(history **history_List, char *input) {
    if (input == NULL) {
        perror("Fail to insert to history");
        return;
    }
    if(input[0] == '!'){
        return;
    }

    historyLinkStruct* newLink = (historyLinkStruct*)malloc(sizeof(historyLinkStruct));
    if (newLink == NULL) {
        perror("Fail to allocate memory for new history link");
        return;
    }
    newLink->cmd = strdup(input); // Duplicate the input string
    if (newLink->cmd == NULL) {
        perror("Fail to allocate memory for command string");
        free(newLink);
        return;
    }
    newLink->next = NULL;

    if (*history_List == NULL) {
        printf("Creating new history list\n");
        *history_List = (history*)malloc(sizeof(history));
        if (*history_List == NULL) {
            perror("Fail to allocate memory for new history list");
            free(newLink->cmd);
            free(newLink);
            return;
        }
        (*history_List)->first = newLink;
        (*history_List)->last = newLink;
    } else {
        if (isFull()) {
            cleanFirst(history_List);
        }
        (*history_List)->last->next = newLink;
        (*history_List)->last = newLink;
    }

    HISTLEN--;
    printf("History length: %d\n", HISTLEN);
}

void printHistory(history** history_List){
    history* historyList = *history_List;
    if(historyList == NULL){
        perror("Hisory li");
    }
    else{
        historyLinkStruct* curr = historyList->first;
        int counter = 0;
        while (curr != NULL){
            printf("%d. %s\n", counter, curr->cmd);
            curr = curr->next;
            counter++;
        }
    }
}

void freeHistory(history** history_List){
    history* historyList = *history_List;
    if (historyList == NULL){
        return;
    }
    historyLinkStruct* curr = historyList->first;
    while (curr != NULL){
        historyLinkStruct* next = curr->next;
        free(curr->cmd);
        free(curr);
        curr = next;
    }
    free(historyList);
}

bool isDigit(char c){
    return c >= '0' && c <= '9';
}

historyLinkStruct* getHistory(history** history_List, int index){
    history* historyList = *history_List;
    if (index > 10 - HISTLEN){
        perror("Index out of range");
        return NULL;
    }
    historyLinkStruct* curr = historyList->first;
    for (int i = 0; i < index; i++){
        curr = curr->next;
    }
    return curr;
}

//puprose : child1 excecute the command and the parent wait for the child to finish
// child12 excecute the next command based on the result of the first child

//old methods of Lab 2
int getChildPID(cmdLine *pCmdLine){
    return atoi(pCmdLine->arguments[1]);
}

void addProcess(process** process_list, cmdLine* cmd, pid_t pid){
    process* newProcess = (process*)malloc(sizeof(process));
    if (newProcess == NULL){
        perror("Fail to allocate memory for new process");
        return;
    }
    newProcess->cmd = cmd;
    newProcess->pid = pid;
    newProcess->status = RUNNING;
    newProcess->next = *process_list;
    *process_list = newProcess;
    
}

void freeProcessList(process* process_list){
    process* curr = process_list;
    while (curr != NULL) {
        process* next = curr->next;
        freeCmdLines(curr->cmd);
        free(curr);
        curr = next;
    }
}

void removeProcess(process** process_list){
    process* prev = NULL;
    process* curr = *process_list;
    while (curr!=NULL){
    if (curr->status == TERMINATED) {
            process* temp = curr;
            if (prev == NULL) {
                *process_list = curr->next;
            } else {
                prev->next = curr->next;
            }
            curr = curr->next;
            freeCmdLines(temp->cmd);
            free(temp);
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
    

}


void updateProcessList(process **process_list) {
    process* curr = *process_list;
    while (curr != NULL) {
        int status;
        pid_t result = waitpid(curr->pid, &status, WNOHANG);
        if (result == -1) {
            perror("waitpid failed");
        } else if (result > 0) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                curr->status = TERMINATED;
            }
        }
        curr = curr->next;
    }
}

void updateProcessStatus(process* process_list, int pid, int status){
    process* curr = process_list;
    while (curr!=NULL){
        if (curr->pid == pid){
            curr->status = status;
            break;
        }
        curr = curr->next;
    }
}


void printProcessList(process** process_list) {
    updateProcessList(process_list);
    process* curr = *process_list;
    while (curr != NULL) {
        printf("PID: %d, Command: %s\n", curr->pid, curr->cmd->arguments[0]);
        if (curr->status == RUNNING) {
            printf("Status: Running\n");
        } else if (curr->status == SUSPENDED) {
            printf("Status: Suspended\n");
        } else if (curr->status == TERMINATED) {
            printf("Status: Terminated\n");
        }
        curr = curr->next;
    }
}

void stop(int processID){
    if (kill(processID, SIGSTOP) == -1){
        perror("Fail to stop process");
    }
    else {
        fprintf(stdout, "Process %d was stopped\n", processID);
    }
}

void term(int processID){
    if (kill(processID, SIGTERM) == -1){
        perror("Fail to terminate process");
    }
    else {
        fprintf(stdout, "Process %d was terminated\n", processID);
    }

}

void wake(int processID){

    if (kill(processID, SIGCONT) == -1){
        perror("Fail to wake process");
    }
    else {
        fprintf(stdout, "Process %d was woken\n", processID);
    }
} 

void handleCD(cmdLine *pCmdLine){
    if (pCmdLine->argCount < 2){
        perror("Fail to change directory");
        return;
    }
    if (chdir(pCmdLine->arguments[1]) == -1){
        perror("Fail to change directory");
    }
    else {
        fprintf(stdout, "Directory was changed\n");
    }
     freeCmdLines(pCmdLine);

}

//using pipe we made a connection between STDOUT and STDIN
void execute(cmdLine *pCmdLine){
    cmdLine *parsedLine;
    addProcess(&processList, pCmdLine, getpid());

    if (strcmp(pCmdLine->arguments[0], "history") == 0){
        printHistory(&historyList);
        return;

    }

    if (strcmp(pCmdLine->arguments[0], "!!") == 0){
        int index = 9 - HISTLEN;
        printf("Index: %d\n", index);
        historyLinkStruct* linkToProcess = getHistory(&historyList, index);
        char* cmdToParse = linkToProcess->cmd;
        parsedLine = parseCmdLines(cmdToParse);
        execute(parsedLine);
        return;        
        
    }
    
    if (pCmdLine->arguments[0][0] == '!' && isDigit(pCmdLine->arguments[0][1])){
        int index = atoi(&pCmdLine->arguments[0][1]);
        historyLinkStruct* linkToProcess = getHistory(&historyList, index);
        char* cmdToParse = linkToProcess->cmd;
        parsedLine = parseCmdLines(cmdToParse);
        execute(parsedLine);
        freeCmdLines(parsedLine);
        return;
    }

    // terms
     if (strcmp(pCmdLine->arguments[0], "procs") == 0){
        printf("Printing process list\n");
        printProcessList(&processList);
        return;
    }

        else if (strcmp (pCmdLine->arguments[0], "stop") == 0){
            if (pCmdLine->argCount < 2){
                perror("Fail to stop process");
            }
            else {
                stop(getChildPID(pCmdLine));
            }
        }

         else if (strcmp (pCmdLine->arguments[0],"wake") == 0){
            if (pCmdLine->argCount < 2){
                perror("Fail to wake process");
            }
            else {
                wake(getChildPID(pCmdLine));
            }
        }

        else if (strcmp (pCmdLine->arguments[0],"term") == 0){
            if (pCmdLine->argCount < 2){
                perror("Fail to terminate process");
            }
            else {
                term(getChildPID(pCmdLine));
            }
        }

        else if (strcmp (pCmdLine->arguments[0],"cd") == 0){
            handleCD(pCmdLine);           
        }




         
    //end of terms
        else{
            int PID = fork();
            if (PID == -1){
            perror("Frok failed");
            freeCmdLines(pCmdLine);
            _exit(1);
            }

            else if (PID == 0){ //child process
            //adding files:
            if (pCmdLine->inputRedirect != NULL){
                printf("Input file: %s\n", pCmdLine->inputRedirect);
                FILE *inputFile = fopen(pCmdLine->inputRedirect, "r");
                if (inputFile == NULL){
                    perror("Fail to open input file");
                    _exit(1);
                }
                dup2(fileno(inputFile), STDIN_FILENO); //to read from the file
                fclose(inputFile);
            }

            if (pCmdLine->outputRedirect != NULL){
                printf("Output file: %s\n", pCmdLine->outputRedirect);
                FILE *outputFile = fopen(pCmdLine->outputRedirect, "w");
                if (outputFile == NULL){
                    perror("Fail to open output file");
                    _exit(1);
                }
                dup2(fileno(outputFile), STDOUT_FILENO); //to write to the file
                fclose(outputFile);
            }

            if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1){ //trying to run
            perror("Error in execvp");
            freeCmdLines(pCmdLine);
            _exit(1); //to 'kill' the child :(
            }
        }
            else{ //I am the father - getting back my child ID
                if (pCmdLine->blocking ==1) {
                int status = 0;
                waitpid(PID, &status, 0); //waiting for the childPID to finish
                }

            }
        }

    //recieves a parsed line
    
} 
    

void execute_pipe(cmdLine *pCmdLine) { 
    cmdLine *parsedLine;
    addProcess(&processList, pCmdLine, getpid());
     if (strcmp(pCmdLine->arguments[0], "history") == 0){
        printHistory(&historyList);
        return;
    }

       if (strcmp(pCmdLine->arguments[0], "!!") == 0){
        int index = 9 - HISTLEN;
        printf("Index: %d\n", index);
        historyLinkStruct* linkToProcess = getHistory(&historyList, index);
        char* cmdToParse = linkToProcess->cmd;
        parsedLine = parseCmdLines(cmdToParse);
        execute(parsedLine);
        freeCmdLines(parsedLine);
        return;        
        
    }
    
    if (pCmdLine->arguments[0][0] == '!' && isDigit(pCmdLine->arguments[0][1])){
        int index = atoi(&pCmdLine->arguments[0][1]);
        historyLinkStruct* linkToProcess = getHistory(&historyList, index);
        char* cmdToParse = linkToProcess->cmd;
        parsedLine = parseCmdLines(cmdToParse);
        execute(parsedLine);
        freeCmdLines(parsedLine);
        return;
    }


    if (strcmp(pCmdLine->arguments[0], "procs") == 0){
        printProcessList(&processList);
        return;
    }

      if (strcmp(pCmdLine->arguments[0], "stop") == 0){
          if (pCmdLine->argCount < 2){
                perror("Fail to terminate process");
            }
            else {
                stop(getChildPID(pCmdLine));
            }
    }

      if (strcmp(pCmdLine->arguments[0], "wake") == 0){
          if (pCmdLine->argCount < 2){
                perror("Fail to terminate process");
            }
            else {
                wake(getChildPID(pCmdLine));
            }
    }

      if (strcmp(pCmdLine->arguments[0], "term") == 0){
          if (pCmdLine->argCount < 2){
                perror("Fail to terminate process");
            }
            else {
                term(getChildPID(pCmdLine));
            }
        return;
    }

    if (strcmp(pCmdLine->arguments[0], "cd") == 0){
        handleCD(pCmdLine);
        return;
    }   

    if (pCmdLine->outputRedirect != NULL || (pCmdLine->next!= NULL && pCmdLine->next->inputRedirect != NULL)) {
        fprintf(stderr, "Error: Invalid redirection with pipe\n");
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {  
        perror("Pipe failed");   
        return;
    }

    pid_t child1 = fork();
    if (child1 == -1) {
        perror("Fork failed");
        return;
    }

    if (child1 == 0) { // First child
        close(pipefd[0]); // Close read-end of the pipe
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the pipe's write-end
        close(pipefd[1]);


        if (pCmdLine->inputRedirect != NULL){
                printf("Input file: %s\n", pCmdLine->inputRedirect);
                FILE *inputFile = fopen(pCmdLine->inputRedirect, "r");
                if (inputFile == NULL){
                    perror("Fail to open input file");
                    _exit(1);
                }
                dup2(fileno(inputFile), STDIN_FILENO); //to read from the file
                fclose(inputFile);
            }




        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {
            perror("Fail to execute first command");
            _exit(1);
        }
    }

    pid_t child2 = fork();
    if (child2 == -1) {
        perror("Fork failed");
        return;
    }


////////////////-----------------------------Second child-----------------------------////////////////
    if (child2 == 0) { 
        close(pipefd[1]); // Close write-end of the pipe
        dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to the pipe's read-end
        close(pipefd[0]);

         if (pCmdLine->outputRedirect != NULL){
                printf("Output file: %s\n", pCmdLine->outputRedirect);
                FILE *outputFile = fopen(pCmdLine->outputRedirect, "w");
                if (outputFile == NULL){
                    perror("Fail to open output file");
                    _exit(1);
                }
                dup2(fileno(outputFile), STDOUT_FILENO); //to write to the file
                fclose(outputFile);
            }



        if (execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments) == -1) {
            perror("Fail to execute second command");
            _exit(1);
        }
    }

    close(pipefd[0]); // Close both ends of the pipe in the parent process
    close(pipefd[1]);

    // Wait for both children to finish
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);
}



int main(int argc, char **argv){

    cmdLine *parsedLine;

    if (argc>1 && strcmp(argv[1], "-d") == 0){
        debug = true;
    }
    

    while (1){ //for infinte loop

        if (debug){
            fprintf(stderr, "PID: %d\n", getpid());
            fprintf(stderr, "Executing command: %s", input);
        }

        char current_dir[MAX_PATH_SIZE]; 
        if (getcwd(current_dir, MAX_PATH_SIZE) == NULL){
            perror("Fail in getcwd");
            _exit(1);
        }

    

        fprintf(stdout, "%s>", current_dir); //print the current directory using prompt symbol

        fgets(input, MAX_LENGTH, stdin); //reading from stdin to line - max length is 2048


        if (strcmp (input,  "quit\n") == 0 ){
            freeProcessList(processList);
            freeHistory(&historyList);
            exit(0);
        }
        
        insertToHistory(&historyList,input);

        parsedLine = parseCmdLines(input); //cmd structure
        if (parsedLine == NULL){
            continue;
        }
        if (parsedLine->next == NULL){
            execute(parsedLine);
        }
        else{

        execute_pipe(parsedLine); 
    }
    freeCmdLines(parsedLine); 

}
}