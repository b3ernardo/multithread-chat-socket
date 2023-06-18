all:

	gcc -Wall -c common.c
	gcc -Wall user.c common.o -o user
	gcc -Wall -pthread server.c common.o -o server