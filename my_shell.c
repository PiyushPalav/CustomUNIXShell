#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>
#include <pwd.h>

#include <signal.h>
#include <errno.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

pid_t foreground_child_PID = 0;
pid_t background_child_PID = 0;
pid_t background_child_PID_arr[64];

// Assumption-single foreground process and multiple background processes upto 64

/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
		tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
		strcpy(tokens[tokenNo++], token);
		tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}

void freeAllocatedMemory(char **tokens){
	for(int i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
	free(tokens);
}

int getNumberOfArgs(char **tokens){
	int count = 0;
	for (int i = 1; tokens[i]!=NULL; i++){
		count++;
	}
	return count;
}

void changeDir(char **tokens,int num_args){
	const char *homedir;
	int cd_status;

	// fetch the current user's home directory
	if ((homedir = getenv("HOME")) == NULL) 
		homedir = getpwuid(getuid())->pw_dir;

	//implement cd using chdir system call
	if (tokens[1]==NULL || (strcmp(tokens[1],"~") == 0))
		cd_status = chdir(homedir); // change to home directory on just cd command or cd ~
	else if (tokens[1]!=NULL && num_args==1)
		cd_status = chdir(tokens[1]); // change the current working directory of the shell process to the directory specified in path
	else
		printf("Shell:Incorrect command\n");

	if (cd_status == -1)
		perror("cd command failed");
}

void signal_handler(int sig) {

  if(sig == SIGINT){
	if (foreground_child_PID > 0){
		int interrupt_status = kill(foreground_child_PID, SIGINT); // terminate foreground child process on Ctrl+C signal
		if (interrupt_status == -1){
			perror("Unable to terminate foreground child process");
		}
	}
	else{
		exit(0);
	}
  }
}

void init_signal_handler(){
	struct sigaction sa;
	sa.sa_handler = &signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGINT, &sa, 0) == -1) {
		perror("Unable to change signal action for sigint");
		exit(EXIT_SUCCESS);
	}
}

void reap_background_processes(){
	pid_t background_child_PID;
	while ((background_child_PID = waitpid((pid_t)(-1), 0, WNOHANG)) > 0) {
		printf("Shell: Background process with PID %ld finished\n",(long) background_child_PID);
	}
}

void executeCmdInBackground(char **tokens,int num_args,int background_processes_count){
	pid_t background_child_PGID, background_child_wait;
    int background_process_status;
	
	background_child_PID = fork();

	if (background_child_PID > 0){
		background_child_PID_arr[background_processes_count-1] = background_child_PID;
		//Move background child process in a separate process group than its parent
		background_child_PGID = setpgid(background_child_PID,background_child_PID);
		if (background_child_PGID == -1){
			perror("Unable to set process group ID");
			exit(EXIT_FAILURE);
		}
	}

	switch (background_child_PID) {
		case -1:
			fprintf(stderr, "%s\n", "Unable to create child process!\n");
			perror("fork");
			exit(EXIT_FAILURE);
		case 0:
			execvp(tokens[0], tokens);
			perror("Exec system call failed!");
			freeAllocatedMemory(tokens);
			_exit(EXIT_SUCCESS);
	}

	// Freeing the allocated memory	
	freeAllocatedMemory(tokens);
}

int main(int argc, char* argv[]) {
	char  line[MAX_INPUT_SIZE];
	char  **tokens;
	int i;

	pid_t foreground_child_PGID, foreground_child_wait;
    int foreground_process_status;
	int num_args;

	int background_processes_count=0;

	while(1) {			
		bzero(line, sizeof(line));
		printf("$ ");
		scanf("%[^\n]", line);
		getchar();

		reap_background_processes();

		init_signal_handler();

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);

		num_args = getNumberOfArgs(tokens);

		if (num_args > 0 && (strcmp(tokens[num_args],"&") == 0)){
			background_processes_count++;
			// remove & from tokens array
			free(tokens[num_args]);
			tokens[num_args] = NULL;

			if (background_processes_count <= 64)
				executeCmdInBackground(tokens, num_args, background_processes_count);
			else
				printf("Limit of 64 background processes reached!. Please wait for existing background processes to finish\n");
			continue;
		}

		// empty command should display the prompt again
		if (tokens[0]==NULL){
			freeAllocatedMemory(tokens);
			continue;
		}

		//implementation of exit command
		if (strcmp(tokens[0],"exit") == 0){
			if (num_args > 0){
				printf("Too many arguments to exit command\n");
				freeAllocatedMemory(tokens);
				continue;
			}
			else{
				freeAllocatedMemory(tokens);

				while (background_processes_count >= 1){
					background_processes_count--;
					int background_kill_status = kill(background_child_PID_arr[background_processes_count],SIGTERM); // kill background processes one by one
						if (background_kill_status == -1){
							perror("Unable to kill background process");
							exit(EXIT_FAILURE);
					}
				}
				break; // break out of the infinite loop for exit command
			}
		}

		//check if the command is cd and implement it using chdir system call
		if (strcmp(tokens[0],"cd") == 0){
			changeDir(tokens,num_args);
			freeAllocatedMemory(tokens);
			continue;
	   	}

		//execution of commands whose executables exist
		foreground_child_PID = fork();

		if (foreground_child_PID > 0){
			//Move foreground child process in a separate process group than its parent
			foreground_child_PGID = setpgid(foreground_child_PID,foreground_child_PID);
			if (foreground_child_PGID == -1){
				perror("Unable to set process group ID");
				exit(EXIT_FAILURE);
			}
		}

		switch (foreground_child_PID) {
			case -1:
				fprintf(stderr, "%s\n", "Unable to create child process!\n");
				perror("fork");
				exit(EXIT_FAILURE);
			case 0:
				execvp(tokens[0], tokens);
				perror("Exec system call failed!");
				freeAllocatedMemory(tokens);
				_exit(EXIT_SUCCESS);
			default:
				do {
					foreground_child_wait = waitpid(foreground_child_PID, &foreground_process_status, WUNTRACED | WCONTINUED);
					if (foreground_child_wait == -1) {
						perror("waitpid");
						exit(EXIT_FAILURE);
					}
				} while (!WIFEXITED(foreground_process_status) && !WIFSIGNALED(foreground_process_status));
		}

		// Freeing the allocated memory	
		freeAllocatedMemory(tokens);

	}
	return 0;
}
