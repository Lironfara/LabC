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



#define MAX_LENGTH 2048
#define MAX_PATH_SIZE 4096
#define TERMINATED  -1
#define RUNNING 1
#define SUSPENDED 0
char input[MAX_LENGTH]; //string is array of chars
bool debug = false;

typedef struct process{
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid; 		                  /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	                  /* next process in chain */
} process;

process* processList = NULL;


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

void stop(cmdLine *pCmdLine){
    if (pCmdLine->argCount < 2){
        perror("Fail to stop process");
        return;
    }
    int childPID = getChildPID(pCmdLine);
    if (kill(childPID, SIGSTOP) == -1){
        perror("Fail to stop process");
    }
    else {
        fprintf(stdout, "Process %d was stopped\n", childPID);
    }
}

void term(cmdLine *pCmdLine){
    if (pCmdLine->argCount < 2){
        perror("Fail to terminate process");
        return;
    }
    int childPID = getChildPID(pCmdLine);
    if (kill(childPID, SIGTERM) == -1){
        perror("Fail to terminate process");
    }
    else {
        fprintf(stdout, "Process %d was terminated\n", childPID);
    }

}

void wake(cmdLine *pCmdLine){
    if (pCmdLine->argCount < 2){
        perror("Fail to wake process");
        return;
    }
    int childPID = getChildPID(pCmdLine);
    if (kill(childPID, SIGCONT) == -1){
        perror("Fail to wake process");
    }
    else {
        fprintf(stdout, "Process %d was woken\n", childPID);
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

    // terms
        if (strcmp (pCmdLine->arguments[0], "stop") == 0){
            handleStop(pCmdLine);
        }

         else if (strcmp (pCmdLine->arguments[0],"wake") == 0){
            handleWake(pCmdLine);
        }

        else if (strcmp (pCmdLine->arguments[0],"term") == 0){
            handleTerm(pCmdLine);
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
    if (strcmp(pCmdLine->arguments[0], "procs") == 0){
        printProcessList(&processList);
        return;
    }

      if (strcmp(pCmdLine->arguments[0], "stop") == 0){
        handleStop(pCmdLine);   
        return;
    }

      if (strcmp(pCmdLine->arguments[0], "wake") == 0){
        handleWake(pCmdLine);
        return;
    }

      if (strcmp(pCmdLine->arguments[0], "term") == 0){
        handleTerm(pCmdLine);
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


    else{
        addProcess(&processList, pCmdLine, getpid());
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


void procs(){

}

void wake(){

}

void stop(){

}

void term(){

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
            break;
        }
        
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