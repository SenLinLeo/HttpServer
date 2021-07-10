all: clean server

server:
	gcc -g -o server server.c list_cache.c  -lpthread

clean:
	rm -f server
