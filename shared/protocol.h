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
  uint32_t sender;
  uint32_t target;
	struct {
    uint32_t len : 30;
    uint32_t flags : 2;
  } info;
	uint32_t timestamp;
	float data[BUFFER_SIZE];
} header_t;

uint32_t hash_user(const char *name);