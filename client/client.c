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

static int serverFD;
static uint32_t user;
static uint32_t target;

void *client_record(void *arg) {
    (void)arg;
    PaStream* instream;
    Pa_OpenDefaultStream(&instream, 1, 0, paFloat32, SAMPLE_RATE, BUFFER_SIZE, NULL, NULL);
    Pa_StartStream(instream);
    header_t buffer = {0};
    buffer.sender = user;
    buffer.target = target;
    while (1) {
        PaError read_err = Pa_ReadStream(instream, buffer.data, BUFFER_SIZE);
        if (read_err != paNoError) break;
        buffer.info.flags = 0;
        buffer.info.len = BUFFER_SIZE * sizeof(float);
        buffer.timestamp = (uint32_t)time(NULL);
        sendto(serverFD, (char *)&buffer, sizeof(buffer), 0, NULL, 0);
    }
    Pa_StopStream(instream);
    Pa_CloseStream(instream);
    return NULL;
}

void *client_play(void *arg) {
    (void)arg;
    PaStream* outstream;
    Pa_OpenDefaultStream(&outstream, 0, 1, paFloat32, SAMPLE_RATE, BUFFER_SIZE, NULL, NULL);
    Pa_StartStream(outstream);
    char buffer[BUFFER_SIZE * sizeof(float)];
    while (1) {
        if (recvfrom(serverFD, buffer, BUFFER_SIZE * sizeof(float),0, NULL, NULL) <= 0) break;
        PaError read_err = Pa_WriteStream(outstream, buffer, BUFFER_SIZE);
        if (read_err != paNoError) break;
    }
    printf("Failed\n");
    Pa_StopStream(outstream);
    Pa_CloseStream(outstream);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <username> [target_username]\n", argv[0]);
        exit(1);
    }
    user = hash_user(argv[1]);
    target = (argc == 3) ? hash_user(argv[2]) : 0;
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        exit(-1);
    }
#endif
    serverFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverFD < 0) {
        perror("socket()");
        exit(-1);
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "10.195.118.204", &server_addr.sin_addr) <= 0) {
        perror("inet_pton()");
        exit(-1);
    }
    if (connect(serverFD, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }
    int *ptr = malloc(sizeof(int));
    if (ptr == NULL) {
        close_socket(serverFD);
        exit(1);
    }
    printf("Connected file descriptor: %d\n", serverFD);

    header_t reg = {0};
    reg.info.flags = 1;
    reg.sender = user;
    strncpy((char*)reg.data, argv[1], sizeof(reg.data) - 1);
    reg.info.len = strlen(argv[1]) + 1;
    sendto(serverFD, (char*)&reg, sizeof(reg), 0, NULL, 0);

    Pa_Initialize();
    pthread_t record_tid;
    if (pthread_create(&record_tid, NULL, client_record, (void*)ptr)) {
        perror("pthread_create()");
        free(ptr);
        close_socket(serverFD);
        exit(1);
    }
    pthread_t play_tid;
    if (pthread_create(&play_tid, NULL, client_play, (void*)ptr)) {
        perror("pthread_create()");
        free(ptr);
        close_socket(serverFD);
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