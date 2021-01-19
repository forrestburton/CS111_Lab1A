//NAME: Forrest Burton
//EMAIL: burton.forrest10@gmail.com
//ID: 005324612

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

struct termios normal_mode;

void setup_terminal_mode(void) {
    //save current mode so we can restore it at the end 
    int error_check = tcgetattr(0, &normal_mode);  
    struct termios new_mode = normal_mode;
    if (error_check < 0) {
        fprintf(stderr, "Error saving up terminal mode: %s\n", strerror(errno));
        exit(1);
    }
    //setting terminal to non-cononical, non-echo mode(letters aren't echoed)
    new_mode.c_iflag = ISTRIP, new_mode.c_oflag = 0, new_mode.c_lflag = 0;
    error_check = tcsetattr(0, TCSANOW, &new_mode);
    if (error_check < 0) {
        fprintf(stderr, "Error setting up new terminal mode: %s\n", strerror(errno));
        exit(1);
    }
}

void reset_terminal(struct termios normal_mode) {  //reset to original mode
    int error_check = tcsetattr(0, TCSANOW, &normal_mode);
    if (error_check < 0) {
        fprintf(stderr, "Error restoring terminal mode: %s\n", strerror(errno));
        exit(1);
    } 
}

void child_processes(void) {
    //close ends of pipe we aren't using
    if (close(pipe1[1]) == -1) { //not writing from shell->terminal in this pipe
        fprintf(stderr, "Error when closing file descriptor: %s\n", strerror(errno));
        exit(1);
    }  
    if (close(pipe2[0]) == -1) { //not reading from terminal->shell in this pipe
        fprintf(stderr, "Error when closing file descriptor: %s\n", strerror(errno));
        exit(1);
    }    

    //dup creates a copy of the file descriptor oldfd, dup2 allows us to specify file descriptor to be used 
    if (dup2(pipe1[0]), 0) == -1) { //read stdin to terminal->shell(parent->child). fd 0
        fprintf(stderr, "Error when copying file descriptor: %s\n", strerror(errno));
        exit(1);
    } 
    if (close(pipe1[0]) == -1) {  
        fprintf(stderr, "Error when closing file descriptor: %s\n", strerror(errno));
        exit(1);
    }  

    if (dup2(pipe2[1]), 1) == -1) { //write shell->terminal to stdout. fd 1
        fprintf(stderr, "Error when copying file descriptor: %s\n", strerror(errno));
        exit(1);
    }
    if (dup2(pipe2[1]), 2) == -1) { //write shell->terminal to stderr. fd 2
        fprintf(stderr, "Error when copying file descriptor: %s\n", strerror(errno));
        exit(1);
    } 
    if (close(pipe2[1]) == -1) { 
        fprintf(stderr, "Error when closing file descriptor: %s\n", strerror(errno));
        exit(1);
    }   

    char* args_exec[] = {"/bin/bash", NULL};
    ret = execvp(args_exec[0], args_exec);  //executing a new (shell) program: /bin/bash. exec(3) replaces current image process with new one
    if (ret == -1) {
        fprintf(stderr, "Error when executing execvp in child process: %s\n", strerror(errno));
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    int c;
    int shell_opt = 0;
    int debug_opt = 0;
    while(1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"shell", no_argument, 0, 's' },
            {"debug", no_argument, 0, 'd' },
            {0,     0,             0, 0 }};
        c = getopt_long(argc, argv, "", long_options, &option_index);
        if (c == -1) break;
        switch (c) {
            case 's':
                shell_opt = 1;
                break;
            case 'd':
                debug_opy = 1;
                break;
            default:
                printf("Incorrect usage: accepted options are: [--shell --debug]\n");
                exit(1);
        }
    }
    setup_terminal_mode();  //setup terminal and save original state

    //Shell option
    if (shell_opt) {
        int pipe1[2];  //pipe[0] is read end of pipe. pipe[1] is write end of pipe
        int pipe2[2];  //HOW ARE PIPES parent->child or vice versa

        int ret1 = pipe(pipe1);  //parent->child (terminal->shell)
        int ret2 = pipe(pipe2);  //child->parent (shell->terminal)
        if (ret1 == -1) {
            fprintf(stderr, "Error when piping parent->child: %s\n", strerror(errno));
            exit(1);
        }
        if (ret1 == -1) {
            fprintf(stderr, "Error when piping child->parent: %s\n", strerror(errno));
            exit(1);
        }
        
        //signal?

        //stdin is file descriptor 0, stdout is file descripter 1
        //Both parent and child have access to both ends of the pipe. This is how they communicate 
        int ret = fork(); //create a child process from main
        if (ret < 0) {
            fprintf(stderr, "Error when forking main: %s\n", strerror(errno));
            exit(1);
        }
        else if (ret == 0) {  //child process will have return value of 0. Output is by default nondeterministic. We don't know order of execution
            child_processes();
        }
        else if (ret > 0) {  //parent process will have return value of > 0
            

            int exit_status;
            waitpid(ret, &exit_status, 0);  //wait for child process to finish
            printf("Child process is exiting. Exit code: %d\n", WEXITSTATUS(exit_status));
            exit(0);
        }
    }

    //Default execution(no options given)
    char buffer;
    ssize_t ret;
    while(1) {   //read (ASCII) input from keyboard into buffer
        ret = read(0, &buffer, sizeof(char));  //read bytes from stdin
        if (ret == -1) {  
            fprintf(stderr, "Error reading from standard input: %s\n", strerror(errno));
            exit(1);
        }

        for (int i = 0; i < ret; i++) { //for each char we read in the buffer
            if (buffer == 0x4) {
                ret = write(1, "^D", 2*sizeof(char));
                if (ret == -1) {  
                    fprintf(stderr, "Error writing to standard output: %s\n", strerror(errno));
                    exit(1);
                }       
                reset_terminal(normal_mode);
                exit(0);
            }
            else if (buffer == '\r' || buffer == '\n') {
                ret = write(1, "\r\n", 2*sizeof(char));
                if (ret == -1) {  
                    fprintf(stderr, "Error writing to standard output: %s\n", strerror(errno));
                    exit(1);
                }
            }
            else if (buffer == 0x3) {
                ret = write(1, "^C", 2*sizeof(char));
                if (ret == -1) {  
                    fprintf(stderr, "Error writing to standard output: %s\n", strerror(errno));
                    exit(1);
                }
            }
            else {
                ret = write(1, &buffer, sizeof(char));
                if (ret == -1) {  
                    fprintf(stderr, "Error writing to standard output: %s\n", strerror(errno));
                    exit(1);
                }
            }
        }
    }
    exit(0);
}