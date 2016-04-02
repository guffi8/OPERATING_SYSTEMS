#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <algorithm>
#include "ftputil.h"

using namespace std;

#define PARAMETER_NUM 5
#define SERVER_PORT 1
#define SERVER_HOST_NAME 2
#define FILE_TO_TRANSFER 3
#define FILENAME_IN_SERVER 4

#define USAGE_ERROR "Usage: clftp server-port server-hostname file-to-transfer filename-in-server"
#define FILE_TOO_BIG "Transmission failed: too big file"

/**
 * return 0 if the given path is a directory, -1 otherwise
 */
int isDirectory(char const* path) {
	DIR *dir= opendir(path);
	if(dir)
	{
		closedir(dir);
		return 0;
	}
	return -1;
}

/**
 * The main function. Gets the port number, the server's host name, the file to transfer and 
 * the name of the file in the server. Open connection with the server and send the file 
 * according to our protocol.
 */
int main(int argc, char* argv[])
{
	// checking input validation
	if(argc != PARAMETER_NUM || isDirectory(argv[FILE_TO_TRANSFER]) == 0)
	{
		cout << USAGE_ERROR << endl;
		exit(1);
	}

	// ----------------- open the file-to-transmit -----------------

	struct stat stats;
	if(stat(argv[FILE_TO_TRANSFER], &stats) < 0)
	{
		closingConnection(-1, (char*)"stat");
	}

	// open the file and get all the info from it
	int fd;
	if((fd = open(argv[FILE_TO_TRANSFER], O_RDONLY)) < 0)
	{
		closingConnection(-1, (char*)"open");
	}

	// ------------ open the connection with the server ------------
	int clientSocket;
	struct sockaddr_in server;
	struct hostent *hp;

	// getting the server address
	if((hp = gethostbyname(argv[SERVER_HOST_NAME])) == NULL)
	{
		closingConnection(fd, (char*)"gethostbyname");
	}

	server.sin_family = hp->h_addrtype;
	memcpy((char*) &server.sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
	server.sin_port = htons((u_short)atoi(argv[SERVER_PORT]));
	memset(server.sin_zero, 0, 8);

	// creating a socket
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(clientSocket < 0)
	{
		closingConnection(fd, (char*)"socket");
	}

	// connecting to the server
	int connectRes = connect(clientSocket, (struct sockaddr *)&server , sizeof(server));
	if(connectRes < 0)
	{
		close(clientSocket);
		closingConnection(fd, (char*)"connect");
	}

	int canTransfer;

	// sending the size of file
	if(sending(clientSocket, (char*)&stats.st_size, sizeof(stats.st_size)) < 0)
	{
		close(clientSocket);
		closingConnection(fd, (char*)"send");
	}

	if(recevied(clientSocket, (char*)&canTransfer, sizeof(int)) < 0)
	{
		close(clientSocket);
		closingConnection(fd, (char*)"recv");
	}

	// The file-to-transfer is too big for the server
	if(canTransfer == FILE_SIZE_NOT_OK)
	{
		cout << FILE_TOO_BIG << endl;
		close(clientSocket);
		close(fd);
		return 0;
	}

	// sending file name length
	size_t fileNameLen = strlen(argv[FILENAME_IN_SERVER]) + 1;
	if(sending(clientSocket, (char*)&fileNameLen, sizeof(size_t)) < 0)
	{
		close(clientSocket);
		closingConnection(fd, (char*)"send");
	}

	// sending the file name
	if(sending(clientSocket, argv[FILENAME_IN_SERVER], fileNameLen) < 0)
	{
		close(clientSocket);
		closingConnection(fd, (char*)"send");
	}

	// waiting for approval
	int legalName = FILE_NAME_NOT_OK;
	if(recevied(clientSocket, (char*)&legalName, sizeof(int)) < 0)
	{
		close(clientSocket);
		closingConnection(fd, (char*)"recv");
	}

	if(legalName == FILE_NAME_NOT_OK)
	{
		close(clientSocket);
		closingConnection(fd, NULL);
	}

	int alreadyBuff = 0;
	int read;
	int sent;

	int currRead;

	char buffer[BUFFER_SIZE];

	// sending all the file's content in BUFFER_SIZE packages
	while((read = pread(fd, buffer, BUFFER_SIZE, alreadyBuff)) > 0)
	{
		currRead = read;

		while(currRead > 0)
		{
			if((sent = sending(clientSocket, buffer, currRead)) < 0)
			{
				close(clientSocket);
				closingConnection(fd, (char*)"send");
			}
			currRead -= sent;
		}

		alreadyBuff += read;

	}

	if(read < 0)
	{
		close(clientSocket);
		closingConnection(fd, (char*)"pread");
	}
	
	close(fd);
	close(clientSocket);
	return 0;
}
