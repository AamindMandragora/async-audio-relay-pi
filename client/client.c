#include "client.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
  #define close_socket closesocket
  #include <stdarg.h>
#else
  #include "unistd.h"
  #define close_socket close
#endif

void* client_record(void *arg) {
    int *ptr = (int*)arg;
    int server_fd = *ptr;
    PaStream* instream;
    Pa_OpenDefaultStream(&instream, 1, 0, paFloat32, SAMPLE_RATE, BUFFER_SIZE, NULL, NULL);
    Pa_StartStream(instream);
    header_t buffer;
    int t = 0;
    while (1) {
        PaError read_err = Pa_ReadStream(instream, buffer.data, BUFFER_SIZE);
        if (read_err != paNoError) break;
        if(t == 0) {
            buffer.send_id = -1;
        } else {
            buffer.send_id = 0;
        }
        buffer.recv_id = 0;
        buffer.timestamp = (uint32_t)time(NULL);
        buffer.load_len = BUFFER_SIZE * sizeof(float);
        sendto(server_fd, (char * )&buffer, sizeof(buffer), 0, NULL, sizeof(struct sockaddr_in));
// #ifdef _WIN32
//         send(server_fd, (const char*)&header, sizeof(header), 0);
//         send(server_fd, buffer, header.load_len, 0);
// #else
//         write(server_fd, (const char*)&header, sizeof(header));
//         write(server_fd, buffer, header.load_len);
// #endif
        t++;
    }
    Pa_StopStream(instream);
    Pa_CloseStream(instream);
    return NULL;
}

void* client_play(void *arg) {
    int *ptr = (int*)arg;
    int server_fd = *ptr;
    PaStream* outstream;
    Pa_OpenDefaultStream(&outstream, 0, 1, paFloat32, SAMPLE_RATE, BUFFER_SIZE, NULL, NULL);
    Pa_StartStream(outstream);
    char buffer[BUFFER_SIZE * sizeof(float)];
    while (1) {
        //printf("Listening to message\n");
        if (recvfrom(server_fd, buffer, BUFFER_SIZE * sizeof(float),0, NULL, NULL) <= 0) break;
        PaError read_err = Pa_WriteStream(outstream, buffer, BUFFER_SIZE);
        if (read_err != paNoError) break;
    }
    printf("Failed\n");
    Pa_StopStream(outstream);
    Pa_CloseStream(outstream);
    return NULL;
}

int main() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        exit(-1);
    }
#endif
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket()");
        exit(-1);
    }
    // int setopt = 1;
    // setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&setopt, sizeof(setopt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "10.195.118.204", &server_addr.sin_addr) <= 0) {
        perror("inet_pton()");
        exit(-1);
    }
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }
    int *ptr = malloc(sizeof(int));
    if (ptr == NULL) {
        close_socket(sock_fd);
        exit(1);
    }
    *ptr = sock_fd;
    printf("Connected file descriptor: %d\n", sock_fd);
    Pa_Initialize();
    pthread_t record_tid;
    if (pthread_create(&record_tid, NULL, client_record, (void*)ptr)) {
        perror("pthread_create()");
        free(ptr);
        close_socket(sock_fd);
        exit(1);
    }
    pthread_t play_tid;
    if (pthread_create(&play_tid, NULL, client_play, (void*)ptr)) {
        perror("pthread_create()");
        free(ptr);
        close_socket(sock_fd);
        exit(1);
    }
    pthread_join(record_tid, NULL);
    pthread_join(play_tid, NULL);
    Pa_Terminate();
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}