all: webproxy

webproxy: webproxy.c
	gcc -pthread -o webproxy webproxy.c -lcrypto -w

clean: 
	rm webproxy
