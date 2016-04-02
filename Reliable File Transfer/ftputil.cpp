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

void closingConnection(int fdToClose, char* systemErrFunc)
{
    if( fdToClose > 0)
    {
        close(fdToClose);
    }

    if(systemErrFunc != NULL)
    {
        cerr << SYSTEM_LIBRARY_ERROR(systemErrFunc);
    }
    exit(1);
}

int recevied(int sock, char* buffer, size_t len)
{
	size_t left = 0;
	int rec;
	while(left < len)
	{
		rec = recv(sock, buffer + left , len - left, 0);
		if(rec < 0)
		{
			return rec;
		}
		if(rec == 0)
		{
			break;
		}
		left += rec;
	}

	return left;
}

int sending(int sock, char* buffer, size_t len)
{
	size_t left = 0;
	int sent;
	while(left < len)
	{
		sent = send(sock, buffer + left, len, 0);
		if(sent < 0)
		{
			return sent;
		}
		if(sent == 0)
		{
			break;
		}
		left += sent;
	}

	return left;
}


