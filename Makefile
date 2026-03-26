CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread -Ishared -Iserver

SERVER_SRC = server/server.c
CLIENT_SRC = client/client.c

SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

SERVER_BIN = server_app
CLIENT_BIN = client_app

all: $(SERVER_BIN) $(CLIENT_BIN)

server: $(SERVER_BIN)

client: $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

tests:
	$(CC) $(CFLAGS) tests/*.c -o tests/tests_runner

clean:
	rm -f server/*.o client/*.o
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
	rm -f tests/tests_runner