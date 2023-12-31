#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <errno.h>
#include <pwd.h>
#include <limits.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include <setjmp.h>
#define MAX_PATHS 5
#define MAX_PATH_LEN 500
#define MAX_CMD_LEN 256
#define MAX_ARGS 10
#define MAX_FILE_LEN 20
#define filename "myShellRc"

int execute(char* cmd_args[MAX_ARGS]);
bool find_executable(char* cmd,char *path);
void set_path();
void load_path();
void sig_hand();
void parse_cmd(char* cmd, char* cmd_args[MAX_ARGS]);
void read_cmd(char* cmd);
void print_welcome();
void print_prompt();
void pipe_hand(char *cmd);
int pipe_trim(char *cmd,char *cmd_args[MAX_ARGS]);
void pipe_opt(char *pipe_com[],char *opt[][MAX_PATH_LEN],int i,char *exe[]);
void do_pipe(char *path[],char *opt[][MAX_PATH_LEN], int i);
void red(char *cmd);
int red_trim(char *cmd,char *cmd_args[MAX_ARGS]);
void red_exe(char *red_cmd[MAX_ARGS],int i);
char* pathname[MAX_PATHS];
char* username;
char cwd[PATH_MAX];
jmp_buf env;
bool req_chk;

int main() {
    char cmd[MAX_CMD_LEN];
    char* cmd_args[MAX_ARGS];
    int status = 0;
    username = getpwuid(getuid())->pw_name;
    print_welcome();

	//look like real shell signal set
	signal(SIGINT,sig_hand);
	signal(SIGQUIT,sig_hand);

	load_path();
    while (true) {
		req_chk=false;
		setjmp(env);
		memset(cmd,0,sizeof(cmd));
		memset(cmd_args,0,sizeof(cmd_args));
        read_cmd(cmd);
		parse_cmd(cmd, cmd_args);
        
        if (strcmp(cmd_args[0], "setpath") == 0) { 
			set_path();
            continue;
        }

        execute(cmd_args);
		if(strcmp(cmd_args[0], "cd") == 0){
			getcwd(cwd, PATH_MAX); // get the current working directory
    		printf("Current directory: %s\n", cwd);
		}
    }
}
void print_welcome() {
    getcwd(cwd, PATH_MAX); // get the current working directory

    printf("\n");
    printf("###############################################################\n");
    printf("#                                                             #\n");
    printf("#                 Welcome to Min_Peace Shell!                 #\n");
    printf("#                                                             #\n");
    printf("###############################################################\n");
    printf("\n");
    printf("Username: %s\n", username);
    printf("Current directory: %s\n", cwd);
    printf("\n");

    print_prompt();
}
void print_prompt() {
    getcwd(cwd, PATH_MAX); // get the current working directory

    printf("\033[1;32m%s@myshell:\033[0m\033[1;34m%s\033[0m$ ", username, cwd); // print the prompt
}
void read_cmd(char* cmd) {
    printf("> ");
    fgets(cmd, MAX_CMD_LEN, stdin);
    if (cmd[strlen(cmd)-1] == '\n')
        cmd[strlen(cmd)-1] = '\0'; // remove newline character
}
void sig_hand(int sig){
	if(sig==SIGINT)
		printf("\nYou Input ctr+C you want exit then Input exit\n");
	else if(sig==SIGQUIT)
		printf("\nYou Input ctr+| you want exit then Input exit\n");
}
void parse_cmd(char* cmd, char* cmd_args[MAX_ARGS]) {
	int cnt=0;

	if(strstr(cmd,"|"))		pipe_hand(cmd);
	if(strstr(cmd,">"))	red(cmd);
	char *tok=strtok(cmd," \n");
	while(tok != NULL){
		cmd_args[cnt]=(char *)malloc(sizeof(char) *10);
		strcpy(cmd_args[cnt++],tok);
		tok=strtok(NULL," \n");
	}
	if(!strcmp(cmd_args[0],"exit"))	exit(0);
}
void red(char *cmd){
	char *red_com[MAX_ARGS];
	memset(red_com,0,sizeof(red_com));
	int i=red_trim(cmd,red_com);
	red_exe(red_com,i);
	exit(0);
}
int red_trim(char *cmd,char *cmd_args[MAX_ARGS]){
	int i=0;
	int cnt=0;
	if( strstr(cmd,">>")){
		char *token=strtok(cmd,">>");
		while(token != NULL){
			cmd_args[i]=(char *)malloc(sizeof(char)*10);
			strcpy(cmd_args[i++],token);
			token=strtok(NULL,">>");
		}
		for(int k=0;k<i;k++){
			char *ptr=cmd_args[k];
			for(int j=0;j<strlen(ptr);j++){
				if( ptr[j]== ' ')
					ptr[j]='\0';
			}
		}
		req_chk=true;
		return i;
	}
	if( strstr(cmd,">")){
		char *token=strtok(cmd,">");
		while(token != NULL){
			cmd_args[i]=(char *)malloc(sizeof(char)*10);
			strcpy(cmd_args[i++],token);
			token=strtok(NULL,">");
		}
		for(int k=0;k<i;k++){
			char *ptr=cmd_args[k];
			for(int j=0;j<strlen(ptr);j++){
				if( ptr[j]== ' ')
					ptr[j]='\0';
			}
		}
		return i;
	}
}
void red_exe(char *red_cmd[MAX_ARGS],int i){
	char path[MAX_PATH_LEN];
	int fd;
	if(!req_chk)
		fd=open(red_cmd[i-1],O_CREAT|O_WRONLY|O_TRUNC,0666);
	else
		fd=open(red_cmd[i-1],O_CREAT|O_WRONLY|O_APPEND,0666);
	find_executable(red_cmd[0],path);
	if( fork()==0){
		dup2(fd,1);
		close(fd);
		execl(path,"min",0);
	}
	longjmp(env,1);
}
void pipe_hand(char *cmd){
    char *path[MAX_PATH_LEN];
	char *exe[MAX_PATH_LEN];
	char *pipe_com[MAX_ARGS];
    char executable[MAX_PATH_LEN];
	int i=pipe_trim(cmd,pipe_com);
	char *opt[i][MAX_PATH_LEN];
	memset(opt,0,sizeof(opt));
	pipe_opt(pipe_com,opt,i,exe);
	for(int j=0;j<i;j++){
		path[j]=(char *)malloc(sizeof(char) * 10);
		if(!find_executable(exe[j],path[j]))
        	printf("Command not found.\n");
    }
	do_pipe(path,opt,i);
	longjmp(env,1);
}
int pipe_trim(char *cmd,char *cmd_args[MAX_ARGS]){
	int i=0;
	char *token=strtok(cmd,"|");
	while(token != NULL){
		cmd_args[i]=(char *)malloc(sizeof(char)*10);
		strcpy(cmd_args[i],token);
		token=strtok(NULL,"|");
		i++;
	}
	return i;
}
void pipe_opt(char *pipe_com[],char *opt[][MAX_PATH_LEN],int i,char *exe[]){
	int pipe_cnt=0;
	for(;pipe_cnt<i;pipe_cnt++){
		int cnt=0;
		char *token=strtok(pipe_com[pipe_cnt]," ");
		while(token != NULL){
			opt[pipe_cnt][cnt]=(char *)malloc(sizeof(char)*10);
			if(cnt==0){
				exe[pipe_cnt]=(char *)malloc(sizeof(char)*10);
				strcpy(exe[pipe_cnt],token);
			}
			strcpy(opt[pipe_cnt][cnt],token);
			token=strtok(NULL," ");
			cnt++;
		}
	}
}
void do_pipe(char *path[],char *opt[][MAX_PATH_LEN], int i){
	int pipes[i][2],pipe_cnt=i-1,pid,status;

	/*****  1번째 명령어 실행  *****/
  	pipe(pipes[0]);        
	if ((pid=fork()) == 0) {
		dup2(pipes[0][1], 1);  
    	close(pipes[0][1]);    
		printf("%s\n %s\n",path[0],*opt[0]);
    	execvp(path[0],opt[0]);
    	fprintf(stderr, "execvp() error\n");
  	}
    close(pipes[0][1]);   
	wait(&status);   

	/***`;**  마지막 명령어 제외 모두 실행  *****/
	for (int j = 0; j < pipe_cnt-1; j++) {
		pipe(pipes[j+1]);     
		if ((pid=fork()) == 0) {
			dup2(pipes[j][0], 0);   
			dup2(pipes[j+1][1], 1); 
			close(pipes[j][0]);   
			close(pipes[j+1][1]); 
			execv(path[j+1], opt[j+1]);  
			fprintf(stderr, "execvp() error\n");
		}
		close(pipes[j+1][1]); 
		wait(&status);        
	}
		/*****  마지막 명령어 실행  *****/
	if ( (pid=fork()) == 0) {
		dup2(pipes[pipe_cnt-1][0], 0); 
		close(pipes[pipe_cnt-1][0]); 
		close(pipes[pipe_cnt-1][1]);  
		execv(path[pipe_cnt], opt[pipe_cnt]);  
	}
	wait(&status);
	return;
}
void load_path() {
    FILE* fp;
    char path[MAX_PATH_LEN];
    int i = 0;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error: Could not open file.\n");
        return;
    }

    while (fgets(path, MAX_PATH_LEN, fp) != NULL) {
        path[strcspn(path, "\n")] = '\0'; // remove newline character
        pathname[i] = (char *)malloc(sizeof(char)*10);
        strcpy(pathname[i++], path);
    }

    fclose(fp);
}

void set_path() {
    int i;
    char path[MAX_PATH_LEN];

    for (i = 0; i < MAX_PATHS; i++) {
        printf("Enter path %d: ", i + 1);
        fgets(path, MAX_PATH_LEN, stdin);
        path[strcspn(path, "\n")] = '\0'; // remove newline character
        pathname[i] = realloc(pathname[i], sizeof(char) * (strlen(path) + 1));
        strcpy(pathname[i], path);
    }
}

bool find_executable(char* cmd,char *path) {
    char executable[MAX_PATH_LEN];
    for (int i = 0; i < MAX_PATHS; i++) {
        strcpy(executable, pathname[i]);
        strcat(executable, cmd);
        if (access(executable, X_OK) == 0) {
            strcpy(path, executable);
            return true;
        }
    }
    return false;
}

int execute(char* cmd_args[MAX_ARGS]) {
    char path[MAX_PATH_LEN];
	int st;
    if (!find_executable(cmd_args[0],path)) {
        printf("Command not found.\n");
        return 0;
    }
    if (strcmp(cmd_args[0], "cd") == 0) {
        if (cmd_args[1] == NULL) {
            printf("Error: No directory provided.\n");
            return 0;
        }
        if (chdir(cmd_args[1]) != 0) {
            printf("Error: Could not change directory.\n");
            return 0;
        }
    }
    pid_t pid = fork();
	if (pid == 0) {
		execvp(path,cmd_args);
    	printf("Error: Failed to execute command.\n");
    	exit(1);
	}
	wait(&st);
}
