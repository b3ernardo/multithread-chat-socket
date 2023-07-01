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

    close(sock);

    pthread_exit(NULL);
}

void parse_user_list(const char *response, char *user_list) {
    int length = strlen(response);
    if (response[length - 1] == '\n')
        length--;

    int count = 0;
    for (int i = 0; i < length; i++) {
        if (response[i] >= '0' && response[i] <= '9') {
            user_list[count++] = response[i];
            user_list[count++] = response[i + 1];
            user_list[count++] = ' ';
            i++;
        }
    }
    user_list[count - 1] = '\0';
}

void *receive_thread(void *arg) {
    int sock = *((int *)arg);
    char buf[BUFSZ];

    while (1) {
        ssize_t count = recv(sock, buf, BUFSZ - 1, 0);
        if (count <= 0) break;
        buf[count] = '\0';

        if (strncmp(buf, "ERROR(01)", sizeof("ERROR(01)") - 1) == 0) {
            printf("User limit exceeded\n");
            close(sock);
            exit(EXIT_SUCCESS);
        } else if (strncmp(buf, "RES_LIST(", sizeof("RES_LIST(") - 1) == 0) {
            char user_list[BUFSZ];
            memset(user_list, 0, BUFSZ);
            parse_user_list(buf + sizeof("RES_LIST(") - 1, user_list);
            printf("%s\n", user_list);
        } else {
            printf("%s\n", buf);
        }
    }

    close(sock);

    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    if (argc < 3) usage(argc, argv);

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) usage(argc, argv);

    int sock = socket(storage.ss_family, SOCK_STREAM, 0);
    if (sock == -1) logexit("socket");

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(sock, addr, sizeof(storage))) logexit("connect");

    pthread_t send_tid, receive_tid;
    pthread_create(&send_tid, NULL, send_thread, &sock);
    pthread_create(&receive_tid, NULL, receive_thread, &sock);

    char inclusion_request[] = "REQ_ADD\n";
    ssize_t count = send(sock, inclusion_request, strlen(inclusion_request), 0);
    if (count <= 0) {
        close(sock);
        exit(EXIT_FAILURE);
    }

    pthread_join(send_tid, NULL);
    pthread_join(receive_tid, NULL);

    exit(EXIT_SUCCESS);
}