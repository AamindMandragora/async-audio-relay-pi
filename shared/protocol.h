#include <stdint.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #ifdef __MINGW32__
    #include <pthread.h>
  #else
    #error "need pthread"
  #endif
#else
  #include <unistd.h>
  #include <pthread.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <netinet/tcp.h>
#endif

#define PORT 1490
#define BUFFER_SIZE 512
#define SAMPLE_RATE 16000

typedef struct header_t {
  int send_id;
	uint32_t recv_id;
	uint32_t timestamp;
	uint32_t load_len;
  float buffer[BUFFER_SIZE];
} header_t;



ssize_t read_full(int fd, char *buffer, size_t total);