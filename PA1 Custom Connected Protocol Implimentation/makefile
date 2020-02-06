all: client server

client: udp_client.c
	gcc -o client udp_client.c -w

server: udp_server.c
	gcc -o server udp_server.c -w

clean:
	rm client 
	rm server
