#ifndef CLIENT_H
#define CLIENT_H

#include "portaudio.h"
#include "protocol.h"

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

void* client_record(void *arg);

void* client_play(void *arg);

#endif