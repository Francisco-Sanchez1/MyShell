#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>


#define MAX_JOBS 20

typedef struct {
    pid_t pgid;
    int job_id;
    char* command;
    char* state;
} Job;

Job jobs[MAX_JOBS]; // max number of jobs;
int job_count = 0;

void add_job(pid_t pgid, char* command, char * state){
    if (job_count >= MAX_JOBS){
        printf("Too many jobs\n");
        return;
    }
    jobs[job_count].pgid = pgid;
    jobs[job_count].job_id = job_count + 1;
    jobs[job_count].command = strdup(command);
    jobs[job_count].state = state;
    job_count++;

}

int find_job(pid_t pgid){
    for (int i = 0; i < job_count; i++){
        if (jobs[i].pgid == pgid){
            return i;
        }
    }
    printf("Job not found\n");
}

void handle_status(pid_t pid, int status, char* input){
    if (WIFEXITED(status)) {
        //normal exit sir
    } 
    if (WIFSIGNALED(status)) {// ctrl c
        int sig = WTERMSIG(status);
        if (sig == SIGINT){
            write(STDOUT_FILENO, "\n", 1);
        }
    } 
    if (WIFSTOPPED(status)) {
        int sig = WSTOPSIG(status);
        if (sig == SIGTSTP){
            write(STDOUT_FILENO, "\n", 1);
            add_job(pid, input, "Stopped");
        } 
    }
    if (WIFCONTINUED(status)){
        write(STDOUT_FILENO, "#\n", 1);


    }
}


int main(){
    pid_t cpid_1;
    pid_t cpid_2;   
    pid_t yash_pgid; //terminal procces group

    pid_t cpid_3; // for fg and bg commands

    char *input;
    char *split_string[200];
    char *filename;

    int pipefd[2];

    
    yash_pgid = getpid(); // shell is group leader
    setpgid(0,0); // create process group where terminal is only member and // who is the caller here? (QUESTION)
    tcsetpgrp(STDIN_FILENO, yash_pgid);// yashpgid is now set to foreground
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);// need to ignore or else will be in suspended state
    signal(SIGTTIN, SIG_IGN);
    
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



        bool background_bool = false;
        int background_i = -1;
        for(int i = 0; split_string[i] != NULL; i++){
            if(strcmp(split_string[i], "&") == 0){
                background_bool = true;
                background_i = i;
                break;
            }
        }

        if (pipe_i != -1){
        //break down into left and right pipe processes 
        char **left_process = split_string; // will stop at pipe because it's null now
        char **right_process = &split_string[pipe_i + 1]; // will start after pipe because it's null now
        split_string[pipe_i] = NULL; // get rid of the pipe before execute
        
        pipe(pipefd);

        //make 2 processes for left and right child
        cpid_1 = fork();
        
    
        if (cpid_1 == 0){
             // child 1 new prgroup and is group leader
            setpgid(0, 0);


            signal(SIGINT, SIG_DFL); //reset signal ctrl c 
            signal(SIGTSTP, SIG_DFL);

                    
            close(pipefd[0]);// close stdin
            dup2(pipefd[1], STDOUT_FILENO);//std out is pipe write end
            close(pipefd[1]);


            //redirection if needed
            for(int i = 0; left_process[i] != NULL; i++){
                if(strcmp(left_process[i], "<") == 0){
                    filename = left_process[i+1];
                    input_fd = open(filename, O_RDONLY); //fd = 3 if first or 4 if second...  
                    if (input_fd == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                }
                if(strcmp(left_process[i], ">") == 0){
                    filename = left_process[i+1];
                    output_fd = creat(filename, 0664);   
                }
            }
            //replace < or > with null
            for(int i = 0; left_process[i] != NULL; i++){
                if(strcmp(left_process[i], "<") == 0 || (strcmp(left_process[i], ">") == 0)){
                    left_process[i] = NULL;
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
            exit(1);
        }
        cpid_2 = fork();
        //child process 
        
        if (cpid_2 == 0){
            setpgid(0, cpid_1); // set calling proccess of cpid2 to pgid of cpid_1

            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);            
            
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            //redirection if needed
            for(int i = 0; right_process[i] != NULL; i++){
                if(strcmp(right_process[i], "<") == 0){
                    filename = right_process[i+1];
                    input_fd = open(filename, O_RDONLY); //fd = 3 if first or 4 if second...  
                    if (input_fd == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                }
                if(strcmp(right_process[i], ">") == 0){
                    filename = right_process[i+1];
                    output_fd = creat(filename, 0664);   
                }
            }
            //replace < or > with null
            for(int i = 0; right_process[i] != NULL; i++){
                if(strcmp(right_process[i], "<") == 0 || (strcmp(right_process[i], ">") == 0)){
                    right_process[i] = NULL;
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
            exit(1);
        }
        else if(cpid_1 == -1 || cpid_2 == -1){ 
            perror("fork");
            exit(1);
        }
        else{ //parent call

            close(pipefd[0]);
            close(pipefd[1]);

            setpgid(cpid_1, cpid_1); //group leader cpid1
            setpgid(cpid_2, cpid_1); // cpid2 same group


            tcsetpgrp(STDIN_FILENO, cpid_1);
            int status1, status2;
            pid_t child1_result =  waitpid(cpid_1,&status1, WUNTRACED | WCONTINUED); // CTRL-Z or fg/bg
            pid_t child2_result = waitpid(cpid_2,&status2, WUNTRACED | WCONTINUED);
            
            if (child1_result == -1 || child2_result == -1){
                perror("waitpid failed");
            }
            else if (child1_result == 0 || child2_result == 0){ // WNOHANG

            }
            else{

            handle_status(cpid_1, status1, right_process[0]);  // small helper, called for each
            handle_status(cpid_2, status2, right_process[0]);
            }

            tcsetpgrp(STDIN_FILENO,yash_pgid);// reset terminal to yash group
        }
        }
        else{ // no pipe but can check for &
            split_string[background_i] = NULL; // remove & from command
            cpid_3 = fork();
        
    
            if (cpid_3 == 0){
                // child 3 new prgroup and is group leader
                setpgid(0, 0);

                signal(SIGINT, SIG_DFL); //reset signal ctrl c 
                signal(SIGTSTP, SIG_DFL);

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
                
                execvp(split_string[0], split_string);//(1st string, array of args with null as terminal)
                perror("execvp"); // only reached if execvp failed
                exit(1);
            }
            else
            { //parent call

            if (background_bool == true){
                
            
                setpgid(cpid_3, cpid_3); //group leader cpid1

                add_job(cpid_3, input, "Running");
                printf("[%d] %d\n", job_count, cpid_3);
                
            
            }
            else{
                setpgid(cpid_3, cpid_3); //group leader cpid1

                tcsetpgrp(STDIN_FILENO, cpid_3);
                int status;
                pid_t child3_result =  waitpid(cpid_3,&status, WUNTRACED);
                
                if (child3_result == -1){
                    perror("waitpid failed");
                }
                else{
                    handle_status(cpid_3, status, input);  // small helper, called for each
                }

                tcsetpgrp(STDIN_FILENO,yash_pgid);
            }
        }

        }
    }
    
    free(input);
    
    
    return 0;


}
