#ifndef SERVER_H
#define SERVER_H

#include "protocol.h"
#include "queue.h"

#define MAX_CLIENTS 10
#define MAX_MESSAGES 1000

typedef struct {
    int id;
    int length;
    char data[BUFFER_SIZE * sizeof(float)];
} AudioMessage;

typedef struct {
    AudioMessage messages[MAX_MESSAGES];
    int count;
    pthread_mutex_t lock;
} MessageQueue;

extern MessageQueue message_queue;

void add_client(struct sockaddr_in fd, uint32_t hash);

void remove_client(struct sockaddr_in fd);

int start_server(int port);

void *handle_client(void *arg);

#endif