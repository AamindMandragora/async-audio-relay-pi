#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
  #define close_socket closesocket
  #include <stdarg.h>
#else
  #include <unistd.h>
  #define close_socket close
#endif

static int serverFD;
static uint32_t user;
static volatile uint32_t target;
static volatile int recording = 0;
static volatile int playback = 1;
static volatile int running = 1;
static volatile int in_call = 0;
static volatile int voicemail = 0;
static volatile uint32_t caller = 0;
static char caller_name[MAX_NAME] = {0};

void handle_sigint(int sig) {
    (void)sig;
    if (recording || in_call == 2) {
        recording = 0;
        in_call = 0;
        voicemail = 0;
        header_t end = {0};
        end.flags = 7;
        end.sender = user;
        end.target = target;
        sendto(serverFD, (char*)&end, sizeof(end), 0, NULL, 0);
    } else if (in_call == 1) {
        in_call = 0;
        header_t end = {0};
        end.flags = 7;
        end.sender = user;
        end.target = target;
        sendto(serverFD, (char*)&end, sizeof(end), 0, NULL, 0);
    } else {
        running = 0;
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
        buffer.flags = 0;
        buffer.len = BUFFER_SIZE * sizeof(float);
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
    while (playback && running) {
        int n = recvfrom(serverFD, (char*)&buffer, sizeof(header_t), 0, NULL, NULL);
        if (n <= 0) break;
        if (buffer.flags == 0 && playback) Pa_WriteStream(outstream, buffer.data, BUFFER_SIZE);
        else if (buffer.flags == 2) {
            playback = 0;
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
        } else if (buffer.flags == 3) {
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
        } else if (buffer.flags == 4) {
            caller = buffer.sender;
            memset(caller_name, 0, MAX_NAME);
            if (buffer.len > 0 && buffer.len < MAX_NAME) memcpy(caller_name, buffer.data, buffer.len);
            printf("\n*** Incomming call from %s ***\n", caller_name);
            printf("Type 'accept' or 'reject'\n> ");
            fflush(stdout);
        } else if (buffer.flags == 5) {
            printf("Call accepted! Speaking...\n");
            in_call = 2;
            recording = 1;
            pthread_t record_tid;
            pthread_create(&record_tid, NULL, client_record, NULL);
            pthread_detach(record_tid);
        } else if (buffer.flags == 6) {
            printf("No answer. Recording voicemail... (Ctrl+C to stop)\n");
            in_call = 0;
            voicemail = 1;
            recording = 1;
            pthread_t record_tid;
            pthread_create(&record_tid, NULL, client_record, NULL);
            pthread_detach(record_tid);
        } else if (buffer.flags == 7) {
            printf("\nCall ended by other party.\n> ");
            fflush(stdout);
            recording = 0;
            in_call = 0;
            voicemail = 0;
        }
    }
    Pa_StopStream(outstream);
    Pa_CloseStream(outstream);
    return NULL;
}

static void request_client_list(void) {
    header_t req = {0};
    req.sender = user;
    req.flags = 3;
    req.target = 0;
    sendto(serverFD, (char*)&req, sizeof(req), 0, NULL, 0);
}

static void request_stored_messages(void) {
    header_t req = {0};
    req.sender = user;
    req.flags = 3;
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
    reg.flags = 1;
    reg.sender = user;
    strncpy((char*)reg.data, argv[1], sizeof(reg.data) - 1);
    reg.len = strlen(argv[1]) + 1;
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
            in_call = 1;
            printf("Ringing %s...\n", target_name);
            header_t ring = {0};
            ring.flags = 4;
            ring.sender = user;
            ring.target = target;
            sendto(serverFD, (char*)&ring, sizeof(ring), 0, NULL, 0);
            for (int i = 0; i < RING_TIMEOUT && in_call == 1 && running; i++) {
#ifdef _WIN32
                Sleep(1000);
#else
                usleep(1000000);
#endif
            }
            if (in_call == 1) {
                printf("No answer.\n");
                in_call = 0;
            }
            while (in_call == 2 && running) {
#ifdef _WIN32
                Sleep(100);
#else
                usleep(100000);
#endif
            }
            while (voicemail && recording && running) {
#ifdef _WIN32
                Sleep(100);
#else
                usleep(100000);
#endif
            }
            voicemail = 0;
        } else if (strcmp(line, "broadcast") == 0) {
            target = 0;
            recording = 1;
            in_call = 2;
            printf("Broadcasting to all... (Ctrl+C to stop)\n");
            pthread_t record_tid;
            pthread_create(&record_tid, NULL, client_record, NULL);
            pthread_join(record_tid, NULL);
            in_call = 0;
        } else if (strcmp(line, "accept") == 0) {
            if (caller == 0) {
                printf("No incoming call to accept.\n\n");
                continue;
            }
            target = caller;
            in_call = 2;
            recording = 1;
            printf("Call accepted with %s. Speaking... (Ctrl+C to end)\n", caller_name);
            header_t accept = {0};
            accept.flags = 5;
            accept.sender = user;
            accept.target = caller;
            sendto(serverFD, (char*)&accept, sizeof(accept), 0, NULL, 0);
            caller = 0;
            pthread_t record_tid;
            pthread_create(&record_tid, NULL, client_record, NULL);
            while (in_call == 2 && running) {
#ifdef _WIN32
                Sleep(100);
#else
                usleep(100000);
#endif
            }
            pthread_join(record_tid, NULL);
        } else if (strcmp(line, "reject") == 0) {
            if (caller == 0) {
                printf("No incoming call to reject.\n\n");
                continue;
            }
            header_t reject = {0};
            reject.flags = 6;
            reject.sender = user;
            reject.target = caller;
            sendto(serverFD, (char*)&reject, sizeof(reject), 0, NULL, 0);
            printf("Call rejected.\n");
            caller = 0;
            in_call = 0;
        } else if (strcmp(line, "quit") == 0) {
            header_t disc = {0};
            disc.flags = 2;
            disc.sender = user;
            sendto(serverFD, (char*)&disc, sizeof(disc), 0, NULL, 0);
            break;
        } else if (strlen(line) > 0) {
            printf("Unknown command: %s\n\n", line);
        }
    }
    playback = 0;
    recording = 0;
    running = 0;
    Pa_Terminate();
    close_socket(serverFD);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}