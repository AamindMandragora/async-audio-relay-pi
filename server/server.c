#define _POSIX_C_SOURCE 200809L

#include "server.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"

int start_server(int port) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket()");
        exit(-1);
    }
    int setopt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &setopt, sizeof(setopt));
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind()");
        close(sock_fd);
        exit(-1);
    }
    if (listen(sock_fd, 10) < 0) {
        perror("listen");
        close(sock_fd);
        exit(-1);
    }
    printf("Server listening on port %d\n", port);
    return sock_fd;
}

void *handle_client(void *arg) {
    int *ptr = (int*)arg; 
    int client_fd = *ptr;
    dprintf(client_fd, "I see you.");
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
    free(ptr);
    return NULL;
}

void add_message(AudioMessage msg) {

}

int get_messages(AudioMessage *buffer, int max) {
    return 1;
}

void init_queue() {

}

int main() {
    int sock_fd = start_server(PORT);
    while (1) {
        int client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept()");
            exit(1);
        }
        int *ptr = malloc(sizeof(int));
        if (ptr == NULL) {
            close(client_fd);
            continue;
        }
        *ptr = client_fd;
        printf("Connected file descriptor: %d\n", client_fd);
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void*)ptr)) {
            perror("pthread_create()");
            free(ptr);
            close(client_fd);
            close(sock_fd);
            exit(1);
        }
        pthread_detach(tid);
    }
}