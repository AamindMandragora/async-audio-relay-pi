CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread -Ishared -Iserver

PI_HOST = pi@10.195.118.204
PI_DIR = ~/async-audio-relay

SERVER_SRC = server/server.c
CLIENT_SRC = client/client.c

SERVER_OBJ = objs/server.o
CLIENT_OBJ = objs/client.o

PROTOCOL_SRC = shared/protocol.c
PROTOCOL_OBJ = objs/protocol.o

QUEUE_SRC = shared/queue.c
QUEUE_OBJ = objs/queue.o

ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32
    PA_LIBS = -lportaudio $(LIBS)
    SERVER_BIN = bin/server_app.exe
    CLIENT_BIN = bin/client_app.exe
    MKDIR = if not exist "$1" mkdir "$1"
    RM = del /Q
    RMDIR = rmdir /S /Q
else
    UNAME_S := $(shell uname -s)
    LIBS =
    SERVER_BIN = bin/server_app
    CLIENT_BIN = bin/client_app
    MKDIR = mkdir -p $1
    RM = rm -f
    RMDIR = rm -rf

    ifeq ($(UNAME_S),Darwin)
        PA_LIBS = -lportaudio -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreFoundation -framework CoreServices
    else
        PA_LIBS = -lportaudio
    endif
endif

all: server client

server: $(SERVER_BIN)

client: $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJ) $(PROTOCOL_OBJ) $(QUEUE_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(CLIENT_BIN): $(CLIENT_OBJ) $(PROTOCOL_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^ $(PA_LIBS)

objs/protocol.o: shared/protocol.c | objs
	$(CC) $(CFLAGS) -c $< -o $@

objs/queue.o: shared/queue.c | objs
	$(CC) $(CFLAGS) -c $< -o $@

objs/server.o: server/server.c | objs
	$(CC) $(CFLAGS) -c $< -o $@

objs/client.o: client/client.c | objs
	$(CC) $(CFLAGS) -c $< -o $@

objs:
	$(call MKDIR,objs)

bin:
	$(call MKDIR,bin)

tests:
	$(CC) $(CFLAGS) tests/*.c -o tests/tests_runner

clean:
	$(RMDIR) objs
	$(RM) $(SERVER_BIN) $(CLIENT_BIN)
	$(RM) tests/tests_runner

.PHONY: all server client tests clean