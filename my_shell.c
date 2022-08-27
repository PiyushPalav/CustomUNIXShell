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


int main(int argc, char* argv[]) {
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;

	pid_t child_PID, w;
	const char *homedir;
    int cd_status,status;

	while(1) {			
		bzero(line, sizeof(line));
		//memset(line, 0, sizeof(line));
		printf("$ ");
		scanf("%[^\n]", line);
		getchar();

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);

		// empty command should display the prompt again
		if (tokens[0]==NULL)
			continue;
		else if (strcmp(tokens[0],"exit") == 0){
			break;
		}
		
		// fetch the current user's home directory
		if ((homedir = getenv("HOME")) == NULL) 
			homedir = getpwuid(getuid())->pw_dir;

		//check if the command is cd and implement it using chdir system call
		if (strcmp(tokens[0],"cd") == 0){
			if (tokens[1]==NULL || (strcmp(tokens[1],"~") == 0))
				cd_status = chdir(homedir); // change to home directory on just cd command or cd ~
			else if (tokens[1]!=NULL && tokens[2]==NULL)
				cd_status = chdir(tokens[1]); // change the current working directory of the shell process to the directory specified in path
			else
				printf("Shell:Incorrect command\n");

			if (cd_status == -1)
				perror("cd command failed");
				
			continue;	
	   	}
			child_PID = fork();

			if (child_PID == -1){
				fprintf(stderr, "%s\n", "Unable to create child process!\n");
				perror("fork");
				exit(EXIT_FAILURE);
			}
			else if (child_PID == 0){
				execvp(tokens[0], tokens);
				//printf("Binary for command %s does not exist\n", tokens[0]);
				perror("Exec system call failed!");
				_exit(EXIT_SUCCESS);
			}else{
				do {
					w = waitpid(child_PID, &status, WUNTRACED | WCONTINUED);
					if (w == -1) {
						perror("waitpid");
						exit(EXIT_FAILURE);
					}
				} while (!WIFEXITED(status) && !WIFSIGNALED(status));
				//printf("Child process with pid %ld reaped successfully\n",(long) w);
			}

		// Freeing the allocated memory	
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);

	}
	return 0;
}
