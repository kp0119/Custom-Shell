 #include <fcntl.h>
 #include <stdio.h>
 #include <stdbool.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/types.h> 
 #include <sys/wait.h> 
 #include <signal.h>
 #include <termios.h>

 
 #define INPUT_LENGTH 2048
 #define MAX_ARGS		 512
 

 int exit_status;
// false = normal true = signal
 bool which_exit = false;
 // false = no True = foreground yes
 bool foreground = false;

 struct command_line
 {
     char *argv[MAX_ARGS + 1];
     int argc;
     char *input_file;
     char *output_file;
     bool is_bg;
 };
// signal handler for SIGINT
void handle_SIGINT(int signo){

}
// signal handler for SIGTSTP
void handle_SIGTSTP(int signo){

    if(!foreground){
        char* message = "\nEntering foreground-only mode (& is now ignored)\n: ";
        write(STDOUT_FILENO, message,strlen(message));
        foreground = true;
    }
    else{
        char* message = "\nExiting foreground-only mode\n: ";
        write(STDOUT_FILENO, message,strlen(message));
        foreground = false;
    }
}


 // handles redirection for input and outputs
 void redirect(struct command_line *text){
    if(text -> input_file){
        // open input file
        int inFile = open(text -> input_file, O_RDONLY);
        if (inFile == -1){
            perror("source open()");
            exit(1);
        }
        // redirect stdin to input
        int result = dup2(inFile,0);
        if (result == -1){
            perror("source dup2()");
            exit(2);
        }

    }
    // open output file
    if (text -> output_file){
        int outFile = open(text -> output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (outFile == -1){
            perror("target open()");
            exit(1);
        }
        // redirect stdout
        int result = dup2(outFile,1);
        if (result == -1){
            perror("source dup2()");
            exit(2);
        }

    }    
 }
// for background process
void background(){

    int childStatus;
    pid_t childPid;
    // while loop runs while waitpid hasnt returned 0
    while((childPid = waitpid(-1, &childStatus, WNOHANG) ) > 0){
        printf("background pid %d is done: ",childPid);

        // if terminatd by signal will give signal number
        if(WIFSIGNALED(childStatus)){
            printf("terminated signal %d\n", WTERMSIG(childStatus));
        }
        // if terminated normally it will return the status value passed to exit 
        else if(WIFEXITED(childStatus)){
                printf("exit value %d\n", WEXITSTATUS(childStatus));
        }

    }
}

 void execute(struct  command_line *text)
 {
   int childStatus;
   // fork new process
   pid_t spawnpid = -5;
   spawnpid = fork();
   switch (spawnpid) {
    case -1:
    // forking error
        perror("fork() failed");
        exit(1);
        break;

    case 0:
    {
        //  ignore sigtstp for all child
        struct sigaction childSTP = {0};
        childSTP.sa_handler = SIG_IGN;
        sigaction(SIGTSTP, &childSTP, NULL);


        // is foreground it will ignore
        if(text -> is_bg && !foreground){
            struct sigaction childINT = {0};
            childINT.sa_handler = SIG_IGN;
            sigaction(SIGINT, &childINT, NULL);
        }
        else{ // does default action
            struct sigaction childINT = {0};
            childINT.sa_handler = SIG_DFL;
            sigaction(SIGINT, &childINT, NULL);

        }

        redirect(text);

    // uses execvp to execute command and the arguments 
        execvp(text -> argv[0], text -> argv);
        perror("execvp");
        exit(1);

        
    }

    default:

        if (text->is_bg){
            printf("background pid is %d \n", spawnpid);
        }

        else{

            // waiting for child term
            waitpid(spawnpid, &childStatus, 0);
            // if terminated normally it will return the status value passed to exit 
            if (WIFEXITED(childStatus)){
                exit_status = WEXITSTATUS(childStatus);
                which_exit = false;
            }
            // if terminated by signal it will give the signal number
            else if(WIFSIGNALED(childStatus)){
                exit_status = WTERMSIG(childStatus);
                which_exit = true;
                // added for status
                printf("terminated by signal %d\n",exit_status);
            }


        }

        break;
   }
 };
 
 
 struct command_line *parse_input()
 {
     char input[INPUT_LENGTH];
     struct command_line *curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));

     printf(": ");
     fflush(stdout);
     fgets(input, INPUT_LENGTH, stdin);

     //handles comments
     if (input[0] == '#'){
        return NULL;
     }

     // Tokenize the input
     char *token = strtok(input, " \n");
     while(token){
         if(!strcmp(token,"<")){
             curr_command->input_file = strdup(strtok(NULL," \n"));
         } else if(!strcmp(token,">")){
             curr_command->output_file = strdup(strtok(NULL," \n"));
         } else if(!strcmp(token,"&")){
             curr_command->is_bg = true;
         } else{
             curr_command->argv[curr_command->argc++] = strdup(token);
         }
         token=strtok(NULL," \n");
     }
     
     return curr_command;
 }
 
 int main()
 {
     struct command_line *curr_command;

     struct sigaction SIGINT_action = {0};
     SIGINT_action.sa_handler = handle_SIGINT;
     sigfillset(&SIGINT_action.sa_mask);
     SIGINT_action.sa_flags = 0;
     sigaction(SIGINT, &SIGINT_action, NULL);

     struct sigaction SIGTSTP_action = {0};
     SIGTSTP_action.sa_handler = handle_SIGTSTP;
     sigfillset(&SIGTSTP_action.sa_mask);
     SIGTSTP_action.sa_flags = SA_RESTART;
     sigaction(SIGTSTP, &SIGTSTP_action, NULL);
     
         
     while(true)
     {
        background();

         curr_command = parse_input();
         // handles blanks
         if (curr_command == NULL){
            continue;
         }
        // no background if in foreground
         if (foreground && curr_command -> is_bg){
            curr_command ->is_bg = false;
         }
         
         if (curr_command->argc > 0){

            /*check arugment count then we determine if cd goes HOME or the next argument after cd*/
            if (strcmp(curr_command -> argv[0], "cd") == 0){
                char* curr_path;
                if (curr_command -> argc == 1){
                    curr_path = getenv("HOME");
                }
                else{
                    curr_path = curr_command -> argv[1];
                }
                // changes path based on above
                chdir(curr_path);
                }
            
            /*exits if it reads exit*/
            else  if (strcmp(curr_command -> argv[0], "exit") == 0){
                exit(0);
            }
            // calls executes for commands thats not cd and exit

            else if(strcmp(curr_command -> argv[0], "status") == 0){
                // if it was normal it will return status if its by signal it will return the number
                if (which_exit){
                    printf("terminated by signal %d\n", exit_status);
                }
                else{
                    printf("exit value %d\n",exit_status);
                }
            }    

            else {
                execute(curr_command);
            }

         }

    }


     return EXIT_SUCCESS;
 }
