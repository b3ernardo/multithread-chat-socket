#include "common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFSZ 1024

void usage(int argc, char **argv) {
    printf("Usage: %s <server IP> <server port>\n", argv[0]);
    printf("Example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void *send_thread(void *arg) {
    int sock = *((int *)arg);
    char buf[BUFSZ];

    while (1) {
        fgets(buf, BUFSZ, stdin);
        ssize_t count = send(sock, buf, strlen(buf), 0);
        if (count <= 0) break;
    }

    pthread_exit(NULL);
}

void *receive_thread(void *arg) {
    int sock = *((int *)arg);
    char buf[BUFSZ];

    while (1) {
        ssize_t count = recv(sock, buf, BUFSZ - 1, 0);
        if (count <= 0) break;
        buf[count] = '\0';

        if (strcmp(buf, "User limit exceeded") == 0) {
            printf("User limit exceeded\n");
            break;
        }

        printf("%s\n", buf);
    }

    close(sock);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (argc < 3) usage(argc, argv);

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) usage(argc, argv);

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) logexit("socket");

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage))) logexit("connect");

    pthread_t send_tid, receive_tid;

    pthread_create(&send_tid, NULL, send_thread, &s);
    pthread_create(&receive_tid, NULL, receive_thread, &s);

    pthread_join(send_tid, NULL);
    pthread_join(receive_tid, NULL);

    close(s);

    exit(EXIT_SUCCESS);
}