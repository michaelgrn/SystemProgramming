/* Project: Mydash 
 * Author: Michael Green
 * Date: Fri 11 Mar 2022 06:42:32 PM MST
 *
 * The purpose of this project was to write a Bash clone. The functionality of
 * this project is as follows:
 * 
 * Fork and Exec
 * Signal Handling
 * FG and BG jobs
 * History
 *
 *
 */

#define _DEFAULT_SOURCE
#define _POSIX_SOURCE

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/history.h>
#include <readline/readline.h>

#define MAXARGS 2048
#define MAXLINE 4096

#include "common.h"
#include "mydash.h"
#include "list.h"
#include "log.h"
#include "version.h"

struct job{
	int id;
	char **cmd;
	int status;
	int jobNum;
};

char** chopLineExec(const char*);
void addAt(void *list, struct job *);
void evaluateCommand(char* commands, char* copy, void *list);
void evaluatePathway(char* commands);
void handleJobs(void *list);
void initializeUI(void);
void printJobs(void *list);
void printUsage(void);
void signalHandler(int signum);

struct job* fgJobs(void *list, int);
struct job* bgJobs(void *list, int);

int stop = 0;
int inter = 0;


/*
 * Main is designed to handle a few arguments, one of them being valgrind (which is
 * being left in as an artifact of the testing process) and the other is -v (which
 * prints the version of mydash); all other arguments will print out usage instructions.
 */

int main(int argc, char **argv)
{
	const char*  ver;
	ver = version();
	if(!(strcmp(argv[0],"valgrind")) || argc ==1){
		initializeUI();
		return 0;
	}else if(argc == 2){
		if(!strcmp(argv[1],"-v")){
			fprintf(stderr,"mydash: Version 1: Revision %s\n",ver);
			return 0;
		}else{
			printUsage();
			return 1;
		}
	}else{
		printUsage();
		return 1;
	}

}

/*
 * The purpose of this method is to intiliaze the UI and then further handle the user 
 * input presented to this UI. 
 */


void initializeUI(void)
{
	const char *envGet = getenv("DASH_PROMPT");
	const char *prompt;
	
    char *line;
	char *token;

	char copy[MAXLINE];
	char tokenize[MAXLINE];

	void *list = NULL;

	int numJobs = 0;

	unsigned int lineLength;

	signal(SIGINT, signalHandler);
	signal(SIGTSTP, signalHandler);
	signal(SIGTTOU, SIG_IGN);

    
    //checks to see if environment variable has been set

	if(envGet != NULL){
		prompt = getenv("DASH_PROMPT");
	}else{
		prompt = "mydash>";
	}

    //initialize history

	vector_alloc(&list);
	using_history();
	stifle_history(1000);

	while ((line=readline(prompt))){

		numJobs = vector_size(list);

		if(numJobs != 0){
			handleJobs(list);
		}

		lineLength = strlen(line);

		if((lineLength != 0) && (strspn(line," \t\n") !=lineLength))
			add_history(line);

		strcpy(copy, line);
		strcpy(tokenize, line);

		free(line);
		token = strtok(tokenize," ");

		evaluateCommand(token, copy, list);
	}
}

/*
 * Evaluates the input sent to this method.
 */
void evaluateCommand(char* commands, char* copy, void *list)
{
	struct job *j = NULL;
	struct job *k = NULL;
	int i =0, x=0, curNumJobs;
	pid_t this;
	int status;
	int numJobs;
	int searchFor;
	
	while(commands !=NULL){

		if((strcmp(commands,"exit")) == 0){

			curNumJobs = vector_size(list);

			for(i=0;i<curNumJobs;i++){

				j = (struct job*)vector_remove(list);

				while(j->cmd[x] != NULL){
					free(j->cmd[x]);
					x++;
				}

				free(j->cmd);
				kill(j->id,SIGKILL);
				free(j);
				x = 0;
			}

			vector_destroy(list);
			exit(0);

		}else if((strcmp(commands, "cd"))== 0){

			commands = strtok (NULL, " ");
			evaluatePathway(commands);
			commands = NULL;

		}else if((strcmp(commands, "jobs"))== 0){

			printJobs(list);
			commands = NULL;

		}else if((strcmp(commands, "history"))== 0){  

			HIST_ENTRY** history;
			history = history_list();

			while(history[x] != NULL){
				fprintf(stderr,"[%d] %s\n",x, history[x]->line);

				x++;
			}

			commands = NULL;

		}else if((strcmp(commands, "bg"))==0){

			curNumJobs = vector_size(list);

			if(curNumJobs > 0){

				commands = strtok (NULL, " ");

				if(commands != NULL){
					searchFor = atoi(commands);
				}else{
					searchFor = 0;
				}

				j = bgJobs(list,searchFor);
			}

			if(j != NULL){

				fprintf(stderr,"[%d] ",j->jobNum);

				while(j->cmd[i] != NULL){
					fprintf(stderr,"%s ",j->cmd[i]);
					i++;
				}

				fprintf(stderr,"&\n");
				kill(j->id, SIGCONT);
				waitpid(j->id,0,WNOHANG);
				j->status = 1;
				addAt(list,j);
			}

			commands = NULL;

		}else if((strcmp(commands, "fg"))==0){
			
			curNumJobs = vector_size(list);

			if(curNumJobs > 0){

			  	commands = strtok (NULL, " ");

				if(commands != NULL){
					searchFor = atoi(commands);
				}else{
					searchFor = 0;
				}

				j = fgJobs(list, searchFor);

			}

			if(j != NULL){

				this = tcgetpgrp(STDIN_FILENO);

				while(j->cmd[i] != NULL && strcmp(j->cmd[i],"&")!= 0){
					fprintf(stderr,"%s ",j->cmd[i]);
					i++;
				}

				fprintf(stderr,"\n");
				tcsetpgrp(STDIN_FILENO,j->id);
				waitpid(j->id,&status,WSTOPPED);
				tcsetpgrp(STDIN_FILENO, this);

				if(WIFSTOPPED(status) > 0){

					j->status = 0;
					addAt(list,j);
					i = 0;
					fprintf(stderr,"\n[%d]  Stopped\t\t",curNumJobs);

					while(j->cmd[i] != NULL){
						fprintf(stderr,"%s ",j->cmd[i]);
						i++;
					}

					fprintf(stderr,"\n");

				}else{

					while(j->cmd[x] != NULL){
						free(j->cmd[x]);
						x++;
					}
			
				free(j->cmd);
				free(j);
				  
				}

			}

			commands = NULL;

		}else{

			struct job *j = NULL;
			char* firstArg = malloc(MAXLINE);
			char** otherArgs;

			int pid;
			int i;
			int length;
			int numArgs;
			int amp;

			amp = 0;
			numArgs = 0;
			strncpy(firstArg, commands, MAXLINE);
			otherArgs = chopLineExec(copy);
			numJobs = vector_size(list);

			while(otherArgs[numArgs] != NULL){
				numArgs++;
			}

			if((otherArgs[1] == NULL)){

				length = strlen(otherArgs[0]);

				if(otherArgs[0][length-1]== '&'){
					amp =1;
					otherArgs[0][length-1]= '\0';
					firstArg[length-1]='\0';
				}

			}else{

				length = strlen(otherArgs[numArgs-1]);

				if(otherArgs[numArgs-1][0] == '&'){

					amp = 1;

					free(otherArgs[numArgs-1]);
					otherArgs[numArgs-1]=NULL;

				}else if(otherArgs[numArgs-1][length-1] == '&'){

					amp = 1;
					otherArgs[numArgs-1][length-1] = '\0';

				}

			}


			pid = fork();
			i = 0;
			this = tcgetpgrp(STDIN_FILENO);
			

			if(pid == 0){

				signal(SIGTSTP, SIG_DFL);
				signal(SIGINT, SIG_DFL);
				setpgid(0,0);

				if(execvp(firstArg,otherArgs)<0){

					fprintf(stderr,"Not a valid command\n");
					free(firstArg);

					while(otherArgs[i] != NULL){
						free(otherArgs[i]);
						i++;
                    }

					free(otherArgs);
					vector_destroy(list);
					exit(1);

				}

				exit(0);
			}
			

			if(amp == 0){

				tcsetpgrp(STDIN_FILENO, pid);
				waitpid(pid,&status,WSTOPPED);
				tcsetpgrp(STDIN_FILENO, this);

				if(WIFSTOPPED(status) > 0){

					j=malloc(sizeof(struct job));
					j->id=pid;
					j->cmd = otherArgs;
					j->status = 0;

					if(numJobs == 0){
                        

						j->jobNum = 1;

					}else{

                        k = (struct job*)vector_remove(list);
			            j ->jobNum = (k->jobNum) + 1;
			            vector_add(list, k);

					}

					vector_add(list,j);
					curNumJobs = vector_size(list);
					amp = 1;
					fprintf(stderr,"\n[%d]  Stopped\t\t",curNumJobs);

					while(otherArgs[i] != NULL){

						fprintf(stderr,"%s ",otherArgs[i]);
						i++;

					}

					fprintf(stderr,"\n");

				}



			}else{

				j=malloc(sizeof(struct job));
				j->id=pid;
				j->cmd = otherArgs;
				j->status = 1;

				if(numJobs == 0){

					j->jobNum = 1;

				}else{

					k = (struct job*)vector_remove(list);
					j ->jobNum = (k->jobNum) + 1;
					vector_add(list, k);

				}

				vector_add(list,j);
				fprintf(stderr,"[%d] %d\n", j->jobNum, pid);
				waitpid(pid,0,WNOHANG);

			}

			if(amp != 1){

				while(otherArgs[i] != NULL){

					free(otherArgs[i]);
					i++;

				}

				free(otherArgs);
			}

		free(firstArg);
		commands = NULL;
		}
	}

	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

}

void signalHandler(int signum){
	if( signum == 2){
		/*fprintf(stderr,"^C");*/
	}else{
		stop = 1;
		/*fprintf(stderr,"^Z");*/
	}
	signal(SIGINT, signalHandler);
	signal(SIGTSTP, signalHandler);
}


void evaluatePathway(char* commands)
{
	if(commands != NULL){
		if(chdir(commands)==0){
			char *curDir = getcwd(NULL,0);
			fprintf(stderr,"%s\n", curDir);
			free(curDir);
		}else{
			fprintf(stderr, "%s does not exist\n", commands);
		}
	}else{
		char *homedir = getenv("HOME");
		if(chdir(homedir)==0){
			char *curDir = getcwd(NULL,0);
			fprintf(stderr,"%s\n", curDir);
			free(curDir);
		}
	}

}

/*Written by Shane Panter*/

char** chopLineExec(const char* line)
{
	char *token = NULL;
	int i=0;
	char linecpy[MAXLINE];
	char **rval = malloc(MAXARGS);
	memset(rval,0,MAXARGS);
	strncpy(linecpy,line,MAXLINE);
	token = strtok(linecpy," ");
	for(i=0;token && i<MAXARGS;i++){
		rval[i]=malloc(MAXLINE);
		strncpy(rval[i],token,MAXLINE);
		token=strtok(NULL," ");
	}

	return rval;

}

void addAt(void *list, struct job* toAdd){

  	int i, numJobs, found = 0;
	struct job *j = NULL;

	void *listCopy = NULL;
	vector_alloc(&listCopy);
	numJobs = vector_size(list);
	if(numJobs > 0){
		for(i = 0; i < numJobs; i++){

			j = (struct job*)vector_remove(list);
			vector_add(listCopy, j);

		}

		for(i = 0; i < numJobs; i++){

			j = (struct job*)vector_remove(listCopy);
			if(j->jobNum > toAdd->jobNum && found == 0){
				vector_add(list,toAdd);
				found = 1;
			}
			vector_add(list,j);	

		}
		if(found == 0){
			vector_add(list, toAdd);
		}
		
	}else{
	      vector_add(list,toAdd);
	}
	vector_destroy(listCopy);
}
struct job* bgJobs(void *list, int searchFor){
	int i, numJobs, found = 0;
	struct job *j = NULL, *toReturn = NULL;

	void *listCopy = NULL;
	vector_alloc(&listCopy);
	numJobs = vector_size(list);

	for(i = 0; i < numJobs; i++){

		j = (struct job*)vector_remove(list);
		if(j->status == 0 && found == 0 && (j->jobNum == searchFor || searchFor == 0)){
			toReturn = j;
			found = 1;
		}else{
			vector_add(listCopy, j);
		}

	}

	numJobs = vector_size(listCopy);

	for(i = 0; i < numJobs; i++){

		j = (struct job*)vector_remove(listCopy);
		vector_add(list,j);	

	}
	vector_destroy(listCopy);

	return toReturn;

}

struct job* fgJobs(void *list, int searchFor){
	int i, numJobs, found = 0;
	struct job *j = NULL, *toReturn = NULL;

	void *listCopy = NULL;
	vector_alloc(&listCopy);
	numJobs = vector_size(list);

	for(i = 0; i < numJobs; i++){

		j = (struct job*)vector_remove(list);
		if(j->status == 1 && found == 0 && (j->jobNum == searchFor || searchFor == 0)){
			toReturn = j;
			found = 1;
		}else{
			vector_add(listCopy, j);
		}

	}

	numJobs = vector_size(listCopy);

	for(i = 0; i < numJobs; i++){

		j = (struct job*)vector_remove(listCopy);
		vector_add(list,j);	

	}
	vector_destroy(listCopy);

	return toReturn;

}


void handleJobs(void *list){
	/*remove way through list into second list adding only non done jobs
	 *if done, print out
	 */
	int i,x, status, numJobsNew, numJobs;
	pid_t return_pid;
	struct job *j = NULL;

	void *listCopy = NULL;
	vector_alloc(&listCopy);
	numJobs = vector_size(list);
	numJobsNew = vector_size(list);
	x = 0;

	for(i = 0; i < numJobs; i++){

		status = 0;		
		j = (struct job*)vector_remove(list);
		return_pid = waitpid(j->id, &status, WNOHANG); 


		if (return_pid == 0) {
			vector_add(listCopy, j);
		} 
		else if (return_pid == j->id) {
			fprintf(stderr,"[%d] ", j->jobNum);
			fprintf(stderr,"Done \t");
			while(j->cmd[x] != NULL){
				fprintf(stderr,"%s ", j->cmd[x]);
				free(j->cmd[x]);
				x++;
			}
			fprintf(stderr,"&\n");
			free(j->cmd);
			free(j);
			numJobsNew--;
		}

		x = 0;


	}

	for(i = 0; i < numJobsNew; i++){
		j = (struct job*)vector_remove(listCopy);
		vector_add(list,j);	

	}
	vector_destroy(listCopy);

}


void printJobs(void *list){
	int i, x, status = 0, numJobs, stopped = 0;
	pid_t return_pid;
	struct job *j = NULL;

	void *listCopy = NULL;
	vector_alloc(&listCopy);
	numJobs = vector_size(list);
	for(i = 0; i < numJobs; i++){

		j = (struct job*)vector_remove(list);
		vector_add(listCopy, j);

	}

	for(i = 0; i < numJobs; i++){
		x = 0;
		j = (struct job*)vector_remove(listCopy);
		fprintf(stderr,"[%d]  ",j->jobNum);
		return_pid = waitpid(j->id, &status, WNOHANG); 
		/*fprintf(stderr,"%d\n,*status);*/
		if(j->status == 0 && (return_pid == 0)){

			fprintf(stderr,"Stopped\t\t");
			stopped = 1;
		} 

		if (j->status == 1 && (return_pid == 0)) {

			fprintf(stderr,"Running\t\t");
			stopped = 0;
		}

		while(j->cmd[x] != NULL){
			fprintf(stderr,"%s ", j->cmd[x]);
			x++;
		}
		if(!stopped){
			fprintf(stderr,"&\n");
		}else{
			fprintf(stderr,"\n");
		}
		vector_add(list,j);	

	}
	vector_destroy(listCopy);

}

void printUsage(void){
	fprintf(stderr,"Mydash can only accept one command line argument, -v. This will print the version of Mydash.\nIf you received this prompt you either input too many arguments or mistyped -v.\nPlease try again.\n");
}

