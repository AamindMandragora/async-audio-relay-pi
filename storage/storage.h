#ifndef STORAGE_H
#define STORAGE_H

#include "protocol.h"

int store_to_file(const header_t header, const char *buffer);

int flush_file(int client_fd, uint32_t client_id);

#endif