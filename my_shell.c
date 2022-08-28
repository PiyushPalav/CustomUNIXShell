#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>
#include <pwd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

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

void freeAllocatedMemoryToTokens(char **tokens){
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

// void executeCmdInBackground(){

// }

int main(int argc, char* argv[]) {
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;

	pid_t foreground_child_PID, foreground_child_wait;
    int foreground_process_status;
	int num_args;

	int background_processes_count=0;

	while(1) {			
		bzero(line, sizeof(line));
		//memset(line, 0, sizeof(line));
		printf("$ ");
		scanf("%[^\n]", line);
		getchar();

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);

		num_args = getNumberOfArgs(tokens);

		if (num_args > 0 && (strcmp(tokens[num_args],"&") == 0)){
			//printf("This command needs to be executed in background\n");
			background_processes_count++;
			if (background_processes_count <= 64)
				executeCmdInBackground(tokens, num_args);
			else
				printf("Limit of 64 background processes reached!. Please wait for existing background processes to finish\n");
			//printf("Background processes count : %d\n",background_processes_count);
			freeAllocatedMemoryToTokens(tokens);
			continue;
		}

		// empty command should display the prompt again
		if (tokens[0]==NULL){
			freeAllocatedMemoryToTokens(tokens);
			continue;
		}
		else if (strcmp(tokens[0],"exit") == 0){
			if (num_args > 0){
				printf("Too many arguments to exit command\n");
				freeAllocatedMemoryToTokens(tokens);
				continue;
			}
			else
				break; // break out of the infinite loop for exit command
		}

		//check if the command is cd and implement it using chdir system call
		if (strcmp(tokens[0],"cd") == 0){
			changeDir(tokens,num_args);
			freeAllocatedMemoryToTokens(tokens);
			continue;
	   	}

		foreground_child_PID = fork();

		if (foreground_child_PID == -1){
			fprintf(stderr, "%s\n", "Unable to create child process!\n");
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else if (foreground_child_PID == 0){
			execvp(tokens[0], tokens);
			perror("Exec system call failed!");
			_exit(EXIT_FAILURE);
		}else{
			do {
				foreground_child_wait = waitpid(foreground_child_PID, &foreground_process_status, WUNTRACED | WCONTINUED);
				if (foreground_child_wait == -1) {
					perror("waitpid");
					exit(EXIT_FAILURE);
				}
			} while (!WIFEXITED(foreground_process_status) && !WIFSIGNALED(foreground_process_status));
			//printf("Child process with pid %ld reaped successfully\n",(long) foreground_child_wait);
		}

		// Freeing the allocated memory	
		freeAllocatedMemoryToTokens(tokens);

	}
	return 0;
}
