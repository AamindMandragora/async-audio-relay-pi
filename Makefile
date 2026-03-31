CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread -Ishared -Iserver

ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32
else
    LIBS =
endif

PI_HOST = pi@10.195.118.204
PI_DIR = ~/async-audio-relay

SERVER_SRC = server/server.c
CLIENT_SRC = client/client.c

SERVER_OBJ = objs/server.o
CLIENT_OBJ = objs/client.o

SERVER_BIN = bin/server_app
CLIENT_BIN = bin/client_app

PROTOCOL_SRC = shared/protocol.c
PROTOCOL_OBJ = objs/protocol.o

QUEUE_SRC = shared/queue.c
QUEUE_OBJ = objs/queue.o

all: server client

server: $(SERVER_BIN)

client: $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJ) $(PROTOCOL_OBJ) $(QUEUE_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(CLIENT_BIN): $(CLIENT_OBJ) $(PROTOCOL_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^ -lportaudio $(LIBS)

objs/protocol.o: shared/protocol.c | objs
	$(CC) $(CFLAGS) -c $< -o $@

objs/queue.o: shared/queue.c | objs
	$(CC) $(CFLAGS) -c $< -o $@

objs/server.o: server/server.c | objs
	$(CC) $(CFLAGS) -c $< -o $@

objs/client.o: client/client.c | objs
	$(CC) $(CFLAGS) -c $< -o $@

objs:
	mkdir -p objs

bin:
	mkdir -p bin

tests:
	$(CC) $(CFLAGS) tests/*.c -o tests/tests_runner

clean:
	rm -rf objs
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
	rm -f tests/tests_runner