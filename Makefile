CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread -Ishared -Iserver

ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32
else
    LIBS =
endif

PI_HOST = pi@cs341-pi-13
PI_DIR = ~/async-audio-relay

SERVER_SRC = server/server.c
CLIENT_SRC = client/client.c

SERVER_OBJ = objs/server.o
CLIENT_OBJ = objs/client.o

SERVER_BIN = server_app
CLIENT_BIN = client_app

all: server client

deploy-server:
	rsync -avz server/ shared/ Makefile $(PI_HOST):$(PI_DIR)/
	ssh $(PI_HOST) "cd $(PI_DIR) && git pull && make server"

run-server: deploy-server
	ssh $(PI_HOST) "cd $(PI_DIR) && ./server_app"

server: $(SERVER_BIN)

client: $(CLIENT_BIN)

run-client: client
	./client_app

$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lportaudio $(LIBS)

objs/server.o: server/server.c | objs
	$(CC) $(CFLAGS) -c $< -o $@

objs/client.o: client/client.c | objs
	$(CC) $(CFLAGS) -c $< -o $@

objs:
	mkdir -p objs

tests:
	$(CC) $(CFLAGS) tests/*.c -o tests/tests_runner

clean:
	rm -rf objs
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
	rm -f tests/tests_runner