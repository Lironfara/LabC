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

process* process_list = NULL;
historyLinkStruct* historyLink = NULL;
history* historyList = NULL;


bool isFull(){
    return HISTLEN == 0;
}

void cleanFirst(history** history_List){
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
}

void printHistory(history** history_List){
    history* historyList = *history_List;
    if(historyList == NULL){
        perror("No history");
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
    printf("Index: %d\n", index);  
    printf("9-HISTLEN: %d\n", 9-HISTLEN); 
    if (index > (9 - HISTLEN)){
        fprintf(stderr ,"Index out of range\n");
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


//void addProcess(process** process_list, cmdLine* cmd, pid_t pid)
void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    // Allocate memory for the new process
    process* newProcess = (process*)malloc(sizeof(process));
    if (newProcess == NULL) {
        perror("Failed to allocate memory for new process");
        return;
    }

    if (cmd == NULL) {
        perror("cmd is NULL");
        free(newProcess); // Free allocated memory to prevent leaks
        return;
    }

    // Initialize the new process
    newProcess->cmd = cmd;
    newProcess->pid = pid;
    newProcess->status = RUNNING;
    newProcess->next = NULL; // Since it's a new node, next should initially be NULL

    // Add the new process to the list
    if (*process_list == NULL) {
        *process_list = newProcess;
    } else {
        process* curr = *process_list;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = newProcess; // Append the new process at the end
    }
}


//void freeProcessList(process* process_list)
void freeProcessList(process* process_list){
    process* curr = process_list;
    while (curr != NULL){
        process* next = curr->next;
        freeCmdLines(curr->cmd);
        free(curr);
        curr = next;
    }
}

void removeDeadProcesses(process** process_list){
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

//void updateProcessList(process **process_list)
void updateProcessList(process **process_list) {
    process* curr = *process_list;
    while (curr != NULL) {
        int status;
        pid_t result = waitpid(curr->pid, &status, WNOHANG);
        if (result == 0) {
            curr->status = RUNNING;
        } 
        else if (result == -1) {
            perror("waitpid");
        } 
        else {
            printf("Process %d has terminated\n", curr->pid);
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                curr->status = TERMINATED;
            }
            else if (WIFSTOPPED(status)) {
                curr->status = SUSPENDED;
            }
            else if (WIFCONTINUED(status)) {
                curr->status = RUNNING;
            }
        }
        curr = curr->next;
    }
}

//void updateProcessStatus(process* process_list, int pid, int status)
void updateProcessStatus(process* process_list, int pid, int status){
    printf("%d\n", pid);
    process* curr = process_list;
    while (curr!=NULL){
        if (curr->pid == pid){
            curr->status = status;
            break;
        }
        curr = curr->next;
    }
}

void printArguments(cmdLine *pCmdLine)
{
    for (int i = 0; i < pCmdLine->argCount; ++i)
        printf("%s ", pCmdLine->arguments[i]);

    printf("\n");
}

//void printProcessList(process** process_list)
//Run updateProcessList() at the beginning of the function.
//If a process was "freshly" terminated, delete it after printing it 
//(meaning print the list with the updated status, then delete the dead processes)
void printProcessList(process** process_list) {
    updateProcessList(process_list);
    printf("Index\t\tPID\t\t\tSTATUS\t\t\tCommand\n");
    process* curr = *process_list;
    int index = 1;
    while (curr != NULL) {
        if (curr->cmd == NULL) {
            fprintf(stderr, "Error: curr->cmd is NULL\n");
            curr = curr->next;
            return;
        }   
        char* status;
        if (curr->status == RUNNING) {
            status = "RUNNING";
        } else if (curr->status == SUSPENDED) {
            status = "SUSPENDED";
        }
        else if (curr->status == TERMINATED) {
            status = "TERMINATED";
        }

        printf("%d\t\t%d\t\t\t%s\t\t\t", index,curr->pid, status);
        printArguments(curr->cmd);
        index++;
        curr = curr->next;
    }
    removeDeadProcesses(process_list);
}

void stop(int processID){
    if (kill(processID, SIGSTOP) == -1){
        perror("Fail to stop process");
    }
    else {
        fprintf(stdout, "Process %d was stopped\n", processID);
        updateProcessStatus(process_list, processID, SUSPENDED);
    }
}

void term(int processID){
    if (kill(processID, SIGTERM) == -1){
        perror("Fail to terminate process");
    }
    else {
        fprintf(stdout, "Process %d was terminated\n", processID);
        printf("Updating process status\n");
        updateProcessStatus(process_list, processID, TERMINATED);
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
    if (strcmp(pCmdLine->arguments[0], "history") == 0){
        printHistory(&historyList);
        freeCmdLines(pCmdLine);
        return;

    }

    if (strcmp(pCmdLine->arguments[0], "!!") == 0){
        int index = 9 - HISTLEN;
        printf("Index: %d\n", index);
        historyLinkStruct* linkToProcess = getHistory(&historyList, index);
        if (linkToProcess == NULL){
            fprintf(stderr,"Fail to get history\n");
            freeCmdLines(pCmdLine);
            return;
        }
        char* cmdToParse = linkToProcess->cmd;
        parsedLine = parseCmdLines(cmdToParse);
        freeCmdLines(pCmdLine);
        execute(parsedLine);
        return;        
        
    }
    
    
    if (pCmdLine->arguments[0][0] == '!' && isDigit(pCmdLine->arguments[0][1])){
        int index = atoi(&pCmdLine->arguments[0][1]);
        printf("Index is : %d", index);
        historyLinkStruct* linkToProcess = getHistory(&historyList, index);
        if (linkToProcess == NULL){
            fprintf(stderr, "Fail to get history\n");
            freeCmdLines(pCmdLine);
            return;
        }   
        char* cmdToParse = linkToProcess->cmd;
        printf("Command to parse: %s\n", cmdToParse);
        freeCmdLines(pCmdLine);
        parsedLine = parseCmdLines(cmdToParse);
        execute(parsedLine);
        return;
    }

    // terms
     if (strcmp(pCmdLine->arguments[0], "procs") == 0){
        printProcessList(&process_list);
        freeCmdLines(pCmdLine);
        return;
    }

        else if (strcmp (pCmdLine->arguments[0], "stop") == 0){
            if (pCmdLine->argCount < 2){
                perror("Fail to stop process");
            }
            else {
                stop(getChildPID(pCmdLine));
            }
            freeCmdLines(pCmdLine);
            return;
        }

         else if (strcmp (pCmdLine->arguments[0],"wake") == 0){
            if (pCmdLine->argCount < 2){
                perror("Fail to wake process");
            }
            else {
                wake(getChildPID(pCmdLine));
            }
            freeCmdLines(pCmdLine);
            return;
        }

        else if (strcmp (pCmdLine->arguments[0],"term") == 0){
            if (pCmdLine->argCount < 2){
                perror("Fail to terminate process");
            }
            else {
                term(getChildPID(pCmdLine));
            }
            freeCmdLines(pCmdLine);
            return;
        }

        else if (strcmp (pCmdLine->arguments[0],"cd") == 0){
            handleCD(pCmdLine);
            return;

        }




         
    //end of terms
        else{
            int PID = fork();
            addProcess(&process_list, pCmdLine, PID);
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
     if (strcmp(pCmdLine->arguments[0], "history") == 0){
        printHistory(&historyList);
        freeCmdLines(pCmdLine);
        return;
    }

       if (strcmp(pCmdLine->arguments[0], "!!") == 0){
        int index = 9 - HISTLEN;
        freeCmdLines(pCmdLine);
        historyLinkStruct* linkToProcess = getHistory(&historyList, index);
        if (linkToProcess == NULL){
            fprintf(stderr, "Fail to get history\n");
            freeCmdLines(pCmdLine);
            return;
        }   
        char* cmdToParse = linkToProcess->cmd;
        parsedLine = parseCmdLines(cmdToParse);
        execute(parsedLine);
        return;        
        
    }
    
    if (pCmdLine->arguments[0][0] == '!' && isDigit(pCmdLine->arguments[0][1])){
        int index = atoi(&pCmdLine->arguments[0][1]);
        printf("Index: %d\n", index);
        freeCmdLines(pCmdLine);
        historyLinkStruct* linkToProcess = getHistory(&historyList, index);
        if (linkToProcess == NULL){
            fprintf(stderr, "Fail to get history\n");
            freeCmdLines(pCmdLine);
            return;
        }
        char* cmdToParse = linkToProcess->cmd;
        parsedLine = parseCmdLines(cmdToParse);
        freeCmdLines(pCmdLine);
        execute(parsedLine);
        return;
    }


    if (strcmp(pCmdLine->arguments[0], "procs") == 0){
        freeCmdLines(pCmdLine);
        printProcessList(&process_list);
        return;
    }

      if (strcmp(pCmdLine->arguments[0], "stop") == 0){
          if (pCmdLine->argCount < 2){
                perror("Fail to terminate process");
            }
            else {
                stop(getChildPID(pCmdLine));
            }
            freeCmdLines(pCmdLine);
            return;
    }

      if (strcmp(pCmdLine->arguments[0], "wake") == 0){
          if (pCmdLine->argCount < 2){
                perror("Fail to terminate process");
            }
            else {
                wake(getChildPID(pCmdLine));
            }
            freeCmdLines(pCmdLine);
            return;
    }

      if (strcmp(pCmdLine->arguments[0], "term") == 0){
          if (pCmdLine->argCount < 2){
                perror("Fail to terminate process");
            }
            else {
                term(getChildPID(pCmdLine));
            }
            freeCmdLines(pCmdLine);
        return;
    }

    if (strcmp(pCmdLine->arguments[0], "cd") == 0){
        handleCD(pCmdLine);
        return;
    }   

    if (pCmdLine->outputRedirect != NULL || (pCmdLine->next!= NULL && pCmdLine->next->inputRedirect != NULL)) {
        fprintf(stderr, "Error: Invalid redirection with pipe\n");
        freeCmdLines(pCmdLine);
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {  
        perror("Pipe failed");   
        return;
    }

    pid_t child1 = fork();
     addProcess(&process_list, pCmdLine, child1);
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
        addProcess(&process_list, pCmdLine->next, child2);
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
        printf("Debug mode is on\n");
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
            freeProcessList(process_list);
            freeHistory(&historyList);
            break;
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

}
}