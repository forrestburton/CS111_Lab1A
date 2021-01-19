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
        fprintf(stderr, "Error setting up terminal mode: %s\n", strerror(errno));
        exit(1);
    }
    //setting terminal to non-cononical, non-echo mode(letters aren't echoed)
    new_mode.c_iflag = ISTRIP, new_mode.c_oflag = 0, new_mode.c_lflag = 0;
    tcsetattr(0, TCSANOW, &new_mode);
}

void reset_terminal(struct termios normal_mode) {
    int error_check = tcsetattr(0, TCSANOW, &normal_mode);
    if (error_check < 0) {
        fprintf(stderr, "Error restoring terminal mode: %s\n", strerror(errno));
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
    //Shell option
    if (shell_opt) {
        setup_terminal_mode();

        int ret = fork(); //create a child process from main

        if (ret < 0) {
            fprintf(stderr, "Error when forking main: %s\n", strerror(errno));
            exit(1);
        }
        else if (ret == 0) {  //child process will have return value of 0. Output is by default nondeterministic. We don't know order of execution
            
            char* args_exec[] = {"/bin/bash", NULL};
            ret = execvp(args_exec[0], args_exec);  //executing a new (shell) program: /bin/bash
            if (ret == -1) {
                fprintf(stderr, "Error when executing execvp in child process: %s\n", strerror(errno));
                exit(1);
            }
            exit(5);
        }
        else if (ret > 0) {  //parent process will have return value of > 0
            

            int exit_status;
            waitpid(ret, &exit_status, 0);  //wait for child process to finish
            printf("Child process is exiting. Exit code: %d\n", WEXITSTATUS(exit_status));
            exit(0);
        }
    }


    //Default execution(no options given)
    setup_terminal_mode();  //setup terminal and save original state

    char buffer;
    ssize_t ret;
    //read (ASCII) input from keyboard into buffer
    while(1) {
        ret = read(0, &buffer, sizeof(char));  //read bytes from stdin
        if(ret < 0) {  
            fprintf(stderr, "Error reading from standard input: %s\n", strerror(errno));
            exit(1);
        }

        for (int i = 0; i < ret; i++) { //for each char we read in the buffer
            if (buffer == 0x4) {
                write(1, "^D", 2*sizeof(char));
                reset_terminal(normal_mode);
                exit(0);
            }
            else if (buffer == '\r' || buffer == '\n') {
                write(1, "\r\n", 2*sizeof(char));
            }
            else if (buffer == 0x3) {
                write(1, "^C", 2*sizeof(char));
            }
            else {
                write(1, &buffer, sizeof(char));
            }
        }
    }
    exit(0);
}