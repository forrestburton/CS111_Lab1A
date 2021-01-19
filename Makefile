#NAME: Forrest Burton
#EMAIL: burton.forrest10@gmail.com
#ID: 005324612


default:
		gcc -g -Wall -Wextra -o lab1a lab1a.c

clean:
		rm -f lab1a *tar.gz

dist:
		tar -czvf lab0-005324612.tar.gz lab1a.c Makefile README

		