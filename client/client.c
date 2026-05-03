#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
  #define close_socket closesocket
  #include <stdarg.h>
#else
  #include "unistd.h"
  #define close_socket close
#endif

static int serverFD;
static uint32_t user;
static volatile uint32_t target;
static volatile int recording = 0;
static volatile int running = 1;

void handle_sigint(int sig) {
    (void)sig;
    if (recording) {
        recording = 0;
        printf("\nCall ended.\n> ");
    } else {
        running = 0;
        printf("\nQuitting.\n");
        exit(0);
    }
}

void *client_record(void *arg) {
    (void)arg;
    PaStream* instream;
    Pa_OpenDefaultStream(&instream, 1, 0, paFloat32, SAMPLE_RATE, BUFFER_SIZE, NULL, NULL);
    Pa_StartStream(instream);
    header_t buffer = {0};
    buffer.sender = user;
    while (recording && running) {
        buffer.target = target;
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
    header_t buffer = {0};
    while (recording) {
        int n = recvfrom(serverFD, (char*)&buffer, sizeof(header_t), 0, NULL, NULL);
        if (n <= 0) break;
        if (buffer.info.flags == 0) Pa_WriteStream(outstream, buffer.data, BUFFER_SIZE);
        else if (buffer.info.flags == 2) {
            recording = 0;
            running = 0;
            Pa_StopStream(outstream);
            Pa_CloseStream(outstream);
            Pa_Terminate();
            close_socket(serverFD);
#ifdef _WIN32
            WSACleanup();
#endif
            exit(0);
        } else if (buffer.info.flags == 3) {
            client_list_t* list = (client_list_t*)buffer.data;
            if (list->count == 0) {
                printf("No other users connected.\n");
            } else {
                printf("Connected users:\n");
                for (uint32_t i = 0; i < list->count; i++) {
                    printf("  %s\n", list->clients[i].name);
                }
            }
            printf("\n> ");
            fflush(stdout);
        }
    }
    Pa_StopStream(outstream);
    Pa_CloseStream(outstream);
    return NULL;
}

static void request_client_list(void) {
    header_t req = {0};
    req.sender = user;
    req.info.flags = 3;
    req.target = 0;
    sendto(serverFD, (char*)&req, sizeof(req), 0, NULL, 0);
}

static void request_stored_messages(void) {
    header_t req = {0};
    req.sender = user;
    req.info.flags = 3;
    req.target = 1;
    sendto(serverFD, (char*)&req, sizeof(req), 0, NULL, 0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <username>\n", argv[0]);
        exit(1);
    }
    user = hash_user(argv[1]);
    signal(SIGINT, handle_sigint);
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
    header_t reg = {0};
    reg.info.flags = 1;
    reg.sender = user;
    strncpy((char*)reg.data, argv[1], sizeof(reg.data) - 1);
    reg.info.len = strlen(argv[1]) + 1;
    sendto(serverFD, (char*)&reg, sizeof(reg), 0, NULL, 0);
    printf("Registered as: %s\n\n", argv[1]);
    Pa_Initialize();
    
    pthread_t play_tid;
    pthread_create(&play_tid, NULL, client_play, NULL);
    pthread_detach(play_tid);

    char line[256];
    while (1) {
        printf("Commands:\n");
        printf("  list\n");
        printf("  voicemail\n");
        printf("  call <user>\n");
        printf("  broadcast\n");
        printf("  quit\n");
        printf("> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "list") == 0) {
            request_client_list();
        } else if (strcmp(line, "voicemail") == 0) {
            printf("Checking for stored messages... \n");
            request_stored_messages();
        } else if (strncmp(line, "call ", 5) == 0) {
            const char *target_name = line + 5;
            if (strlen(target_name) == 0) {
                printf("Usage: call <username>\n\n");
                continue;
            }
            target = hash_user(target_name);
            recording = 1;
            printf("Calling %s... (Ctrl+C to stop)\n", target_name);
            pthread_t record_tid;
            pthread_create(&record_tid, NULL, client_record, NULL);
            pthread_join(record_tid, NULL);
        } else if (strcmp(line, "broadcast") == 0) {
            target = 0;
            recording = 1;
            printf("Broadcasting to all... (Ctrl+C to stop)\n");
            pthread_t record_tid;
            pthread_create(&record_tid, NULL, client_record, NULL);
            pthread_join(record_tid, NULL);
        } else if (strcmp(line, "quit") == 0) {
            header_t disc = {0};
            disc.info.flags = 2;
            disc.sender = user;
            sendto(serverFD, (char*)&disc, sizeof(disc), 0, NULL, 0);
            break;
        } else if (strlen(line) > 0) {
            printf("Unknown command: %s\n\n", line);
        }
    }
    recording = 0;
    running = 0;
    Pa_Terminate();
    close_socket(serverFD);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}