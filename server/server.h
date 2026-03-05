#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 1490
#define MAX_CLIENTS 4
#define BUFFER_SIZE 4096
#define MAX_MESSAGES 100

// structure of an audio message stored on the server
typedef struct {
    int id;
    int length;
    char data[BUFFER_SIZE];
} AudioMessage;

// a thread-safe queue of such audio messages
typedef struct {
    AudioMessage messages[MAX_MESSAGES];
    int count;
    pthread_mutex_t lock;
} MessageQueue;

// global message storage
extern MessageQueue message_queue;

int start_server(int port);

void *handle_client(void *arg);

void add_message(AudioMessage msg);

int get_messages(AudioMessage *buffer, int max);

void init_queue();

#endif