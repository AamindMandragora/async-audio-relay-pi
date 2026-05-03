#ifndef SERVER_H
#define SERVER_H

#include "protocol.h"
#include "queue.h"

#define MAX_CLIENTS 10
#define MSG_DIR "messages"

void add_client(struct sockaddr_in fd, uint32_t hash, const char *name);

void remove_client(struct sockaddr_in fd);

int start_server(int port);

void *handle_client(void *arg);

#endif