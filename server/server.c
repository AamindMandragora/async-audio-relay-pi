#define _POSIX_C_SOURCE 200809L

#include "server.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"

static int num_clients = 0;
static int client_fds[MAX_CLIENTS];
static pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

void add_client(int fd) {
    pthread_mutex_lock(&clients_lock);
    if (num_clients < MAX_CLIENTS) {
        client_fds[num_clients++] = fd;
    }
    pthread_mutex_unlock(&clients_lock);
}

void remove_client(int fd) {
    pthread_mutex_lock(&clients_lock);
    for (int i = 0; i < num_clients; i++) {
        if (client_fds[i] == fd) {
            client_fds[i] = client_fds[--num_clients];
            break;
        }
    }
    pthread_mutex_unlock(&clients_lock);
}

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
    dprintf(client_fd, "I see you.\n");
    char buffer[1000];
    while (1) {
        int len = read(client_fd, buffer, sizeof(buffer) - 1);
        if (len <= 0) break;
        buffer[len] = '\0';
        int receiver_fd;
        if (!strncmp(buffer, "get_fds", strlen("get_fds"))) {
            dprintf(client_fd, "The connected fds are: {");
            pthread_mutex_lock(&clients_lock);
            for (int i = 0; i < num_clients; i++) {
                dprintf(client_fd, "%d", client_fds[i]);
                if (i != num_clients - 1) {
                    dprintf(client_fd, ", ");
                }
            }
            pthread_mutex_unlock(&clients_lock);
            dprintf(client_fd, "}.\n");
        } else if (sscanf(buffer, "send_fd %d ", &receiver_fd) == 1) {
            char* msg_start = strchr(buffer, ' ');
            if (msg_start) msg_start = strchr(msg_start + 1, ' ');
            else {
                dprintf(client_fd, "No file descriptor specified!\n");
                continue;
            }
            if (msg_start) msg_start++;
            else {
                dprintf(client_fd, "No message specified!\n");
                continue;
            }
            pthread_mutex_lock(&clients_lock);
            for (int i = 0; i < num_clients; i++) {
                if (client_fds[i] == receiver_fd) {
                    dprintf(receiver_fd, "%s", msg_start);
                }
            }
            pthread_mutex_unlock(&clients_lock);
        } else if (strncmp(buffer, "exit", strlen("exit"))) {
            dprintf(client_fd, "%s\n", buffer);
        }
    }
    remove_client(client_fd);
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
        add_client(client_fd);
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