/*
* CSCI3150 Introduction to Operating Systems
*
* --- Declaration ---
* For all submitted files, including the source code files and the written  
* report, for this assignment:
*
* I declare that the assignment here submitted is original except for source
* materials explicitly acknowledged. I also acknowledge that I am aware of
* University policy and regulations on honesty in academic work, and of the
* disciplinary guidelines and procedures applicable to breaches of such policy
* and regulations, as contained in the website
* http://www.cuhk.edu.hk/policy/academichonesty/
*
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define MAX_ARG_NUM  30

int shell_execute(char ** args, int argc)
{
	int child_pid, wait_return, status;

	if(strcmp(args[0], "EXIT") == 0)
		return -1; 

	/* find the starting positions of each command in args */
	int cmd_start[4] = {0, argc-1, argc-1, argc-1};
	for(int i=0, p=1; i<argc-1; ++i)
		if(strcmp(args[i], "|") == 0)
			cmd_start[p++] = i + 1;

	/* retrieve the four commands from args */
	char **cmd1 = (char**)malloc(MAX_ARG_NUM * sizeof(char*)),
		 **cmd2 = (char**)malloc(MAX_ARG_NUM * sizeof(char*)),
		 **cmd3 = (char**)malloc(MAX_ARG_NUM * sizeof(char*)),
		 **cmd4 = (char**)malloc(MAX_ARG_NUM * sizeof(char*));
	
	if (cmd1==NULL || cmd2==NULL || cmd3==NULL || cmd4==NULL) {
		printf("malloc() error for cmd1 cmd2 cmd3 cmd4\n");
		exit(-1);
	}

	int p1 = 0,  // counting the arg num of each command
		p2 = 0,
		p3 = 0,
		p4 = 0;

	for(int i=0; i<argc-1; ++i) { // copy the refrence of arguments from args to cmd1 2 3 4
		if(strcmp(args[i], "|") == 0) continue;		
		else if(i < cmd_start[1]) cmd1[p1++] = args[i];
		else if(i < cmd_start[2]) cmd2[p2++] = args[i];
		else if(i < cmd_start[3]) cmd3[p3++] = args[i];
		else                      cmd4[p4++] = args[i];
	}

	/* initialise the pipe */
	int pipe1[2], pipe2[2], pipe3[2];
    if(pipe(pipe1)==-1 || pipe(pipe2)==-1 || pipe(pipe3)==-1) {
        printf("pipe() error \n");
        exit(1);
    }

	/* execute the commands */
	// executing command 1
	if((child_pid = fork()) < 0) {
		printf("fork() error \n");
	} else if(child_pid == 0) {
		// Child Process
		// redirect stdout to pipe if next command exists
		if(p2 > 0) {
			close(STDOUT_FILENO);
			dup(pipe1[1]);
		}
		// close pipes
		for(int i=0; i<2; ++i) {
			close(pipe1[i]);
            close(pipe2[i]);
            close(pipe3[i]);
        }
		// execute command 1
		if(execvp(cmd1[0], cmd1) < 0) { 
			printf("execvp() error \n");
			exit(-1);
		}
	}

	// executing command 2
	if(p2 > 0) { // check if command 2 exists
		if((child_pid = fork()) < 0) {
			printf("fork() error \n");
		} else if(child_pid == 0) {
			// Child Process
			// redirect stdout to pipe if next command exists
			if(p3 > 0) {
				close(STDOUT_FILENO);
				dup(pipe2[1]);
			}
			// redirect stdin to pipe
			close(STDIN_FILENO);
			dup(pipe1[0]);
			// close pipes
			for(int i=0; i<2; ++i) {
                close(pipe1[i]);
                close(pipe2[i]);
                close(pipe3[i]);
            }
			// execute command 2
			if(execvp(cmd2[0], cmd2) < 0) { 
				printf("execvp() error \n");
				exit(-1);
			}
		}
	}

	// executing command 3
	if(p3 > 0) { // check if command 3 exists
		if((child_pid = fork()) < 0) {
			printf("fork() error \n");
		} else if(child_pid == 0) {
			// Child Process
			// redirect stdout to pipe if next command exists
			if(p4 > 0) {
				close(STDOUT_FILENO);
				dup(pipe3[1]);
			}
			// redirect stdin to pipe
			close(STDIN_FILENO);
			dup(pipe2[0]);
			// close pipess
			for(int i=0; i<2; ++i) {
                close(pipe1[i]);
                close(pipe2[i]);
                close(pipe3[i]);
            }
			// execute command 3
			if(execvp(cmd3[0], cmd3) < 0) { 
				printf("execvp() error \n");
				exit(-1);
			}
		}
	}

	// executing command 4
	if(p4 > 0) { // check if command 4 exists
		if((child_pid = fork()) < 0) {
			printf("fork() error \n");
		} else if(child_pid == 0) {
			// Child Process
			// redirect stdin to pipe
			close(STDIN_FILENO);
			dup(pipe3[0]);
			// close pipes
			for(int i=0; i<2; ++i) {
                close(pipe1[i]);
                close(pipe2[i]);
                close(pipe3[i]);
            }
			// execute command 4
			if(execvp(cmd4[0], cmd4) < 0) { 
				printf("execvp() error \n");
				exit(-1);
			}
		}
	}

	// Parent Precess
	// close pipes
	for(int i=0; i<2; ++i) {
		close(pipe1[i]);
		close(pipe2[i]);
		close(pipe3[i]);
	}

	// wait all children to finish
	while((wait_return = wait(&status)) > 0);

	// delete useless variables
	free(cmd1);
	free(cmd2);
	free(cmd3);
	free(cmd4);

	return 0;

}
