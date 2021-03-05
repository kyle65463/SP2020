all: threefunctions.o main.c scheduler.c
	gcc -Wall -o main main.c scheduler.c threefunctions.o
threefunctions.o: threefunctions.c
	gcc -c threefunctions.c