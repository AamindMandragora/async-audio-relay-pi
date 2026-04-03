#include "protocol.h"

uint32_t hash_user(const char *name) {
    uint32_t h = 5381;
    while (*name) {
        h = ((h << 5) + h) + (unsigned char)(*name++);
    }
    return h;
}