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

struct termios normal_mode;

void setup_terminal_mode(void) {
    //save current mode so we can restore it at the end 
    int error_check = tcgetattr(0, &normal_mode);  
    if (error_check < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1)
    }
    //setting terminal to non-cononical, non-echo mode(letters aren't echoed)
    tmp.c_iflag = ISTRIP, tmp.c_oflag = 0, tmp.c_lflag = 0;
    struct termios new_mode = normal_mode;
    tcsetattr(0, TCSANOW, &new_mode);
}

int main(int argc, char *argv[]) {
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
                //shell option
                break;
            case 'd':
                //debug option
                break;
            default:
                printf("Incorrect usage: accepted options are: [--shell --debug]\n");
                exit(1);
        }
    }
    
    setup_terminal_mode();
}