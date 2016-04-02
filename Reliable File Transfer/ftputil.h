#ifndef FTPUTIL_H_
#define FTPUTIL_H_

#define SYSTEM_LIBRARY_ERROR(x) "Error: function " << x << " errno: " << errno << endl
#define BUFFER_SIZE 4096

#define FILE_SIZE_OK 1
#define FILE_SIZE_NOT_OK 0

#define FILE_NAME_OK 1
#define FILE_NAME_NOT_OK 0

/**
 * Get the socket, the buffer and the size and received from the socket data in size of 'len'
 * to the given buffer 
 */
int recevied(int sock, char* buffer, size_t len);

/**
 * Get the socket, the buffer and the size and send the data in the 'len' bytes from the
 * buffer to the socket 
 */
int sending(int sock, char* buffer, size_t len);

/**
 * gets fd and name of function, close the fd, print an error according to the given function name
 * and exit the program.
 */ 
void closingConnection(int fdToClose, char* systemErrFunc);

#endif /* FTPUTIL_H_ */
