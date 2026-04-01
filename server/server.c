#define _POSIX_C_SOURCE 200809L
#define NTHREADS 4


#include "server.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

#ifdef _WIN32
  #define close_socket closesocket
  #include <stdarg.h>
#else
  #include "unistd.h"
  #define close_socket close
#endif

static int num_clients = 0;
static struct sockaddr_in client_fds[MAX_CLIENTS];
static pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;
static int serverFD;

typedef struct threadInp {
    struct sockaddr_in * client_addr;
    header_t * buffer;
} threadInp;

void print_addr(struct sockaddr_in *addr) {
    char ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));

    printf("%s:%d\n", ip, ntohs(addr->sin_port));
}

int cmp_addr(const struct sockaddr_in *a, const struct sockaddr_in *b) {
    return (a->sin_family == b->sin_family) && (a->sin_port   == b->sin_port) && (a->sin_addr.s_addr == b->sin_addr.s_addr);
}


void add_client(struct sockaddr_in fd) {
    pthread_mutex_lock(&clients_lock);
    if (num_clients < MAX_CLIENTS) {
        client_fds[num_clients++] = fd;
    }
    pthread_mutex_unlock(&clients_lock);
}

void remove_client(struct sockaddr_in fd) {
    pthread_mutex_lock(&clients_lock);
    for (int i = 0; i < num_clients; i++) {
        if ((client_fds[i]).sin_addr.s_addr == fd.sin_addr.s_addr) {
            client_fds[i] = client_fds[--num_clients];
            break;
        }
    }
    pthread_mutex_unlock(&clients_lock);
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
    //Don't need listen on UDP  
    // if (listen(sock_fd, 10) < 0) {
    //     perror("listen");
    //     close_socket(sock_fd);
    //     exit(-1);
    // }
    printf("Server listening on port %d\n", port);
    return sock_fd;
}





void *handle_client(void *arg) {
    queue* taskQueue = (queue *)arg;
    //int *ptr = (int*)arg;
    //char buffer[BUFFER_SIZE * sizeof(float)];
    //header_t header;
    while (1) {
        threadInp * data = (threadInp*)queue_pull(taskQueue);
        header_t * buffer = data->buffer;
        if(buffer->send_id == -1) {
            //I'm new
            printf("Client Connected\n");
            add_client(*(data->client_addr)); 
        } else if (buffer->send_id == - 2) {
            //This is my last message
            remove_client(*(data->client_addr));
        } else {
            if (buffer->recv_id == 0) {
                pthread_mutex_lock(&clients_lock);
                for (int i = 0; i < num_clients; i++) {
                    if (!cmp_addr(client_fds + i, data->client_addr)) {
                        //printf("Sending data\n");
                        sendto(serverFD, (char *)(buffer->data), buffer->load_len,0, (struct sockaddr*)&(client_fds[i]),sizeof(client_fds[i]));
    // #ifdef _WIN32
    //                     sendto(serverFD, (char*)&header, sizeof(header), 0);
    // #else
    //                     write(client_fds[i], (char*)&header, sizeof(header));
    //                     write(client_fds[i], buffer, header.load_len);
    // #endif
                    }
                }
                pthread_mutex_unlock(&clients_lock);
                
            } else {
                // send direct
            }
        }
        free(buffer);
        free(data->client_addr);
        free(data);

    }
    //remove_client(client_fd);
#ifdef _WIN32
    //shutdown(client_fd, SD_BOTH);
#else
    //shutdown(client_fd, SHUT_RDWR);
#endif
    //close_socket(client_fd);
    //free(ptr);
    return NULL;
}

int main() {
    printf("Starting server\n");
    int init = 1;
    queue * thread_queue = queue_init();
    for(size_t i = 0; i < NTHREADS; i ++) {
        pthread_t tid;
        if(pthread_create(&tid, NULL, handle_client, (void*)thread_queue)) {
            perror("pthread_create()");
            init = 0;
            break;
        }
        pthread_detach(tid);
    }

    int sock_fd = start_server(PORT);
    serverFD = sock_fd;
    while (init) {
        // if (num_clients >= MAX_CLIENTS) continue;
        struct sockaddr_in* client_addr = malloc(sizeof(struct sockaddr_in));
        header_t* buffer= malloc(sizeof(header_t));
        if(buffer == NULL) {
            printf("Malloc Failed\n");
            break;
        }
        
#ifdef _WIN32
        int len = sizeof(struct sockaddr_in);
#else
        socklen_t len = sizeof(struct sockaddr_in);
#endif

        //Recvfrom should read an entire data packet
        int n = recvfrom(sock_fd, (char * )buffer, sizeof(header_t),
                0, (struct sockaddr*)client_addr,&len); 
        if(n == -1) {
            close_socket(sock_fd);
            break;
        }
        threadInp * tInp = malloc(sizeof(threadInp));
        if(tInp == NULL){
            printf("Malloc failed\n");
            continue;
        };
        tInp->client_addr = client_addr;
        tInp->buffer = buffer;
        queue_push(thread_queue, tInp);

    }

    queue_destroy(thread_queue);
}


//client will send a packet -> contains its original ID or -1 for give me an ID (cached in file)
//server responds with a packet that has an id, add this person to the address list
//