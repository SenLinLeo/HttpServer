all: clean server

server:
	gcc -g -o server server.c ListCache.c  -lpthread

clean:
	rm -f server
