/* 
 * tcpechosrv.c - A concurrent TCP echo server using threads
 */

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

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */
#define SIZE_OF_BLACKLIST 20 

int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);
const char delimiter[6] = " \n\0";
char* urlArray [100];
char* ipArray [100];
int z = 0; // global variable for ipArray
int timeout = 60;

int ipCache(char* url)
{
    for (int i = 0; i < 100; i++)
    {
        if (NULL != urlArray[i])
        {
            if (strcmp(urlArray[i], url) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

void appendArrays(char* filename, char* website)
{
    char * copy = malloc(strlen(filename) + 1); 
    strcpy(copy, filename);
    char * copy2 = malloc(strlen(website) + 1);
    strcpy(copy2, website);
    ipArray[z] = copy2;
    urlArray[z] = copy;
    z++;
    printf("%d\n",z);
    printf("FUNCTION-URL = %s\n FUNCTION-IP = %s\n CURRENT-Z = %d\n", urlArray[z-1],ipArray[z-1],z);
}

int main(int argc, char **argv) 
{
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 3) 
    {
	   fprintf(stderr, "usage: %s <port>\n", argv[0]);
	   exit(0);
    }
    port = atoi(argv[1]);
    timeout = atoi(argv[2]);
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

/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd) 
{
    size_t n;
    size_t length;
    char* cmd;
    char* url;
    char* urlCopy;
    char* suffix;
    char* request_type;
    char web_addr [INET6_ADDRSTRLEN];
    char* port_num;
    char* client_message;
    char gReq[MAXLINE];
    int server_socket;
    int port_int;
    struct sockaddr_in serv_addr;
    char buf[MAXLINE];
    char messBuf[MAXLINE];
    char testBuf[MAXLINE];
    char anotherBuffer[MAXLINE];
    int text;
    int match = 0;
    int validFlag = 0;
    int found;
    int size = 0;
    char* line;
    char* notnewline;
    char* blacklist = "blacklist.txt";
    DIR *dp;
    struct dirent *d; 



    n = read(connfd, buf, MAXLINE); // connection and request coming into the server from the client into the buffer
    printf("server received the following request:\n%s\n",buf);
    
    //printf("strlen of buffer %d\n", strlen(buf));
    if(strlen(buf) != 0)
    {
        if (urlArray[0] != NULL)
            printf("TOPURL %s\nTOPIP %s\n", urlArray[0],ipArray[0]);
        strcpy(gReq,buf);
        cmd = strtok(buf,delimiter); // tokenize the command (in this case always a GET)
        url = strtok(NULL,delimiter); // tokenize the requested file
        suffix = strtok(NULL,delimiter);

        if(strncmp(cmd,"GET",3) == 0 && url != NULL && suffix != NULL)
        {
            url = strchr(url,':');
            url++;
            url++;
            url++;
            urlCopy = strchr(url,'/');
            port_num = strchr(url,':');
            char * path = malloc(strlen(urlCopy) + 1); 
            strcpy(path, urlCopy);
            char* fullURL = malloc(strlen(url)+1);
            printf("URL1 - %s\n", url);
            strcpy(fullURL,url);
            printf("RESOLVED ADDRESS: %s FULLURL:%s URL2: %s\n",web_addr, fullURL, url);

            unsigned char hashOutput[MD5_DIGEST_LENGTH];
            MD5_CTX tyler;
            //bzero(hashOutput, sizeof(hashOutput));
            //bzero(&tyler, sizeof(&tyler));
            MD5_Init(&tyler);
            MD5_Update(&tyler, fullURL, strlen(fullURL));
            MD5_Final(hashOutput, &tyler);
            char zach [MD5_DIGEST_LENGTH*2+1];
            for(int i = 0; i < MD5_DIGEST_LENGTH; ++i)
                sprintf(&zach[i*2],"%02x", (unsigned int)hashOutput[i]);
            printf("MY HASH VALUE IS: %s\n",zach);
            
            dp = opendir("./");
            while(d=readdir(dp))
            {
                if(strcmp(d->d_name, zach) == 0)
                {
                    found = 1;
                }
            }
            if (found == 1)
            {
                time_t systime = time(0);
                struct tm const* tm = localtime(&systime);
                struct stat timestat;
                stat(zach,&timestat);
                double diferentdiffernce = difftime(systime,timestat.st_ctime);
                if(diferentdiffernce > timeout)
                {
                    remove(zach);
                    found = 0;
                    printf("FILE %s WAS TOO OLD AND HAS BEEN DELETED\n", zach);
                }
                else
                {
                    printf("FILE %s WAS FOUND IN CASH\n", zach);
                    FILE *fp2;
                    fp2 = fopen(zach, "rb");
                    flockfile(fp2);
                    if(fp2 != NULL)
                    {
                        printf("AQUI\n");
                        while((size = fread(anotherBuffer, 1, MAXLINE - 1,fp2))>0)
                        {
                            printf("%lu\n",size);
                            write(connfd, anotherBuffer, size);
                        }
                        funlockfile(fp2);
                        fclose(fp2);
                    }
                }
            }
            if(found == 0)
            {
                if (port_num != NULL)
                {
                    printf("URL:%s\n",url);
                    printf("PORT NUM:%s\n", port_num);
                    url[strlen(url)-strlen(port_num)] = 0;
                    port_num++;
                    port_int = atoi(port_num);
                }
                else
                {
                    url[strlen(url)-strlen(urlCopy)] = 0;
                    port_int = 80;
                }
                printf("URL: %s\n",url);
                printf("PORT NUM:%s\n", port_num);
                printf("PATH: %s\n",path);
                if (urlCopy != NULL && port_num != NULL)
                {
                    port_num[strlen(port_num)-strlen(urlCopy)] = 0;
                }
                
                int pos = ipCache(url);
                printf("POS: %d\n",pos);
                if (pos == -1)
                {
                    struct hostent *valid_site = gethostbyname(url);
                    if(valid_site)
                    {
                        printf("This is a valid URL\n");
                        if((server_socket = socket(AF_INET, SOCK_STREAM, 0))<0)
                        {
                            printf("INVALID SOCKET\n");
                        }
                        bzero((char *) &serv_addr, sizeof(serv_addr));
                        serv_addr.sin_family = AF_INET;
                        serv_addr.sin_port = htons(port_int);

                        inet_ntop(AF_INET, valid_site->h_addr, web_addr, INET6_ADDRSTRLEN); // flips the order of the bytes
                        printf("BEFORE WRITE URL = %s\n", url);
                        appendArrays(url,web_addr);
                        printf("ARRAY APPENDED: %s\n", ipArray[z-1]);
                        if(inet_pton(AF_INET, web_addr, &serv_addr.sin_addr)<=0) // text to binary
                        {
                            printf("invalid address\n");
                            exit(0);
                        }

                        
                        FILE *fp;
                        fp = fopen(blacklist,"r");
                        if (fp != NULL)
                        {
                            while(getline(&line, &length, fp) > 0)
                            {
                                notnewline = strchr(line,'\n');
                                if (notnewline != NULL)
                                {
                                    *notnewline = 0;
                                }
                                if ((strcmp(line,url) == 0) || (strcmp(line,web_addr) == 0))
                                {
                                    match = 1;
                                }

                            }
                        }
                        if (match != 1)
                        {
                            if(connect(server_socket,(struct sockaddr *) &serv_addr, sizeof(serv_addr))<0)
                            {
                                printf("ERROR CONNECTING\n");
                            }

                            printf("SENDING: %s\n",gReq);
                            write(server_socket,gReq,sizeof(gReq));
                            FILE *fp3;
                            fp3 = fopen(zach,"wb");
                            flockfile(fp3);
                            if (fp3 != NULL)
                            {
                                while((text = read(server_socket, testBuf, sizeof(testBuf)))>0)
                                {
                                    printf("BYTES READ: %d\n",text);
                                    write(connfd, testBuf, text);
                                    fwrite(testBuf,sizeof(char),text,fp3);
                                    bzero(testBuf, sizeof(testBuf));
                                }
                                funlockfile(fp3);
                                fclose(fp3);
                            }
                        }
                        else
                        {
                            printf("HTTP/1.1 403 FORBIDDEN\n");
                            bzero(buf, sizeof(buf));
                            //strcpy(buf,"HTTP/1.1 403 Forbidden\n");
                            write(connfd, "HTTP/1.1 403 Forbidden\n\n",25);
                        }
                    }
                    else
                    {
                        bzero(buf, sizeof(buf));
                        printf("HTTP/1.1 404 NOT FOUND\n");
                        //strcpy(buf,"HTTP/1.1 404 Not Found\n\n");
                        write(connfd, "HTTP/1.1 404 Not Found\n\n",31);
                    }
                }

                /* item was in the cache */
                else
                {
                    printf("%s\n",url);
                    printf("%s\n",ipArray[pos]);
                    printf("%s\n",urlArray[pos]);
                    //char chachedIP[100];
                    //strcpy(chachedIP, chachedIPstar);
                    printf("Using Cached IPAddress\n");
                    if((server_socket = socket(AF_INET, SOCK_STREAM, 0))<0)
                    {
                        printf("INVALID SOCKET\n");
                    }
                    bzero((char *) &serv_addr, sizeof(serv_addr));
                    serv_addr.sin_family = AF_INET;
                    serv_addr.sin_port = htons(port_int);
                    /*
                    char* fullURL = malloc(strlen(url)+strlen(path)+1);
                    strcpy(fullURL,url);
                    strcat(fullURL,path);
                    printf("RESOLVED ADDRESS: %s FULLURL%s\n",ipArray[pos], fullURL);*/
                    inet_aton(ipArray[pos],(struct in_addr*) &serv_addr.sin_addr);

                    FILE *fp;
                    fp = fopen(blacklist,"r");
                    
                    if (fp != NULL)
                    {
                        while(getline(&line, &length, fp) > 0)
                        {
                            notnewline = strchr(line,'\n');
                            if (notnewline != NULL)
                            {
                                *notnewline = 0;
                            }
                            if ((strcmp(line,url) == 0) || (strcmp(line,web_addr) == 0))
                            {
                                match = 1;
                            }

                        }
                    }
                    
                    if (match != 1)
                    {
                        if(connect(server_socket,(struct sockaddr *) &serv_addr, sizeof(serv_addr))<0)
                        {
                            printf("ERROR CONNECTING\n");
                        }

                        printf("SENDING: %s\n",gReq);
                        write(server_socket,gReq,sizeof(gReq));
                        FILE *fp4;
                        fp4 = fopen(zach,"wb");
                        flockfile(fp4);
                        if (fp4 != NULL)
                        {
                            while((text = read(server_socket, testBuf, sizeof(testBuf)))>0)
                            {
                                printf("BYTES READ: %d\n",text);
                                write(connfd, testBuf, text);
                                fwrite(testBuf,sizeof(char),text,fp4);
                                //bzero(testBuf, sizeof(testBuf));
                            }
                            funlockfile(fp4);
                            fclose(fp4);
                        }
                    }
                    else
                    {
                        printf("HTTP/1.1 403 FORBIDDEN");
                        bzero(buf, sizeof(buf));
                        //strcpy(buf,"HTTP/1.1 403 Forbidden\n");
                        write(connfd, "HTTP/1.1 403 Forbidden\n",25);
                    }
                }
            }
        }
        
        else
        {
            bzero(buf, sizeof(buf));
            printf("HTTP/1.1 400 BAD REQUEST\n");
            strcpy(buf,"HTTP/1.1 400 Bad Request\n");
            write(connfd, buf,strlen(buf));
        }
        bzero(buf,sizeof(buf));
    }
}

/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
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
} /* end open_listenfd */