#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#define BUFSZ 1024
#define USER_LIMIT 15

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

int id_generator() {
    for (int i = 1; i < USER_LIMIT; i++) {
        if (id_list[i] == 0) {
            id_list[i] = 1;
            return i;
        };
    };
    return 0;
};

void send_to_group(const char *msg) {
    for (int i = 1; i < USER_LIMIT; i++) {
        if (id_list[i] == 1) send(group[i]->csock, msg, strlen(msg), 0);
    };
};

void * client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data; 
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    if (cdata->id >= 10) {
        printf("User %i added\n", cdata->id);
    } else {
        printf("User 0%i added\n", cdata->id);
    };
    group[cdata->id] = cdata;

    char join_msg[BUFSZ];
    memset(join_msg, 0, BUFSZ);
    if (cdata->id >= 10) {
        sprintf(join_msg, "User %i joined the group!", cdata->id);
    } else {
        sprintf(join_msg, "User 0%i joined the group!", cdata->id);
    };
    send_to_group(join_msg);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    size_t count = recv(cdata->csock, buf, BUFSZ - 1, 0);
    printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

    id_list[cdata->id] = 0;

    close(cdata->csock);

    pthread_exit(EXIT_SUCCESS);
};

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
    };

    exit(EXIT_SUCCESS);
};