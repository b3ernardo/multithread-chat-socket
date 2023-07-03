#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>

#define BUFSZ 1024
#define USER_LIMIT 4

void usage(int argc, char **argv) {
    printf("Usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("Example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
};

struct client_data {
    int id;
    int csock;
    struct sockaddr_storage storage;
};

struct client_data *group[USER_LIMIT];
int id_list[USER_LIMIT];
int connected_users = 0;

int id_generator() {
    for (int i = 1; i < USER_LIMIT; i++) {
        if (id_list[i] == 0) {
            id_list[i] = 1;
            return i;
        }
    }
    return 0;
}

void send_to_group(const char *msg, int sender_id) {
    for (int i = 1; i < USER_LIMIT; i++) {
        if (id_list[i] == 1) {
            char response[BUFSZ];
            snprintf(response, BUFSZ, "MSG(%02d, NULL, %s)", sender_id, msg);
            send(group[i]->csock, response, strlen(response), 0);
        }
    }
}

void list_users(int client_socket, int client_id) {
    char user_list[BUFSZ];
    memset(user_list, 0, BUFSZ);

    for (int i = 1; i < USER_LIMIT; i++) {
        if (id_list[i] == 1 && i != client_id) {
            char user_id[4];
            sprintf(user_id, "%02d ", i);
            strcat(user_list, user_id);
        }
    }

    char response[BUFSZ];
    snprintf(response, BUFSZ + 20, "RES_LIST(%s)", user_list);
    send(client_socket, response, strlen(response), 0);
}

void *client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);
    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    while (1) {
        ssize_t count = recv(cdata->csock, buf, BUFSZ - 1, 0);
        if (count <= 0) {
            if (count == 0) {
                char remove_msg[BUFSZ];
                memset(remove_msg, 0, BUFSZ);
                snprintf(remove_msg, BUFSZ, "User %02d left the group!", cdata->id);
                send_to_group(remove_msg, cdata->id);
                printf("User %02d removed\n", cdata->id);
            }
            connected_users--;
            id_list[cdata->id] = 0;
            close(cdata->csock);
            free(cdata);
            pthread_exit(EXIT_SUCCESS);
        }
        
        buf[count] = '\0';

        if (strncmp(buf, "REQ_ADD", sizeof("REQ_ADD") - 1) == 0) {
            if (connected_users >= USER_LIMIT - 1) {
                send(cdata->csock, "ERROR(01)", sizeof("ERROR(01)"), 0);
                free(cdata);
                close(cdata->csock);
                id_list[cdata->id] = 0;
                pthread_exit(EXIT_SUCCESS);
            } else {
                printf("User %02d added\n", cdata->id);
                group[cdata->id] = cdata;
                connected_users++;
            }

            char join_msg[BUFSZ];
            memset(join_msg, 0, BUFSZ);
            sprintf(join_msg, "User %02d joined the group!", cdata->id);
            send_to_group(join_msg, cdata->id);
        } else if (strcmp(buf, "list users\n") == 0) {
            list_users(cdata->csock, cdata->id);
        } else if (strncmp(buf, "REQ_REM(", sizeof("REQ_REM(") - 1) == 0) {
            int requested_id;
            sscanf(buf, "REQ_REM(%02d)", &requested_id);

            if (id_list[requested_id] == 1 && group[requested_id] != NULL) {
                for (int i = 1; i < USER_LIMIT; i++) {
                    if (id_list[i] == 1 && i != requested_id) {
                        char remove_req[BUFSZ];
                        memset(remove_req, 0, BUFSZ);
                        snprintf(remove_req, BUFSZ, "REQ_REM(%02d)", requested_id);
                        send(group[i]->csock, remove_req, strlen(remove_req), 0);
                    }
                }
                send(cdata->csock, "OK(01)", strlen("OK(01)"), 0);
                close(cdata->csock);
                free(cdata);
                id_list[requested_id] = 0;
                pthread_exit(EXIT_SUCCESS);
            } else {
                send(cdata->csock, "ERROR(02)", sizeof("ERROR(02)"), 0);
            }
        } else if (strncmp(buf, "MSG(", sizeof("MSG(") - 1) == 0) {
            char *msg_start = strchr(buf, '\"') + 1;
            char *msg_end = strrchr(buf, '\"');
            size_t msg_len = msg_end - msg_start;
            char recv_msg[BUFSZ];
            memset(recv_msg, 0, BUFSZ);
            strncpy(recv_msg, msg_start, msg_len);
            recv_msg[msg_len] = '\0';

            char time_str[8];
            time_t now = time(NULL);
            struct tm *timeinfo = localtime(&now);
            strftime(time_str, sizeof(time_str), "[%H:%M]", timeinfo);

            int receiver_id = -1;
            if (msg_end != NULL) {
                sscanf(buf, "MSG(%*d, %02d", &receiver_id);
            }

            if (receiver_id == -1) {
                printf("%s %02d: %s\n", time_str, cdata->id, recv_msg);
                for (int i = 1; i < USER_LIMIT; i++) {
                    if (id_list[i] == 1) {
                        char response[BUFSZ];
                        memset(response, 0, BUFSZ);
                        snprintf(response, BUFSZ + 20, "MSG(%02d, NULL, \"%s\")", cdata->id, recv_msg);
                        send(group[i]->csock, response, strlen(response), 0);
                    }
                }
            } else if (id_list[receiver_id] == 1 && group[receiver_id] != NULL) {
                char response[BUFSZ];
                memset(response, 0, BUFSZ);
                snprintf(response, BUFSZ + 20, "MSG(%02d, %02d, \"%s\")", cdata->id, receiver_id, recv_msg);
                send(group[receiver_id]->csock, response, strlen(response), 0);

                for (int i = 0; i < USER_LIMIT; i++) {
                    if (id_list[i] == 1 && group[i] != NULL) {
                        if (i == cdata->id) {
                            send(cdata->csock, response, strlen(response), 0);
                        }
                    }
                }
            } else {
                printf("User %02d not found\n", receiver_id);
                send(cdata->csock, "ERROR(03)", sizeof("ERROR(03)"), 0);
            }
        }
    }
    connected_users--;
    id_list[cdata->id] = 0;

    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (argc < 3) usage(argc, argv);

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) usage(argc, argv);

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) logexit("socket");

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) logexit("setsockopt");

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) logexit("bind");

    if (0 != listen(s, 10)) logexit("listen");

    for (int i = 0; i < USER_LIMIT; i++) group[i] = NULL;

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("Bound to %s, waiting connections\n", addrstr);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) logexit("accept");

        struct client_data *cdata = malloc(sizeof(*cdata));
        if (!cdata) logexit("malloc");

        cdata->csock = csock;
        memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));
        cdata->id = id_generator();

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}