//from lab 2

/*
Having learned how to create a pipe between 2 processes/programs in Part 1, we now wish to implement 
a pipeline inside our own shell. In this part you will extend your shell's capabilities to support
pipelines that consist of just one pipe and 2 child processes. That is, support a command 
line with one pipe between 2 processes resulting from running executable files mentioned in the command line.
The scheme uses basically the same mechanism as in part 1, except that now the program to be executed in each 
child process is determined by the command line.

Your shell must be able now to run commands like: ls|wc -l which basically counts the number of 
files/directories under the current working dir. The most important thing to remember about pipes 
is that the write-end of the pipe needs to be closed in all processes, otherwise the read-end of the pipe will 
not receive EOF, unless the main process terminates.
Notes:

The line parser automatically generates a list of cmdLine structures to accommodate pipelines. 
For instance, when parsing the command "ls | grep .c", two chained cmdLine structures are created, 
representing ls and grep respectively.
Your shell must still support all previous features, 
including input/output redirection from lab 2. 
Obviously, it makes no sense to redirect the output of the left--hand-side process (as then nothing goes into the pipe), 
and this should be considered an error, and likewise redirecting the input of the right-hand-side process is an error 
(as then the pipe output is hanging). In such cases, print an error message to stderr without generating any new processes.
It is important to note that commands utilizing both I/O redirection and pipelines are indeed quite common
(e.g. "cat < in.txt | tail -n 2 > out.txt").
As in previous tasks, you must keep your program free of memory leaks.
*/

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
        execvp(cmdLine->arguments[0], cmdLine->arguments); // Execute the command
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
            execvp(cmdLine->arguments[0], cmdLine->arguments); // Execute the command   
            perror("execlp failed");        // Handle execlp failure
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