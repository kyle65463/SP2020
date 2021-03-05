all: read_server write_server
read_server: main.o server.o
	gcc main.c -D READ_SERVER -c
	gcc server.c -c
	gcc server.o main.o -o read_server
write_server: main.o server.o
	gcc main.c -c
	gcc server.c -c
	gcc server.o main.o -o write_server
clear: 
	rm *.o