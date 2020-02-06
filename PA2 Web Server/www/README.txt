INCLUDED:
./server.c - c program for server
makefile - file used to build server.c

HOW TO RUN:
From the command line navigate to the appropriate folder and type 'make' to compile the program.o run the server after making type ./server [portnumber] from the command line where [portnumber] is the portnumber that you want the server connected to.

NOTES AND PROCESS:
Server begins by opening a port and creating a thread as per provided code. My implimentation was done inside of the echo function. First the server parses the incoming GET command and filename. If the filename is blank or a / the server will default to index.html otherwise I use file++ to skip the initial / because fopen() does not like it! I use the helper function types() to determine what type of file is being requested so that this information can be added to the httpmsg string. I open the requested file, seek to the end of the file and record the position so that I have the size of the file. i then put all of the elements of the httpmsg together using strcat including the file type and the file size. I put the httpmsg into the buffer and send it to the client. Finally i read the contents of the file into the file buffer and send it to the client in packet increments, zeroing the buffer after each transmission. When there is no more data from the file to be sent the while loops terminates and the transmission is finished.
