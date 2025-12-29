CC = gcc
CFLAGS = -Wall -pthread -g -I/usr/include/mysql
LDFLAGS_SERVER = -lmysqlclient -pthread -L/usr/lib/x86_64-linux-gnu -L/usr/lib64/mysql
LDFLAGS_CLIENT = -lSDL2 -pthread

SERVER_SRC = Server_side/server.c Server_side/database.c Server_side/game_logic.c
CLIENT_SRC = Client_side/client.c Client_side/game_render.c

all: server client

server: $(SERVER_SRC)
	$(CC) $(CFLAGS) -o server $(SERVER_SRC) $(LDFLAGS_SERVER)

client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o client $(CLIENT_SRC) $(LDFLAGS_CLIENT)

clean:
	rm -f server client

run_server: server
	./server

run_client: client
	./client

.PHONY: all clean run_server run_client