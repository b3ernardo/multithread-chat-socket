#include "common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define BUFSZ 1024

void usage(int argc, char **argv) {
    printf("Usage: %s <server IP> <server port>\n", argv[0]);
    printf("Example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
};

int main(int argc, char **argv) {
    if (argc < 3) usage(argc, argv);

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) usage(argc, argv);

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) logexit("socket");

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage))) logexit("connect");

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    while (1) {
        ssize_t count = recv(s, buf, BUFSZ - 1, 0);
        if (count <= 0) break;

        printf("%s\n", buf);

        if (strncmp(buf, "exit", 4) == 0) break;

        memset(buf, 0, BUFSZ);
    };

    close(s);

    exit(EXIT_SUCCESS);
};

/*
#include "common.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define BUFSZ 1024

void usage(int argc, char **argv) {
    printf("Usage: %s <server IP> <server port>\n", argv[0]);
    printf("Example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
};

int main(int argc, char **argv) {
    if (argc < 3) usage(argc, argv);

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) usage(argc, argv);

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) logexit("socket");

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage))) logexit("connect");

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    recv(s, buf, BUFSZ - 1, 0);
    printf("%s\n", buf);

    while (1) {
        printf("> ");
        char input[BUFSZ];
        fgets(input, BUFSZ - 1, stdin);

        send(s, input, strlen(input), 0);

        if (strncmp(input, "exit", 4) == 0) break;

        recv(s, buf, BUFSZ - 1, 0);
        printf("%s\n", buf);
    };

    exit(EXIT_SUCCESS);
};
*/