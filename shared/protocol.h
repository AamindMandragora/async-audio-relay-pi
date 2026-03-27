#define PORT 1490
#define BUFFER_SIZE 512
#define SAMPLE_RATE 16000

ssize_t read_full(int fd, char *buffer, size_t total) {
    size_t received = 0;
    while (received < total) {
#ifdef _WIN32
        int n = recv(fd, buffer + received, total - received, 0);
#else
        int n = read(fd, buffer + received, total - received);
#endif
        if (n <= 0) return n;
        received += n;
    }
    return received;
}