#ifndef SERVER_H
#define SERVER_H

#include "protocol.h"

#define MAX_CLIENTS 2
#define MAX_MESSAGES 100

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

void add_client(int fd);

void remove_client(int fd);

int start_server(int port);

void *handle_client(void *arg);

#endif