#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>
#include <string.h>

void handler(int sig) //sig is the signal number recived from user
{
	printf("\nRecieved Signal : %s\n", strsignal(sig));
	if (sig == SIGTSTP) //if asking to stop the program
	{
		signal(SIGTSTP, SIG_DFL); //update the signal to default
	}
	else if (sig == SIGCONT)
	{
		signal(SIGCONT, SIG_DFL); 
	}
	signal(sig, SIG_DFL); 
	raise(sig);//resend the signal - like return
}

int main(int argc, char **argv)
{

	printf("Starting the program\n");
	signal(SIGINT, handler); //when pressing ctrl+c
	signal(SIGTSTP, handler);
	signal(SIGCONT, handler);

	while (1)
	{
		sleep(1);
	}

	return 0;
}