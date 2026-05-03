#define _POSIX_C_SOURCE 200809L

#include "server.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#ifdef _WIN32
  #define close_socket closesocket
  #include <stdarg.h>
  #include <io.h>
#else
  #include "unistd.h"
  #define close_socket close
#endif

typedef struct client {
    uint32_t id;
    char name[MAX_NAME];
    struct sockaddr_in addr;
    int active;
} client;

static client clients[MAX_CLIENTS];
static pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;
static int serverFD;

typedef struct tdata {
    struct sockaddr_in *client_addr;
    header_t *buffer;
} tdata;

void handle_sigint(int sig) {
    (void)sig;
    printf("\nShutting down server. Disconnecting all clients...\n");
    header_t kill_msg = {0};
    kill_msg.info.flags = 2;
    kill_msg.sender = 0;
    pthread_mutex_lock(&clients_lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            sendto(serverFD, (char*)&kill_msg, sizeof(header_t), 0, (struct sockaddr*)&clients[i].addr, sizeof(clients[i].addr));
        }
    }
    pthread_mutex_unlock(&clients_lock);
#ifdef _WIN32
    Sleep(50);
#else
    usleep(50000);
#endif
    close_socket(serverFD);
#ifdef _WIN32
    WSACleanup();
#endif
    printf("Server exited.\n");
    exit(0);
}

void print_addr(struct sockaddr_in *addr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    printf("%s:%d\n", ip, ntohs(addr->sin_port));
}

int cmp_addr(const struct sockaddr_in *a, const struct sockaddr_in *b) {
    return (a->sin_family == b->sin_family) && (a->sin_port == b->sin_port) && (a->sin_addr.s_addr == b->sin_addr.s_addr);
}

static void ensure_msg_dir(void) {
    struct stat st;
    if (stat(MSG_DIR, &st) == -1) {
#ifdef _WIN32
        mkdir(MSG_DIR);
        chmod(MSG_DIR, _S_IREAD | _S_IWRITE);
#else
        mkdir(MSG_DIR, 0755);
#endif
    }
}

static void store_message(uint32_t target_id, header_t *msg) {
    ensure_msg_dir();
    char path[128];
    snprintf(path, sizeof(path), MSG_DIR "/%u.bin", target_id);
    FILE *f = fopen(path, "ab");
    if (f) {
        fwrite(msg, sizeof(header_t), 1, f);
        fclose(f);
        printf("Stored offline message for %u\n", target_id);
    }
}

static void deliver_stored(uint32_t user_id, struct sockaddr_in *addr) {
    char path[128];
    snprintf(path, sizeof(path), MSG_DIR "/%u.bin", user_id);
    FILE *f = fopen(path, "rb");
    if (!f) return;

    header_t msg;
    int count = 0;
    while (fread(&msg, sizeof(header_t), 1, f) == 1) {
        sendto(serverFD, (char*)(msg.data), msg.info.len, 0, (struct sockaddr*)addr, sizeof(*addr));
        count++;
    }
    fclose(f);
    remove(path);
    if (count > 0) {
        printf("Delivered %d stored messages to %u\n", count, user_id);
    }
}

void add_client(struct sockaddr_in in, uint32_t hash, const char *name) {
    pthread_mutex_lock(&clients_lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].active = 1;
            clients[i].addr = in;
            clients[i].id = hash;
            strncpy(clients[i].name, name, MAX_NAME - 1);
            clients[i].name[MAX_NAME - 1] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&clients_lock);
}

struct sockaddr_in *find_client(uint32_t id) {
    struct sockaddr_in *c = NULL;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].id == id) {
            c = &clients[i].addr;
            break;
        }
    }
    return c;
}

void remove_client(struct sockaddr_in fd) {
    pthread_mutex_lock(&clients_lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].addr.sin_addr.s_addr == fd.sin_addr.s_addr) {
            printf("Client disconnected: %s\n", clients[i].name);
            clients[i].active = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_lock);
}

static void send_client_list(struct sockaddr_in *requester, uint32_t requester_id) {
    client_list_t list = {0};
    pthread_mutex_lock(&clients_lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].id != requester_id) {
            list.clients[list.count].id = clients[i].id;
            strncpy(list.clients[list.count].name, clients[i].name, MAX_NAME - 1);
            list.count++;
        }
    }
    pthread_mutex_unlock(&clients_lock);
    sendto(serverFD, (char*)&list, sizeof(list), 0, (struct sockaddr*)requester, sizeof(*requester));
}

int start_server(int port) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        exit(-1);
    }
#endif
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket()");
        exit(-1);
    }
    int setopt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&setopt, sizeof(setopt));
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind()");
        close_socket(sock_fd);
        exit(-1);
    }
    printf("Server listening on port %d\n", port);
    return sock_fd;
}

void *handle_client(void *arg) {
    queue* taskQueue = (queue *)arg;
    while (1) {
        tdata *data = (tdata*)queue_pull(taskQueue);
        header_t *buffer = data->buffer;
        if (buffer->info.flags == 1) {
            char name[MAX_NAME] = {0};
            memcpy(name, buffer->data, buffer->info.len < MAX_NAME ? buffer->info.len : MAX_NAME - 1);
            printf("Client connected: %s (id=%u)\n", name, buffer->sender);
            add_client(*(data->client_addr), buffer->sender, name);
        } else if (buffer->info.flags == 2) {
            remove_client(*(data->client_addr));
        } else if (buffer->info.flags == 3) {
            if (buffer->target == 1) {
                printf("User %u requested stored message playback\n", buffer->sender);
                deliver_stored(buffer->sender, data->client_addr);
            } else {
                send_client_list(data->client_addr, buffer->sender);
            }
        } else {
            if (buffer->target == 0) {
                pthread_mutex_lock(&clients_lock);
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].active && !cmp_addr(&clients[i].addr, data->client_addr)) {
                        sendto(serverFD, (char*)(buffer->data), BUFFER_SIZE * sizeof(float), 0, (struct sockaddr*)&(clients[i].addr), sizeof(clients[i].addr));
                    }
                }
                pthread_mutex_unlock(&clients_lock);
            } else {
                pthread_mutex_lock(&clients_lock);
                struct sockaddr_in *target = find_client(buffer->target);
                if (target) {
                    sendto(serverFD, (char*)(buffer->data), buffer->info.len, 0, (struct sockaddr*)target, sizeof(*target));
                } else {
                    store_message(buffer->target, buffer);
                }
                pthread_mutex_unlock(&clients_lock);
            }
        }
        free(buffer);
        free(data->client_addr);
        free(data);
    }
    return NULL;
}

int main() {
    printf("Starting server\n");
    signal(SIGINT, handle_sigint);
    ensure_msg_dir();
    int init = 1;
    queue* thread_queue = queue_init();
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void*)thread_queue)) {
            perror("pthread_create()");
            init = 0;
            break;
        }
        pthread_detach(tid);
    }

    int sock_fd = start_server(PORT);
    serverFD = sock_fd;
    while (init) {
        struct sockaddr_in* client_addr = malloc(sizeof(struct sockaddr_in));
        header_t* buffer = malloc(sizeof(header_t));
        if (buffer == NULL) {
            printf("Malloc Failed\n");
            break;
        }
#ifdef _WIN32
        int len = sizeof(struct sockaddr_in);
#else
        socklen_t len = sizeof(struct sockaddr_in);
#endif
        int n = recvfrom(sock_fd, (char*)buffer, sizeof(header_t), 0, (struct sockaddr*)client_addr, &len);
        if (n == -1) {
            close_socket(sock_fd);
            break;
        }
        tdata *tInp = malloc(sizeof(tdata));
        if (tInp == NULL) {
            printf("Malloc failed\n");
            continue;
        }
        tInp->client_addr = client_addr;
        tInp->buffer = buffer;
        queue_push(thread_queue, tInp);
    }
    queue_destroy(thread_queue);
}