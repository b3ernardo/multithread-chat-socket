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

        if (strcmp(buf, "close connection\n") == 0) {
            printf("Removed Successfully\n");
            char remove_req[BUFSZ];
            send(sock, remove_req, strlen(remove_req), 0);
            close(sock);
            exit(EXIT_SUCCESS);
        }
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
        } else if (strncmp(buf, "ERROR(02)", sizeof("ERROR(02)") - 1) == 0) {
            printf("User not found\n");
            close(sock);
            exit(EXIT_SUCCESS);
        } else if (strncmp(buf, "MSG(", sizeof("MSG(") - 1) == 0) {
            char *msg_start = strchr(buf + sizeof("MSG(") - 1, ',') + 8;
            char *msg_end = strrchr(msg_start, ')') - 1;
            size_t msg_len = msg_end - msg_start + 1;
            char join_msg[BUFSZ];
            strncpy(join_msg, msg_start, msg_len);
            join_msg[msg_len] = '\0';
            printf("%s\n", join_msg);
        } else if (strncmp(buf, "RES_LIST(", sizeof("RES_LIST(") - 1) == 0) {
            char user_list[BUFSZ];
            memset(user_list, 0, BUFSZ);
            parse_user_list(buf + sizeof("RES_LIST(") - 1, user_list);
            printf("%s\n", user_list);
        } else if (strncmp(buf, "REQ_REM(", sizeof("REQ_REM(") - 1) == 0) {
            int requested_id;
            sscanf(buf, "REQ_REM(%d)", &requested_id);
        } else if (strncmp(buf, "OK(01)", sizeof("OK(01)") - 1) == 0) {
            printf("Removed Successfully\n");
            close(sock);
            exit(EXIT_SUCCESS);
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