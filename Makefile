CC=gcc

all:
	$(CC) src/server.c -o server
	$(CC) src/client.c -o client

clean:
	rm server
	rm client
