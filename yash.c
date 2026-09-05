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

int main(){
    pid_t cpid;
    char *input;
    char *split_string[200];
    char *filename;
    int input_fd = -1; //i file directory number 
    int output_fd = -1; //o file directory number
    
    
    
    while(1){
        

        input = readline("# ");
        
        //Parse string
        char* token = strtok(input, " ");
        int i=0;
        split_string[i] = token;
        while (token != NULL){
            token = strtok(NULL, " ");
            split_string[++i] = token;
        }


        //make process
        cpid = fork();

        //child process
        if (cpid == 0){

            //open or create file
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
            
           
            execvp(split_string[0],split_string);//(1st string, array of args with null as terminal)
        }
        
        else if(cpid == -1){ 
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else{ //parent call
            wait((int *)NULL);
        }
        
        free(input);
    }
   
    
    return 0;
    
}
