#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
 #include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h> 

int main(){
    pid_t cpid_1;
    pid_t cpid_2;

    char *input;
    char *split_string[200];
    char *filename;
    int pipefd[2];
    char buf;

    
    
    while(1){
        
        
        int input_fd = -1; //i file directory number 
        int output_fd = -1; //o file directory number
        input = readline("# ");
        
        //Parse string
        char* token = strtok(input, " ");
        int i = 0;
        split_string[i] = token;
        while (token != NULL){
            token = strtok(NULL, " ");
            split_string[++i] = token;
        }

        int pipe_i = -1;
        for(int i = 0; split_string[i] != NULL; i++){
            if(strcmp(split_string[i], "|") == 0){
                pipe_i = i;
                break;
            }
        }

        //break down into left and right pipe processes 
        char **left_process = split_string;
        char **right_process = &split_string[pipe_i + 1];
        split_string[pipe_i] = NULL; // get rid of the pipe before execute

        pipe(pipefd);
        
        //make 2 processes for left and right child
        cpid_1 = fork();
    
        if (cpid_1 == 0){
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            //redirection if needed
            for(int i = 0; split_string[i] != NULL; i++){
                if(strcmp(split_string[i], "<") == 0){
                    filename = split_string[i+1];
                    input_fd = open(filename, O_RDONLY); //fd = 3 if first or 4 if second...  
                    if (input_fd == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                }
                if(strcmp(split_string[i], ">") == 0){
                    filename = split_string[i+1];
                    output_fd = creat(filename, 0664);   
                }
            }
            //replace < or > with null
            for(int i = 0; split_string[i] != NULL; i++){
                if(strcmp(split_string[i], "<") == 0 || (strcmp(split_string[i], ">") == 0)){
                    split_string[i] = NULL;
                }
            }
            //preform redirection
            if(input_fd != -1){
                dup2(input_fd,STDIN_FILENO); // replace input
                close(input_fd);
            }
            if(output_fd != -1){
                dup2(output_fd,STDOUT_FILENO); // replace output
                close(output_fd);
            }
        
            execvp(left_process[0],left_process);//(1st string, array of args with null as terminal)
            perror("execvp"); // only reached if execvp failed
            exit(EXIT_FAILURE);
        }
        cpid_2 = fork();
        //child process 
        if (cpid_2 == 0){
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            //redirection if needed
            for(int i = 0; split_string[i] != NULL; i++){
                if(strcmp(split_string[i], "<") == 0){
                    filename = split_string[i+1];
                    input_fd = open(filename, O_RDONLY); //fd = 3 if first or 4 if second...  
                    if (input_fd == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                }
                if(strcmp(split_string[i], ">") == 0){
                    filename = split_string[i+1];
                    output_fd = creat(filename, 0664);   
                }
            }
            //replace < or > with null
            for(int i = 0; split_string[i] != NULL; i++){
                if(strcmp(split_string[i], "<") == 0 || (strcmp(split_string[i], ">") == 0)){
                    split_string[i] = NULL;
                }
            }
            //preform redirection
            if(input_fd != -1){
                dup2(input_fd,STDIN_FILENO); // replace input
                close(input_fd);
            }
            if(output_fd != -1){
                dup2(output_fd,STDOUT_FILENO); // replace output
                close(output_fd);
            }
        
            execvp(right_process[0],right_process);//(1st string, array of args with null as terminal)
            perror("execvp"); // only reached if execvp failed
            exit(EXIT_FAILURE);
        }
        if(cpid_1 == -1 || cpid_2 == -1){ 
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else{ //parent call
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(cpid_1,NULL, 0);
            waitpid(cpid_2,NULL, 0);
        }
    }
    
    free(input);
    
    
    return 0;


}
