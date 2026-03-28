#ifndef CLIENT_H
#define CLIENT_H

#include "portaudio.h"
#include "protocol.h"

void* client_record(void *arg);

void* client_play(void *arg);

#endif