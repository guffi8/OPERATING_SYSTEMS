#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <algorithm>
#include "ftputil.h"
#include <sys/time.h>

using namespace std;

#define PARAMETER_NUM 3
#define SERVER_PORT 1
#define MAX_FILE_SIZE 2

#define MAX_PANDING_CONNACTION 5

#define USAGE_ERROR "Usage: srftp server-port max-file-size"

off_t maxFileSize;

/**
 *  closing the thread, print the correct error and exit the theard, 
 *  if it receveie a file it close it too. 
 */
void closingThread(int fdToClose, char* systemErrFunc)
{
    if( fdToClose > 0)
    {
        close(fdToClose);
    }

    if(systemErrFunc != NULL)
    {
        cerr << SYSTEM_LIBRARY_ERROR(systemErrFunc);
    }
    pthread_exit(NULL);
}

/**
 *  The connection handler - each client is a theard -> this function
 * 	recevie the client file and handle is request with our protocool.
 */
void* connectionHandler(void* clientSocketDesc)
{

    int sock = *(int*)clientSocketDesc;
    off_t fileSize;
    // receive from the client the file size
    if(recevied(sock, (char*)&fileSize, sizeof(long)) < 0)
    {
        free(clientSocketDesc);
        clientSocketDesc = NULL;
        closingThread(sock, (char*)"revc");
    }
	// a binary flag for the client if the server can deal with this file size
    int canTransfer = FILE_SIZE_NOT_OK;

    if(fileSize <= maxFileSize)
    {
        canTransfer = FILE_SIZE_OK;
    }

     // answer to the client if the server can deal with this file size
    if(sending(sock, (char*)&canTransfer, sizeof(int)) < 0)
    {
    	free(clientSocketDesc);
		clientSocketDesc = NULL;
		closingThread(sock, (char*)"send");
    }

    // can't transfer the file, too big
    if(canTransfer == FILE_SIZE_NOT_OK)
    {
    	free(clientSocketDesc);
		clientSocketDesc = NULL;
		closingThread(sock, NULL);
    }

    // if its OK, receive from the client the file name, open new file in 
    //that name, and write to it the content that received from the client

    // received the length of the file name
    size_t fileNameLen;
    if(recevied(sock, (char*)&fileNameLen, sizeof(long)) < 0)
	{
		free(clientSocketDesc);
		clientSocketDesc = NULL;
		closingThread(sock, (char*)"revc");
	}

	// received the file name
    char fileName[PATH_MAX];
    if(recevied(sock, fileName, fileNameLen) < 0)
    {
    	free(clientSocketDesc);
		clientSocketDesc = NULL;
		closingThread(sock, (char*)"revc");
    }

    int fd;
    int legalName = FILE_NAME_OK;
    // open the file that we write to - if it fail send it to the client
    if((fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
    {
    	free(clientSocketDesc);
		clientSocketDesc = NULL;
		legalName = FILE_NAME_NOT_OK;
		if(sending(sock, (char*)&legalName, sizeof(int)) < 0)
		{
			free(clientSocketDesc);
			clientSocketDesc = NULL;
			closingThread(sock, (char*)"send");
		}

		closingThread(sock, (char*)"open");
    }
	// send the client that the given name is legal and open with no problems
    if(sending(sock, (char*)&legalName, sizeof(int)) < 0)
	{
		free(clientSocketDesc);
		clientSocketDesc = NULL;
		closingThread(sock, (char*)"send");
	}

    int writen;
    off_t alreadyBuff = 0;
    int sent = 0 ;
    int currWrite;
    int currSent;
    char buffer[BUFFER_SIZE];
	// received the file data
    while((sent = recevied(sock, buffer, BUFFER_SIZE)) > 0)
    {
		currSent = sent;
		currWrite = 0;
		while(currSent > 0)
		{
			if((writen = pwrite(fd, buffer + currWrite, currSent, alreadyBuff)) < 0)
			{
				free(clientSocketDesc);
				clientSocketDesc = NULL;
				closingThread(sock, (char*)"pwrite");
			}
			currWrite += writen;
			currSent -= writen;
		}

        alreadyBuff += sent;
       
    }
    if(sent < 0)
	{
		free(clientSocketDesc);
		clientSocketDesc = NULL;
		closingThread(sock, (char*)"revc");
	}

    free(clientSocketDesc);
    clientSocketDesc = NULL;
    return NULL;
}

/**
 * create the server - recevie it port and max size it can handle with,
 * the server opened and wait for connection, if an error occur on the way
 * the correct error massage is printed. 
 */
int main(int argc, char* argv[])
{
    // checking input validation
    if(argc != PARAMETER_NUM || atoi(argv[MAX_FILE_SIZE]) < 0)
    {
        cout << USAGE_ERROR << endl;
        exit(1);
    }

    maxFileSize = atol(argv[MAX_FILE_SIZE]);

    int socketDesc, clientSocketDesc, *newSocket;
    struct sockaddr_in server/*, client*/;
    struct hostent* host;
    char hostName[HOST_NAME_MAX];

    // preparing the server
    memset(&server, 0, sizeof(server));
    if(gethostname(hostName,HOST_NAME_MAX) < 0)
    {
        closingConnection(-1, (char*)"gethostname");
    }
    if((host = gethostbyname(hostName)) == NULL)
    {
        closingConnection(-1, (char*)"gethostbyname");
    }

    memcpy((char*)&server.sin_addr, host->h_addr, host->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons((u_short)atoi(argv[SERVER_PORT]));

    // creating the file descriptor of the server socket
    socketDesc = socket(AF_INET, SOCK_STREAM, 0);
    if(socketDesc < 0)
    {
        closingConnection(-1, (char*)"socket");
    }

    // bind
    int bindRes = bind(socketDesc, (struct sockaddr*) &server, sizeof(struct sockaddr_in));
    if(bindRes < 0)
    {
        closingConnection(socketDesc, (char*)"bind");
    }


    // start listening
    listen(socketDesc, MAX_PANDING_CONNACTION);

    // accept connections
    while((clientSocketDesc = accept(socketDesc, NULL, NULL)))
    {
        // creating thread
        pthread_t clientThread;
        newSocket = (int*)malloc(sizeof(int));
        *newSocket = clientSocketDesc;
        if(pthread_create(&clientThread, NULL, connectionHandler, (void*) newSocket) < 0)
        {
            free(newSocket);
            newSocket = NULL;
            cerr << "Error: function pthread_create errno: " << endl; 
            closingConnection(socketDesc, NULL);
        }
        pthread_detach(clientThread);

    }

    // check if connect function failed
    if(clientSocketDesc < 0)
    {
    	closingConnection(socketDesc, (char*)"connect");
    }

    return 0;
}
