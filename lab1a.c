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
#include <poll.h>

struct termios normal_mode;
int pipe1[2];  //to shell. pipe[0] is read end of pipe. pipe[1] is write end of pipe
int pipe2[2];  //from shell
int pid;
int shell_opt = 0;
int debug_opt = 0;

void setup_terminal_mode(void) {
    //save current mode so we can restore it at the end 
    int error_check = tcgetattr(0, &normal_mode);  
    struct termios new_mode = normal_mode;
    if (error_check < 0) {
        fprintf(stderr, "Error saving up terminal mode: %s\n", strerror(errno));
        exit(1);
    }
    atexit(reset_terminal);
    //setting terminal to non-cononical, non-echo mode(letters aren't echoed)
    new_mode.c_iflag = ISTRIP, new_mode.c_oflag = 0, new_mode.c_lflag = 0;
    error_check = tcsetattr(0, TCSANOW, &new_mode);
    if (error_check < 0) {
        fprintf(stderr, "Error setting up new terminal mode: %s\n", strerror(errno));
        exit(1);
    }
}

void reset_terminal() {  //reset to original mode
    int error_check = tcsetattr(0, TCSANOW, &normal_mode);
    if (error_check < 0) {
        fprintf(stderr, "Error restoring terminal mode: %s\n", strerror(errno));
        exit(1);
    } 
    if(shell_opt) {
        int shell_status;
        //wait for shell to exit
        waitpid(pid, &shell_status, 0);
        if (shell_status == -1) {
            fprintf(stderr, "Error with child process terminating: %s\n", strerror(errno));
            exit(1);
        }

        //print exit message
        int signal = WTERMSIG(shell_status);
        int status = WEXITSTATUS(shell_status);
        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d \n", signal, status);
    }
    exit(0);
}

void child_processes(void) {
    //close ends of pipe we aren't using
    if (close(pipe1[1]) == -1) { //reading to shell 
        fprintf(stderr, "Error when closing file descriptor: %s\n", strerror(errno));
        exit(1);
    }  
    if (close(pipe2[0]) == -1) { //writing to terminal 
        fprintf(stderr, "Error when closing file descriptor: %s\n", strerror(errno));
        exit(1);
    }    

    //dup creates a copy of the file descriptor oldfd, dup2 allows us to specify file descriptor to be used 
    if (dup2(pipe1[0]), 0) == -1) { //read stdin to shell(child). so now fd 0 points to the read of the shell
        fprintf(stderr, "Error when copying file descriptor: %s\n", strerror(errno));
        exit(1);
    } 
    if (close(pipe1[0]) == -1) {  
        fprintf(stderr, "Error when closing file descriptor: %s\n", strerror(errno));
        exit(1);
    }  

    if (dup2(pipe2[1]), 1) == -1) { //write shell to stdout. fd 1 
        fprintf(stderr, "Error when copying file descriptor: %s\n", strerror(errno));
        exit(1);
    }
    if (dup2(pipe2[1]), 2) == -1) { //write shell to stderr. fd 2
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

void parent_processes(void) {
    //close ends of pipe we aren't using
    if (close(pipe1[0]) == -1) { //closing read for to shell
        fprintf(stderr, "Error when closing file descriptor: %s\n", strerror(errno));
        exit(1);
    }  
    if (close(pipe2[1]) == -1) { //closing write for from shell 
        fprintf(stderr, "Error when closing file descriptor: %s\n", strerror(errno));
        exit(1);
    }   

    //Poll blocks and returns when: one set of file descriptors is ready for I/O or a specified time has passed 
    struct pollfd poll_event[2];
    poll_event[0].fd = 0;  //stdin
    poll_event[0].events = POLLIN + POLLHUP + POLLERR;

    poll_event[1].fd = pipe2[0]; //read in from shell
    poll_event[0].events = POLLIN + POLLHUP + POLLERR;

    int poll_val;
    while(1) {
        poll_val = poll(poll_events, 2, -1);
        if (poll_val < 0) {
            fprintf(stderr, "Error polling: %s\n", strerror(errno));
            exit(1);
        }
        
        ssize_t ret;
        if (poll_event[0].revents & POLLIN) { //Read input from stdin
            char buffer[256];
            ret = read(0, buffer, sizeof(char)*256);  //parent and child each have copies of file descripters. In parent 0 still maps to stdin
            if (ret == -1) {  
                fprintf(stderr, "Error reading from standard input: %s\n", strerror(errno));
                exit(1);
            }

            for (int i = 0; i < ret; i++) { //for each char we read in the buffer
                if (buffer[i] == 0x4) {
                    ret = write(1, "^D", 2*sizeof(char));
                    if (buffer[i] == -1) {  
                        fprintf(stderr, "Error writing to standard output: %s\n", strerror(errno));
                        exit(1);
                    }       
                    close(pipe1[1]);
                }
                else if (buffer[i] == '\r' || buffer == '\n') {
                    ret = write(1, "\r\n", 2*sizeof(char));
                    if (ret == -1) {  
                        fprintf(stderr, "Error writing to standard output: %s\n", strerror(errno));
                        exit(1);
                    }
                    ret = write(pipe1[1], "\n", sizeof(buffer));  //write to shell 
                    if (ret == -1) {  
                        fprintf(stderr, "Error writing to shell: %s\n", strerror(errno));
                        exit(1);
                    }
                }
                else if (buffer[i] == 0x3) {
                    ret = write(1, "^C", 2*sizeof(char));
                    if (ret == -1) {  
                        fprintf(stderr, "Error writing to standard output: %s\n", strerror(errno));
                        exit(1);
                    }
                    kill(pid, SIGINT); //kill child process
                }
                else {
                    ret = write(1, &buffer[i], sizeof(char));
                    if (ret == -1) {  
                        fprintf(stderr, "Error writing to standard output: %s\n", strerror(errno));
                        exit(1);
                    }
                    ret = write(pipe1[1], &buffer[i], sizeof(buffer)); //write to shell 
                    if (ret == -1) {  
                        fprintf(stderr, "Error writing to shell: %s\n", strerror(errno));
                        exit(1);
                    }
                }
            }
        }

        //Read input from shell
        char buf[256];
        if (poll_event[1].revents & POLLIN) {
            ret = read(pipe2[0], buf, sizeof(char)*56);
            if (ret == -1) {  
                fprintf(stderr, "Error reading from shell: %s\n", strerror(errno));
                exit(1);
            }
            for (int i = 0; i < ret; i++) {
                if (buf[i] == '\n') {
                    ret = write(1, "\r\n", 2*sizeof(char));
                    if (buf[i] == -1) {  
                        fprintf(stderr, "Error writing to standard output: %s\n", strerror(errno));
                        exit(1);
                    }    
                } 
                else {
                    ret = write(1, &buf[i], sizeof(char));
                    if (buf[i] == -1) {  
                        fprintf(stderr, "Error writing to standard output: %s\n", strerror(errno));
                        exit(1);
                    }    
                }
            }
        }

        if (poll_event[0].revents & (POLLHUP | POLLERR)) {
            fprintf(stderr, "stdin error: %s\n", strerror(errno));
            exit(1);
        }
    }


    int exit_status;
    waitpid(ret, &exit_status, 0);  //wait for child process to finish
    printf("Child process is exiting. Exit code: %d\n", WEXITSTATUS(exit_status));
    exit(0);
}

int main(int argc, char *argv[]) {
    int c;
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
        pid = fork(); //create a child process from main
        if (pid < 0) {
            fprintf(stderr, "Error when forking main: %s\n", strerror(errno));
            exit(1);
        }
        else if (pid == 0) {  //child process will have return value of 0. Output is by default nondeterministic. We don't know order of execution
            child_processes();
        }
        else if (pid > 0) {  //parent process will have return value of > 0
            parent_processes();
        }
    }
    else {
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
    }
    exit(0);
}