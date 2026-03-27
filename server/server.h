#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #ifdef __MINGW32__
    #include <pthread.h>      // MinGW ships winpthreads
  #else
    // MSVC: you'd need a pthreads-win32 library or switch to Windows threads
    #error "Non-MinGW Windows builds need a pthread implementation"
  #endif
#else
  #include <pthread.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
#endif

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

void add_client(int fd);

void remove_client(int fd);

int start_server(int port);

void *handle_client(void *arg);

void add_message(AudioMessage msg);

int get_messages(AudioMessage *buffer, int max);

void init_queue();

#endif