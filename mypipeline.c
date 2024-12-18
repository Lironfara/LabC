#include <stdio.h>   
#include <stdlib.h>   
#include <unistd.h>  
#include <string.h>
#include <sys/wait.h>  

#define MAX_LENGTH 2048

int main(){

    //create pipe
    int pipefd[2];  //pipe[0] is the read end and pipe[1] is the write end
    if (pipe(pipefd) ==-1){  //for read and write
    //pipefd[0] is the read and pipefd[1] is the write
        perror("Pipe failed");   
        exit(1);
    }

    //fork first child
    int PIDchild1 = fork();
    
    if (PIDchild1 == -1){
        perror("Fork failed");
        exit(1);
    }

    if (PIDchild1 == 0){
        //child1 process
        close(STDOUT_FILENO); //close standard output
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n");
        dup2(pipefd[1], STDOUT_FILENO); //duplicate the write end of the pipe
        close(pipefd[1]); //close the file descriptor that was duplicated
        fprintf(stderr, "(child1>going to execute cmd: ls -l)\n");
        execlp("ls", "ls", "-l", NULL); // Execute "ls -l" entering it to the duplicpipe we closed - child2 reading from the duplicate read
    }


    //parent process
    else{ //parent of child1
        fprintf(stderr, "parent_process>closing the write end of the pipe…\n");
        close(pipefd[1]);
        fprintf(stderr, "(parent_process>forking…)\n");
        int PIDchild2 = fork();
        fprintf(stderr, "(parent_process>created process with id: %d\n", PIDchild2);
        if (PIDchild2 == -1){
             perror("Fork failed");
            exit(1);
        }

        if (PIDchild2 == 0){ //child
            close(STDIN_FILENO); //close standard output
            fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n");
            dup2(pipefd[0], STDIN_FILENO); //duplicate the read end of the pipe
            close(pipefd[0]); //close the file descriptor that was duplicated
            fprintf(stderr, "(child2>going to execute cmd: tail -n 2)\n");
            execlp("tail", "tail", "-n", "2", NULL); // Execute "tail -n 2"            perror("execlp failed");        // Handle execlp failure
            exit(1);
        }
        else{ //parent of child2
            fprintf(stderr, "parent_process>closing the read end of the pipe…\n");
            close(pipefd[0]);
            int status;
            fprintf(stderr, "parent_process>waiting for child processes to terminate…\n");
            waitpid(PIDchild1, &status, 0); //by the order it was created
            waitpid(PIDchild2, &status, 0);
            fprintf(stderr, "parent_process>exiting…\n");
        } 

    }

}