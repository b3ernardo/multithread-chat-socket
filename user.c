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

// Variável global que armazena o ID do usuário no user.c
int aux_user_id = -1;

void usage(int argc, char **argv) {
    printf("Usage: %s <server IP> <server port>\n", argv[0]);
    printf("Example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

// Formata a lista de usuários
void parse_user_list(const char *response, char *user_list) {
    int length = strlen(response);
    if (response[length - 1] == '\n') length--;
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

// Extrai o ID do remetente mensagem, para que seja possível usar o ID do usuário no user.c
int extract_id (const char* input_string) {
    const char* pattern = "MSG(%d, NULL";
    int msg_id;
    if (sscanf(input_string, pattern, &msg_id) == 1) {
        return msg_id;
    } else {
        return -1;
    }
}

// Thread de envio
void *send_thread(void *arg) {
    int sock = *((int *)arg);
    char buf[BUFSZ];
    while (1) {
        fgets(buf, BUFSZ, stdin);
        ssize_t count = send(sock, buf, strlen(buf), 0);
        if (count <= 0) break;
        // Mensagens enviadas ao servidor
        if (strcmp(buf, "close connection\n") == 0) {
            printf("Removed Successfully\n");
            char remove_req[BUFSZ];
            send(sock, remove_req, strlen(remove_req), 0);
            close(sock);
            exit(EXIT_SUCCESS);
        } else if (strncmp(buf, "send all \"", sizeof("send all \"") - 1) == 0) {
            char *msg_start = strchr(buf, '\"') + 1;
            char *msg_end = strrchr(buf, '\"');
            if ((msg_start != NULL && msg_end != NULL && msg_end > msg_start) || (msg_start != NULL && msg_start == msg_end)) {
                size_t msg_len = msg_end - msg_start;
                char send_msg[BUFSZ];
                memset(send_msg, 0, BUFSZ);
                strncpy(send_msg, msg_start, msg_len);
                send_msg[msg_len] = '\0';
                char response[BUFSZ];
                snprintf(response, BUFSZ + 20, "MSG(%02d, NULL, \"%s\")", aux_user_id, send_msg);
                send(sock, response, strlen(response), 0);
            } else continue;
        } else if (strncmp(buf, "send to", strlen("send to")) == 0) {
            int receiver_id;
            char message[BUFSZ];
            memset(message, 0, BUFSZ);
            int result = sscanf(buf, "send to %02d \"%s\"", &receiver_id, message);
            if (result == 2 && message[strlen(message) - 1] == '"') {
                char msg_buf[BUFSZ];
                memset(msg_buf, 0, BUFSZ);
                snprintf(msg_buf, BUFSZ + 20, "MSG(%02d, %02d, \"%s\")", aux_user_id, receiver_id, message);
                send(sock, msg_buf, strlen(msg_buf), 0);
            } else continue;
        } else continue;
    }
    close(sock);
    pthread_exit(NULL);
}

// Thread de recebimento
void *receive_thread(void *arg) {
    int sock = *((int *)arg);
    char buf[BUFSZ];    

    while (1) {
        ssize_t count = recv(sock, buf, BUFSZ - 1, 0);
        if (count <= 0) break;
        buf[count] = '\0';
        // Mensagens recebidas do servidor
        if (strncmp(buf, "ERROR(01)", sizeof("ERROR(01)") - 1) == 0) {
            printf("User limit exceeded\n");
            close(sock);
            exit(EXIT_SUCCESS);
        } else if (strncmp(buf, "ERROR(02)", sizeof("ERROR(02)") - 1) == 0) {
            printf("User not found\n");
        } else if (strncmp(buf, "ERROR(03)", sizeof("ERROR(03)") - 1) == 0) {
            printf("Receiver not found\n");
        } else if (strncmp(buf, "MSG(", sizeof("MSG(") - 1) == 0) {
            if (aux_user_id == -1) aux_user_id = extract_id(buf);
            char *msg_start = strchr(buf + sizeof("MSG(") - 1, ',') + 8;
            char *msg_end = strrchr(msg_start, ')') - 1;
            size_t msg_len = msg_end - msg_start + 1;

            char msg[BUFSZ];
            strncpy(msg, msg_start, msg_len);
            msg[msg_len] = '\0';
            char join_msg[BUFSZ];
            memset(join_msg, 0, BUFSZ);
            strncpy(join_msg, msg_start, msg_len);
            join_msg[msg_len] = '\0';

            int id1, id2;
            char message[BUFSZ];
            if (strstr(buf, "NULL") == NULL && strstr(buf, "MSG(") != NULL) {
                sscanf(buf, "MSG(%02d, %02d, \"%[^\"]\")", &id1, &id2, message);
                char time_str[8];
                time_t now = time(NULL);
                struct tm *timeinfo = localtime(&now);
                strftime(time_str, sizeof(time_str), "[%H:%M]", timeinfo);

                if (id1 == aux_user_id && id1 != id2) {
                    printf("P %s -> %02d: %s\n", time_str, id2, message);
                } else if (id2 == aux_user_id && id1 != id2) {
                    printf("P %s %02d: %s\n", time_str, id1, message);
                } else if (id1 == id2) {
                    printf("P %s -> %02d: %s\n", time_str, id2, message);
                    printf("P %s %02d: %s\n", time_str, id1, message);
                }
            } else if (join_msg[0] == '\"' && join_msg[msg_len - 1] == '\"') {
                memmove(msg, msg + 1, msg_len - 2);
                msg[msg_len - 2] = '\0';
                char id_str[4];
                size_t id_len = strchr(buf, ',') - (buf + sizeof("MSG(") - 1);
                strncpy(id_str, buf + sizeof("MSG(") - 1, id_len);
                id_str[id_len] = '\0';
                int id = atoi(id_str);

                char time_str[8];
                time_t now = time(NULL);
                struct tm *timeinfo = localtime(&now);
                strftime(time_str, sizeof(time_str), "[%H:%M]", timeinfo);

                if (aux_user_id == id) {
                    printf("%s -> all: %s\n", time_str, msg);
                } else {
                    printf("%s %02d: %s\n", time_str, id, msg);
                }
            } else {
                printf("%s\n", msg);
            }
        } else if (strncmp(buf, "RES_LIST(", sizeof("RES_LIST(") - 1) == 0) {
            char user_list[BUFSZ];
            parse_user_list(buf + sizeof("RES_LIST(") - 1, user_list);
            printf("%s\n", user_list);
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

    char inclusion_request[] = "REQ_ADD";
    ssize_t count = send(sock, inclusion_request, strlen(inclusion_request), 0);
    if (count <= 0) {
        close(sock);
        exit(EXIT_FAILURE);
    }

    pthread_join(send_tid, NULL);
    pthread_join(receive_tid, NULL);

    exit(EXIT_SUCCESS);
}