#include <stdint.h>

#define PORT 1490
#define BUFFER_SIZE 512
#define SAMPLE_RATE 16000

typedef struct header_t {
    uint32_t send_id;
	uint32_t recv_id;
	uint32_t timestamp;
	uint32_t load_len;
} header_t;

ssize_t read_full(int fd, char *buffer, size_t total);