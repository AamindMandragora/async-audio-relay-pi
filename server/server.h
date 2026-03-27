#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #ifdef __MINGW32__
    #include <pthread.h>
  #else
    #error "need pthread"
  #endif
#else
  #include <pthread.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
#endif

#include "protocol.h"

#define MAX_CLIENTS 2
#define MAX_MESSAGES 100

typedef struct {
    int id;
    int length;
    float data[BUFFER_SIZE];
} AudioMessage;

typedef struct {
    AudioMessage messages[MAX_MESSAGES];
    int count;
    pthread_mutex_t lock;
} MessageQueue;

extern MessageQueue message_queue;

void add_client(int fd);

void remove_client(int fd);

int start_server(int port);

void *handle_client(void *arg);

#endif