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
#include <sys/types.h>
#include <time.h>
#include <openssl/md5.h>
#include <dirent.h>

#define BUFSIZE 1024
#define STRING_SIZE 100

int flags[4] = {0}; // checks to see if a connection was made
int sockets[4];
struct sockaddr_in serveraddr;
char values[4][STRING_SIZE];
char ports[4][STRING_SIZE]; 


/* 
 * error - wrapper for perror
 */
void error(char *msg) 
{
    perror(msg);
    //exit(0);
}

int min(int totalBytes, int quarter)
{
	if (quarter - totalBytes < BUFSIZE)
		return quarter-totalBytes;
	else
		return BUFSIZE;
}

void connectCheck()
{
	int i = 0;
	for (; i < 4; i++)
	{
    	sockets[i] = socket(AF_INET, SOCK_STREAM, 0);
	    if (sockets[i] < 0) 
	    {
	    	printf("socket less than 0\n");
	        error("ERROR opening socket");
	    }
	    
	    bzero((char *) &serveraddr, sizeof(serveraddr));
	    serveraddr.sin_family = AF_INET;
		int check = inet_aton(values[i],(struct in_addr *)&serveraddr.sin_addr.s_addr);
		serveraddr.sin_port = htons(atoi(ports[i]));
		
		if (check != 0)
		{
			if(connect(sockets[i],(struct sockaddr *) &serveraddr, sizeof(serveraddr))<0)
			{
				flags[i] = 0;
			}
			else
			{
				flags[i] = 1;
			}
		}
	}
}

int main(int argc, char **argv) 
{
	int sockfd, portno, n;
	int serverlen;
	struct hostent *server;
	char *hostname;
	char buf[BUFSIZE];
	char readbuf[BUFSIZE];
	char writebuf[BUFSIZE];

	size_t length;
	char* username = malloc(BUFSIZE*sizeof(char));
	char* password = malloc(BUFSIZE*sizeof(char));
	char* line = malloc(BUFSIZE*sizeof(char));
	char* trashline = malloc(BUFSIZE*sizeof(char));
	char* authentication = malloc(BUFSIZE*sizeof(char)); // username password
	char* dfc = "dfc.conf";
	char* command = malloc(BUFSIZE*sizeof(char));
	char* filename = malloc(BUFSIZE*sizeof(char));
	char* pieceInfo = malloc(BUFSIZE*sizeof(char));
	//char* pieceSize = malloc(BUFSIZE*sizeof(char));

	
	
	// parsing out username, password, ip addresses, and sockets
	FILE *fp;
    fp = fopen(dfc,"r");
    if (fp != NULL)
    {
    	int i = 0;
    	while(getline(&line, &length, fp) > 0)
		{	
			if (i < 4)
			{
            	trashline = strtok(line," ");
            	trashline = strtok(NULL," ");
            	trashline = strtok(NULL,":");
            	sprintf(values[i],"%s",trashline);
            	trashline = strtok(NULL,"\n");
            	sprintf(ports[i],"%s",trashline);
            }
            else if (i == 4)
            {
            	trashline = strtok(line," ");
            	strcpy(username,strtok(NULL, "\n"));
            }
            else if (i == 5)
            {
            	trashline = strtok(line," ");
            	strcpy(password,strtok(NULL,"\n"));
            }
            i++;
        }
    }
    strcat(authentication,username); strcat(authentication," "); strcat(authentication,password); // put the username and password together for authentication
    while(1)
    {
    	connectCheck();
    	int authorized = 0; // will = 4 if all running servers authorize user
    	
        /* zero out the buffer before  */
        bzero(buf, BUFSIZE);
        /* user menu options */
        printf("\nPlease choose from the following options\n");
        printf("Enter your selection in the indicated format\n\n");
        printf("get [filename]: Download a File From the Server\n");
        printf("put [filename]: Upload a File to the Server\n");
        printf("ls : List Files\n");
        printf("exit : Shut Down Server and Exit Client Interface\n");
        fgets(buf, BUFSIZE, stdin);

        /* program exit */
        if (strcmp(buf,"exit\n") == 0)
        {
            printf("Bye Young <3\n");
            exit(0);
        }


        else
        {
        	// Authenticating
        	for(int j = 0; j < 4; j++)
        	{
	        	if (flags[j] != 0) // if this is a working socket
	        	{
	        		printf("\nrecognized socket\n");
	        		printf("writing %s to server\n",authentication);
	        		n = write(sockets[j],authentication,strlen(authentication)); // WRITE 1 - write username and password to server
	        		bzero(readbuf, BUFSIZE);
	        		n = read(sockets[j],readbuf,BUFSIZE); // READ 1 - authentication status
	        		printf("%d\n",n);
	        		if (n == 17)
	        			authorized ++;
	        	}
	        	else
	        	{
	        		printf("no socket\n");
	        		authorized ++;
	        	}
	        }
        	if(authorized == 4)
        	{
        		printf("You are an Authorized User\n");
        		strcpy(writebuf,buf);
        		for (int i = 0; i < 4; i++)
        		{
        			if (flags[i] == 1)
        			{
        				printf("Sending %s to server%d\n", writebuf,i);
        				write(sockets[i], writebuf, strlen(writebuf)); // WRITE 2 - Command
        			}
        		}
	        	if (strcmp(buf,"ls\n") == 0)
		        {
		        	printf("List Path");
		        }
		        else
		        {
		        	command = strtok(buf," ");
		        	filename = strtok(NULL,"\n");
		        	if (strcmp(command,"put") == 0)
		    		{
		        		//MD5 Hash
		        		unsigned char c[MD5_DIGEST_LENGTH];
					    FILE *inFile = fopen (filename, "rb");
					    if (inFile == NULL) 
					    {
					        printf ("%s can't be opened.\n", filename);
					    }
					    MD5_CTX mdContext;
					    unsigned char data[1024];
					    char md5value[1024];
					    MD5_Init (&mdContext);
					    int bytes;
					    int totalBytes = 0;
					    while ((bytes = fread (data, 1, 1024, inFile)) != 0)
					    {
					        MD5_Update (&mdContext, data, bytes);
					        totalBytes += bytes;
					    }
					    MD5_Final (c,&mdContext);
					    int i;
					    for(i = 0; i < MD5_DIGEST_LENGTH; i++) 
					    {
					    	sprintf(&md5value[i*2],"%02x",(unsigned int)c[i]);
					    }
					    int x;
					    x = atoi(md5value)%4;
					    printf("hash value %s\ntotal bytes %d\nx = %d\n",md5value,totalBytes,x);

					    int quarter = totalBytes/4;
					    printf("%d\n",quarter);
					    rewind(inFile);
					    bzero(data, BUFSIZE);

					    //Time to Put
					    if (x == 0)
					    {
					    	sprintf(pieceInfo, "1 %d\n", quarter);
					    	printf("writing piece %s \n", pieceInfo);
					    	//putting piece 1 onto server 1
					    	if (flags[0] == 1)
					    	{
					    		bzero(buf,BUFSIZE);
					    		printf("Server 1\n");
					    		write(sockets[0],pieceInfo,6); // WRITE 3(0) - pieceInfo
					    		n = read(sockets[0],buf, strlen(pieceInfo)); // READ 2(0) - file was created successfully
					    		printf("n = %d, buf = %s",n,buf);

					    		
					    		totalBytes = 0;
					    		while (totalBytes < quarter)
					    		{
					    			bzero(data,BUFSIZE);
					    			printf("%d\n",min(totalBytes,quarter));
					    			int w = fread(data, 1, min(totalBytes,quarter), inFile);
					    			printf("w: %d\n", w);
					    			n = write(sockets[0],data,w); // WRITE 4(0) - write data to server


					    			totalBytes += n;
					    			
					    		}
					    	}

					    	
					    	//putting piece 1 onto server 4
					    	if (flags[3] == 1)
					    	{
					    		fseek(inFile,0,SEEK_SET);
					    		bzero(buf,BUFSIZE);
					    		printf("Server 4\n");
					    		write(sockets[3],pieceInfo,6); // WRITE 3(3) - pieceinfo
					    		n = read(sockets[3],buf, 12); // READ 2(3) - file was created successfully
					    		printf("n = %d, buf = %s",n,buf);

					    		
					    		totalBytes = 0;
					    		while (totalBytes < quarter)
					    		{
					    			bzero(data,BUFSIZE);
					    			printf("%d\n",min(totalBytes,quarter));
					    			int w = fread(data, 1, min(totalBytes,quarter), inFile);
					    			printf("w: %d\n", w);
					    			n = write(sockets[3],data,w); // WRITE 4(3) - write data to server


					    			totalBytes += n;
					    			
					    		}
					    	}
					    	
					    	// piece 2
					    	sleep(1);
					    	
					    	printf("putting piece 2 onto server 1\n");
					    	sprintf(pieceInfo, "2 %d\n", quarter);
					    	printf("writing piece %s \n", pieceInfo);
					    	read(sockets[0],trashline,strlen(trashline)); // READ 3 - waiting to hear from server that first pieces were written
					    	write(sockets[0],"ready",5); // WRITE 5 - tell server we are ready to send
					    	if (flags[0] == 1)
					    	{
					    		bzero(buf,BUFSIZE);
					    		printf("Server 1 sending %s\n", pieceInfo);
					    		write(sockets[0],pieceInfo,6); // WRITE 6 - send the second piece information
					    		n = read(sockets[0],buf, strlen(pieceInfo)); // READ 4 - file creation
					    		printf("n = %d, buf = %s",n,buf);

					    		
					    		totalBytes = 0;
					    		while (totalBytes < quarter)
					    		{
					    			bzero(data,BUFSIZE);
					    			printf("%d\n",min(totalBytes,quarter));
					    			int w = fread(data, 1, min(totalBytes,quarter), inFile);
					    			printf("w: %d\n", w);
					    			n = write(sockets[0],data,w);


					    			totalBytes += n;
					    			
					    		}
					    	}
					    	
					    	// putting piece 2 onto 2
					    	if (flags[1] == 1)
					    	{
					    		fseek(inFile,-quarter,SEEK_CUR);
					    		bzero(buf,BUFSIZE);
					    		printf("Server 4\n");
					    		write(sockets[1],pieceInfo,6);
					    		n = read(sockets[1],buf, strlen(pieceInfo));
					    		printf("n = %d, buf = %s",n,buf);

					    		
					    		totalBytes = 0;
					    		while (totalBytes < quarter)
					    		{
					    			bzero(data,BUFSIZE);
					    			printf("%d\n",min(totalBytes,quarter));
					    			int w = fread(data, 1, min(totalBytes,quarter), inFile);
					    			printf("w: %d\n", w);
					    			n = write(sockets[1],data,w);


					    			totalBytes += n;
					    			
					    		}
					    	}

					    	// piece 3
					    	sleep(1);
					    	
					    	printf("putting piece 3 onto server 2\n");
					    	sprintf(pieceInfo, "3 %d\n", quarter);
					    	printf("writing piece %s \n", pieceInfo);
					    	read(sockets[1],trashline,strlen(trashline)); // READ 3 - waiting to hear from server that first pieces were written
					    	write(sockets[1],"ready",5); // WRITE 5 - tell server we are ready to send
					    	if (flags[1] == 1)
					    	{
					    		bzero(buf,BUFSIZE);
					    		printf("Server 1 sending %s\n", pieceInfo);
					    		write(sockets[1],pieceInfo,6); // WRITE 6 - send the second piece information
					    		n = read(sockets[1],buf, strlen(pieceInfo)); // READ 4 - file creation
					    		printf("n = %d, buf = %s",n,buf);

					    		
					    		totalBytes = 0;
					    		while (totalBytes < quarter)
					    		{
					    			bzero(data,BUFSIZE);
					    			printf("%d\n",min(totalBytes,quarter));
					    			int w = fread(data, 1, min(totalBytes,quarter), inFile);
					    			printf("w: %d\n", w);
					    			n = write(sockets[1],data,w);


					    			totalBytes += n;
					    			
					    		}
					    	}
					    	
					    	// putting piece 3 onto 3
					    	if (flags[2] == 1)
					    	{
					    		fseek(inFile,-quarter,SEEK_CUR);
					    		bzero(buf,BUFSIZE);
					    		printf("Server 4\n");
					    		write(sockets[2],pieceInfo,6);
					    		n = read(sockets[2],buf, strlen(pieceInfo));
					    		printf("n = %d, buf = %s",n,buf);

					    		
					    		totalBytes = 0;
					    		while (totalBytes < quarter)
					    		{
					    			bzero(data,BUFSIZE);
					    			printf("%d\n",min(totalBytes,quarter));
					    			int w = fread(data, 1, min(totalBytes,quarter), inFile);
					    			printf("w: %d\n", w);
					    			n = write(sockets[2],data,w);


					    			totalBytes += n;
					    			
					    		}
					    	}

					    	sleep(1);
					    	
					    	printf("putting piece 4 onto server 3\n");
					    	sprintf(pieceInfo, "4 %d\n", quarter);
					    	printf("writing piece %s \n", pieceInfo);
					    	read(sockets[2],trashline,strlen(trashline)); // READ 3 - waiting to hear from server that first pieces were written
					    	write(sockets[2],"ready",5); // WRITE 5 - tell server we are ready to send
					    	if (flags[2] == 1)
					    	{
					    		bzero(buf,BUFSIZE);
					    		printf("Server 1 sending %s\n", pieceInfo);
					    		write(sockets[2],pieceInfo,6); // WRITE 6 - send the second piece information
					    		n = read(sockets[2],buf, strlen(pieceInfo)); // READ 4 - file creation
					    		printf("n = %d, buf = %s",n,buf);

					    		
					    		totalBytes = 0;
					    		while (totalBytes < quarter)
					    		{
					    			bzero(data,BUFSIZE);
					    			printf("%d\n",min(totalBytes,quarter));
					    			int w = fread(data, 1, min(totalBytes,quarter), inFile);
					    			printf("w: %d\n", w);
					    			n = write(sockets[2],data,w);


					    			totalBytes += n;
					    			
					    		}
					    	}
					    	
					    	// putting piece 4 onto 4
					    	if (flags[3] == 1)
					    	{
					    		fseek(inFile,-quarter,SEEK_CUR);
					    		bzero(buf,BUFSIZE);
					    		sprintf(pieceInfo, "4 %d\n", quarter);
					    		printf("Server 4 sending %s\n",pieceInfo);
					    		write(sockets[3],pieceInfo,6);
					    		n = read(sockets[3],buf, strlen(pieceInfo));
					    		printf("n = %d, buf = %s",n,buf);

					    		
					    		totalBytes = 0;
					    		while (totalBytes < quarter)
					    		{
					    			bzero(data,BUFSIZE);
					    			printf("%d\n",min(totalBytes,quarter));
					    			int w = fread(data, 1, min(totalBytes,quarter), inFile);
					    			printf("w: %d\n", w);
					    			n = write(sockets[3],data,w);


					    			totalBytes += n;
					    			
					    		}
					    	}
					    	fclose(inFile);
					    }
					}
				}
			}
		
					   		
					    	/*
					    	totalBytes = 0;
					    	while (totalBytes < quarter)
					    	{
					    		totalBytes += fread(data, 1, 1024, inFile);
					    		if (flags[0] == 1)
					    		{
					    			piece = "2";
					    			write(sockets[0],piece,strlen(piece));
					    			//write(sockets[0],data,strlen(data));
					    		}
					    		if (flags[1] == 1)
					    		{	
					    			piece = "2";
					    			write(sockets[1],piece,strlen(piece));
					    			write(sockets[1],data,strlen(data));
					    		}
					    	}
					    	totalBytes = 0;
					    	while (totalBytes < quarter)
					    	{
					    		totalBytes += fread(data, 1, 1024, inFile);
					    		if (flags[1] == 1)
					    		{
					    			piece = "3";
					    			write(sockets[1],piece,strlen(piece));
					    			write(sockets[0],data,strlen(data));
					    		}
					    		if (flags[2] == 1)
					    		{	
					    			piece = "3";
					    			write(sockets[2],piece,strlen(piece));
					    			write(sockets[2],data,strlen(data));
					    		}
					    	}
					    	totalBytes = 0;
					    	while (totalBytes < quarter)
					    	{
					    		totalBytes += fread(data, 1, 1024, inFile);
					    		if (flags[2] == 1)
					    		{
					    			piece = "4";
					    			write(sockets[2],piece,strlen(piece));
					    			write(sockets[2],data,strlen(data));
					    		}
					    		if (flags[3] == 1)
					    		{	
					    			piece = "4";
					    			write(sockets[3],piece,strlen(piece));
					    			write(sockets[3],data,strlen(data));
					    		}
					    	}
					    	
					    }
					    
					    else if (x == 1)
					    {
					    	totalBytes = 0;
					    	while (totalBytes < quarter)
					    	{
					    		totalBytes += fread(data, 1, 1024, inFile);
					    		if (flags[0] == 1)
					    			write(sockets[0],data,strlen(data));
					    		if (flags[1] == 1)
					    			write(sockets[1],data,strlen(data));
					    	}
					    	totalBytes = 0;
					    	while (totalBytes < quarter)
					    	{
					    		totalBytes += fread(data, 1, 1024, inFile);
					    		if (flags[1] == 1)
					    			write(sockets[1],data,strlen(data));
					    		if (flags[2] == 1)
					    			write(sockets[2],data,strlen(data));
					    	}
					    	totalBytes = 0;
					    	while (totalBytes < quarter)
					    	{
					    		totalBytes += fread(data, 1, 1024, inFile);
					    		if (flags[2] == 1)
					    			write(sockets[2],data,strlen(data));
					    		if (flags[3] == 1)
					    			write(sockets[3],data,strlen(data));
					    	}
					    	totalBytes = 0;
					    	while (fread(data,1,1024,inFile) != 0)
					    	{
					    		if (flags[3] == 1)
					    			write(sockets[3],data,strlen(data));
					    		if (flags[0] == 1)
					    			write(sockets[0],data,strlen(data));
					    	}
					    }
					    else if (x == 2)
					    {
					    	totalBytes = 0;
					    	while (totalBytes < quarter)
					    	{
					    		totalBytes += fread(data, 1, 1024, inFile);
					    		if (flags[1] == 1)
					    			write(sockets[1],data,strlen(data));
					    		if (flags[2] == 1)
					    			write(sockets[2],data,strlen(data));
					    	}
					    	totalBytes = 0;
					    	while (totalBytes < quarter)
					    	{
					    		totalBytes += fread(data, 1, 1024, inFile);
					    		if (flags[2] == 1)
					    			write(sockets[2],data,strlen(data));
					    		if (flags[3] == 1)
					    			write(sockets[3],data,strlen(data));
					    	}
					    	totalBytes = 0;
					    	while (totalBytes < quarter)
					    	{
					    		totalBytes += fread(data, 1, 1024, inFile);
					    		if (flags[3] == 1)
					    			write(sockets[3],data,strlen(data));
					    		if (flags[0] == 1)
					    			write(sockets[0],data,strlen(data));
					    	}
					    	totalBytes = 0;
					    	while (fread(data,1,1024,inFile) != 0)
					    	{
					    		if (flags[0] == 1)
					    			write(sockets[0],data,strlen(data));
					    		if (flags[1] == 1)
					    			write(sockets[1],data,strlen(data));
					    	}
					    }
					    else if (x == 3)
					    {
					    	totalBytes = 0;
					    	while (totalBytes < quarter)
					    	{
					    		totalBytes += fread(data, 1, 1024, inFile);
					    		if (flags[2] == 1)
					    			write(sockets[3],data,strlen(data));
					    		if (flags[3] == 1)
					    			write(sockets[3],data,strlen(data));
					    	}
					    	totalBytes = 0;
					    	while (totalBytes < quarter)
					    	{
					    		totalBytes += fread(data, 1, 1024, inFile);
					    		if (flags[3] == 1)
					    			write(sockets[3],data,strlen(data));
					    		if (flags[0] == 1)
					    			write(sockets[0],data,strlen(data));
					    	}
					    	totalBytes = 0;
					    	while (totalBytes < quarter)
					    	{
					    		totalBytes += fread(data, 1, 1024, inFile);
					    		if (flags[0] == 1)
					    			write(sockets[0],data,strlen(data));
					    		if (flags[1] == 1)
					    			write(sockets[1],data,strlen(data));
					    	}
					    	totalBytes = 0;
					    	while (fread(data,1,1024,inFile) != 0)
					    	{
					    		if (flags[1] == 1)
					    			write(sockets[1],data,strlen(data));
					    		if (flags[2] == 1)
					    			write(sockets[2],data,strlen(data));
					    	}
					    }
					    
					    else 
					    	printf("Mod ERROR\n");




					    
		        		write(sockets[0],writebuf,strlen(writebuf));
		        		bzero(readbuf,BUFSIZE);
		        		read(sockfd, readbuf, strlen(readbuf));
		        		printf("%s\n",readbuf);
		        		

		        		fclose (inFile);
		        	}
		        }
		    }*/
		    else
		    {
		    	printf("Invalid Username/Password. Please Try Again\n");
		    }
		}
	}

		
	    
	    
		/*
	    // check command line arguments
	    if (argc != 3) {
	       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
	       exit(0);
	    }
	    hostname = argv[1];
	    portno = atoi(argv[2]

	    // build the server's Internet address
	   
	    serveraddr.sin_family = AF_INET;
	    bcopy((char *)server->h_addr, 
		  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
	    
	    serverlen = sizeof(serveraddr);
	    */
}
    