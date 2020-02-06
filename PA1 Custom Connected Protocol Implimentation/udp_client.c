/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    
    // variables I added

    int size; // buffer size
    FILE *fp; // file pointer
    char *cmd; // first word input from user. command to be performed
    char *file; // second word input from user. name of file

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
    serverlen = sizeof(serveraddr);

    while(1)
    {
        /* zero out the buffer before  */
        bzero(buf, BUFSIZE);
        /* user menu options */
        printf("\nPlease choose from the following options\n");
        printf("Enter your selection in the indicated format\n\n");
        printf("get [filename]: Download a File From the Server\n");
        printf("put [filename]: Upload a File to the Server\n");
        printf("delete [filename]: Delete a File From the Server\n");
        printf("ls : List Files\n");
        printf("exit : Shut Down Server and Exit Client Interface\n");
        fgets(buf, BUFSIZE, stdin);

        /* program exit */
        if (strcmp(buf,"exit\n") == 0)
        {
            n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
            break;
        }
        
        /* list the files on the server */
        else if (strcmp(buf,"ls\n") == 0)
        {
            n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
            bzero(buf, BUFSIZE);
            n = recvfrom(sockfd, &buf, sizeof(buf), 0, &serveraddr, &serverlen);
            if (n < 0)
                error("ERROR in recvfrom");
            printf("\nCONTENTS OF FILE:\n");
            printf("%s\n",buf);
        }
        
        /* commands that require a filename */
        else
        {
            n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
            cmd = strtok(buf, " ");
            file = strtok(NULL, "\n");
            
            /* error message when no filename is entered */
            if(file == NULL)
            {
                printf("\n");
                printf("Please include a filename with a %s command\n\n", cmd);
            }

            /* get command - used to download a file from the server */
            if (strcmp(buf,"get") == 0)
            {
                fp = fopen(file, "wb"); // wb to recieve in binary format
                while(1)
                {
                    /* get the size of the incoming packet */
                    n = recvfrom(sockfd, &size, sizeof(int), 0, &serveraddr, &serverlen);
                    
                    /* server will send -1 if no data was transmitted and the file transfer is complete */
                    if (size == -1)
                        break;

                    /* start receiving packets */
                    n = recvfrom(sockfd, buf, sizeof(buf), 0, &serveraddr, &serverlen);

                    /* write contents of buffer to the position of the file pointer in increments of 1 byte */
                    fwrite(buf, 1, size, fp);                  
                }
                /* close the file once all writing is complete */
                fclose(fp);

            }

            /* put command - used to upload a file to the server */
            else if (strcmp(buf,"put") == 0)
            {
                fp = fopen(file, "rb");
                if (fp != NULL) // check to make sure filename exists
                {
                    while(!feof(fp))
                    {
                        size = fread(buf, 1, BUFSIZE - 1,fp); // read the first 1024 bytes into the buffer
                        n = sendto(sockfd, &size, sizeof(int), 0, &serveraddr, serverlen);
                        n = sendto(sockfd, buf, BUFSIZE, 0, &serveraddr, serverlen);
                    }
                    fclose(fp);
                    size = -1;
                    n = sendto(sockfd, &size, sizeof(int), 0, &serveraddr, serverlen);
                }
            }
            else if (strcmp(buf,"delete") == 0)
            {
                printf("Deleting file: %s\n",file);
            }
            else
            {
                printf("\n");
                printf("Invalid Entry\n");
                n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
                printf("\n");
            }
        }

    }

    /* send the message to the server */
    //serverlen = sizeof(serveraddr);
    //n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto1");
    /* print the server's reply */
    n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
    if (n < 0) 
      error("ERROR in recvfrom");
    printf("Echo from server: %s", buf);
    return 0;
}
