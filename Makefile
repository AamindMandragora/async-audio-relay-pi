CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread -Ishared -Iserver

PI_HOST = pi@cs341-pi-13
PI_DIR = ~/async-audio-relay

SERVER_SRC = server/server.c
CLIENT_SRC = client/client.c

SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

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
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lportaudio

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

tests:
	$(CC) $(CFLAGS) tests/*.c -o tests/tests_runner

clean:
	rm -f server/*.o client/*.o
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
	rm -f tests/tests_runner