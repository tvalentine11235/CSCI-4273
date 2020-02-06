#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <openssl/md5.h>
#include <dirent.h>
#include <sys/types.h>


#define MAXLINE 1024
#define BUFSIZE 1024
#define LISTENQ 1024

int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);
const char delimiter[6] = " \n\0";
char* urlArray [100];
char* ipArray [100];
int z = 0; // global variable for ipArray
int timeout = 60;
char folder[MAXLINE];
int size;


int main(int argc, char **argv) 
{
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 3) 
    {
	   fprintf(stderr, "usage: %s <folder> <port>\n", argv[0]);
	   exit(0);
    }
    port = atoi(argv[2]);
    strcpy(folder,argv[1]);
    listenfd = open_listenfd(port);
    while (1) 
    {
	   connfdp = malloc(sizeof(int));
	   *connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
	   pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void * thread(void * vargp) 
{  
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self()); 
    free(vargp);
    echo(connfd);
    close(connfd);
    return NULL;
}


void echo(int connfd)
{
	int n;
	int authorized = 0;
	char* dfs = "dfs.conf";
	char buf[MAXLINE];
	char authbuf[MAXLINE];
	char readbuf[MAXLINE];
	char* line = malloc(BUFSIZE*sizeof(char));
	size_t length = MAXLINE;
	char* piece = malloc(BUFSIZE*sizeof(char));
	char* piece2 = malloc(BUFSIZE*sizeof(char));
	char* command = malloc(BUFSIZE*sizeof(char));
	char* username = malloc(BUFSIZE*sizeof(char));
	char* filename = malloc(BUFSIZE*sizeof(char));
	char* filepath = malloc(BUFSIZE*sizeof(char));
	char* filename2 = malloc(BUFSIZE*sizeof(char));
	char* filepath2 = malloc(BUFSIZE*sizeof(char));
	char* trashline = malloc(BUFSIZE*sizeof(char));
	char* x = malloc(BUFSIZE*sizeof(char));



	bzero(authbuf,BUFSIZE);
	n = read(connfd, authbuf, MAXLINE); // READ 1 - get authentication
	printf("Buffer %s\n",authbuf);

	FILE *fp;
    fp = fopen(dfs,"r");
    if (fp != NULL)
    {
    	printf("in dfs\n");
    	while(getline(&line, &length, fp) > 0)
    	{
    		line[strlen(line)-1] = 0;
    		if (strcmp(authbuf,line) == 0)
    		{
    			printf("Authorized\n");
    			authorized = 1;
    			x = strtok(authbuf," ");
    			strcpy(username,x);
    		}
    	}
    }
    printf("Authentication Provided: %s\n",authbuf);
    bzero(authbuf,BUFSIZE);
    if (authorized == 0)
    {
    	printf("Invalid Authentication\n");
    	n = write(connfd,"Invalid Username/Password. Please try again.\n",46); // WRITE 1a - Authentication Failed.
    }
    else
    {
    	printf("Valid Authentication\n");
    	char foldername[MAXLINE];
    	sprintf(foldername,".%s/%s",folder,username);
    	mkdir(foldername,0777);
    	n = write(connfd,"Welcome User\n\n",17); // WRITE 1b - Authentication passed
    	DIR* dp;
    	dp = opendir(foldername);
    	if (dp != NULL)
    	{
    		printf("%s\n",username);
    	}
    
	    bzero(buf,MAXLINE);
	    n = read(connfd, buf, MAXLINE); // READ 2 - get command (PUT or GET or LIST)
	    
	    if (strcmp(buf,"ls\n") == 0)
	    {
	    	printf("List Path\n");
	    }
	    else
	    {
	    	int filesize;
	    	int transfersize = 0;
	    	x = strtok(buf," ");
	    	strcpy(command,x);
	    	x = strtok(NULL,"\n");
	    	strcpy(filename,x);
	    	printf("command %s %s\n",command,filename);
	    	if (strcmp(command,"put") == 0)
	    	{
	    		FILE* fp;
	    		bzero(buf,MAXLINE);
	    		printf("Reading Piece%s\n",buf);
	    		n = read(connfd,buf,MAXLINE); // READ 3 - piece info
	    		x = strtok(buf, " ");
	    		strcpy(piece,x);
	    		x = strtok(NULL,"\n");
	    		filesize = atoi(x);
	    		sprintf(filepath,".%s/%s/.%s.%s",folder,username,filename,piece);
	    		printf("piece = %s filesize = %d\n",piece,filesize);
	    		fp = fopen(filepath, "wb");
	    		if(fp == NULL)
	    		{
		    		printf("file could not be created\n");
		    	}
		    	else
		    	{
		    		printf("writing okay message\n");
		    		write(connfd,"write okay\n",12); // WRITE 2 - File was created
		    	}
	    		printf("%s\n",filepath);
	    		while (transfersize < filesize)
	    		{
	    			bzero(readbuf,BUFSIZE);
	    			n = read(connfd,readbuf,BUFSIZE); // READ 4* - Read in data from client
	    			transfersize += n;
	    			printf("writing %d BYTES to file\n",n);
	    			fwrite(readbuf, 1, n, fp);
	    		}
	    		fclose(fp);

	    		transfersize = 0;
	    		bzero(buf,BUFSIZE);
	    		bzero(readbuf,BUFSIZE);
	    		write(connfd,"write okay\n",12); // WRITE 3 - Ready to read
	    		read(connfd,trashline,10); // READ 5 - make sure client is ready to send
	    		printf("ready to read\n");
	    		n = read(connfd,buf,6); // READ 6 - second piece info
	    		printf("Supposed piece 2: %s\n",buf);
	    		x = strtok(buf, " ");
	    		strcpy(piece,x);
	    		x = strtok(NULL,"\n");
	    		filesize = atoi(x);
	    		sprintf(filepath,".%s/%s/.%s.%s",folder,username,filename,piece);
	    		printf("piece = %s filesize = %d\n",piece,filesize);
	    		fp = fopen(filepath, "wb");
	    		if(fp == NULL)
	    		{
		    		printf("file could not be created\n");
		    	}
		    	else
		    	{
		    		printf("writing okay message\n");
		    		write(connfd,"write okay\n",12); // WRITE 3 - File was created
		    	}
	    		printf("%s\n",filepath);
	    		while (transfersize < filesize)
	    		{
	    			bzero(readbuf,BUFSIZE);
	    			n = read(connfd,readbuf,BUFSIZE); // READ 6* - Read in data from client
	    			transfersize += n;
	    			printf("writing %d BYTES to file\n",n);
	    			fwrite(readbuf, 1, n, fp);
	    		}
	    		fclose(fp);
	    		bzero(buf,BUFSIZE);
	    		bzero(readbuf,BUFSIZE);
	    	}
	    }
	}
}
















int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
}
