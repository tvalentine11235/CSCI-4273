/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  char *cmd;
  char *file;
  FILE *fp;
  int size = 0;
  DIR *dp;
  struct dirent *p;

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) 
  {
    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");
   
    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
	   hostp->h_name, hostaddrp);
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
    /* 
     * sendto: echo the input back to the client 
     */
    //n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
      error("ERROR in sendto");

    if (strcmp(buf,"exit\n") == 0)
    {
      n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen); // for recieve at end of client
      exit(1);
    }

    else if (strcmp(buf,"ls\n") == 0)
    {
      bzero(buf,BUFSIZE);
      dp = opendir("./");
      while(p = readdir(dp))
      {
        if(p->d_name[0] != '.')
        {
          strcat(buf,p->d_name);
          strcat(buf, "\n");
        }
      }

      n = sendto(sockfd, buf, BUFSIZE, 0, &clientaddr, clientlen);
    }

    else
    {
      cmd = strtok(buf, " ");
      
      if(strcmp(cmd,"get") == 0)
      {
        //bzero(buf,BUFSIZE);
        //if ((fp = fopen(file, "rb")) != 0);
        file = strtok(NULL, "\n");
        fp = fopen(file, "rb");
        if (fp != NULL)
        {
          while(!feof(fp))
            {
              size = fread(buf, 1, BUFSIZE - 1,fp); // read the first 1024 bytes into the buffer
              n = sendto(sockfd, &size, sizeof(int), 0, &clientaddr, clientlen);
              n = sendto(sockfd, buf, BUFSIZE, 0, &clientaddr, clientlen);
            }
            fclose(fp);
            size = -1;
            n = sendto(sockfd, &size, sizeof(int), 0, &clientaddr, clientlen);
        }
      }

      else if(strcmp(cmd,"put") == 0)
      {
        file = strtok(NULL, "\n");
        fp = fopen(file, "wb");
        if (fp != NULL)
        {
          while(1)
          {
            n = recvfrom(sockfd, &size, sizeof(int), 0, &clientaddr, &clientlen);
            /* client will send -1 if no data was transmitted */
            if (size == -1)
              break;
            /* start receiving packets */
            n = recvfrom(sockfd, buf, sizeof(buf), 0, &clientaddr, &clientlen);
            /* write contents of buffer to the position of the file pointer in increments of 1 byte */
            fwrite(buf, 1, size, fp);
          }
          fclose(fp);
        }
      }

      else if(strcmp(cmd,"delete") == 0)
      {
        file = strtok(NULL, "\n");
        int check = remove(file);
        if (check != 0)
        {
          printf("Delete request failed. Filename %s does not exist.\n", file);
        }
      }
      else
      {
        n = sendto(sockfd, buf, BUFSIZE, 0, &clientaddr, clientlen);
        printf("INVALID REQUEST\n");
      }

      


    }
  }
}
