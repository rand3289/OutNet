#ifndef SOCK_H_INCLUDED
#define SOCK_H_NCLUDED

#include <windows.h>

class Sock {
	WSADATA wsd;
	SOCKET s;
	SOCKADDR_IN ip;	// used for a server
	char peek; // used in readLine()
public:

	int conn(const char * ip, int port);
	int close(void);

	int read(char * buffer, int size);
	int readLine(char* buffer);
	int write(const char * buffer, int size);

	int listen(unsigned short port);	// server - start listening for a connection
	SOCKET accept();					// server - accept an incoming connection and return a client socket
	Sock(SOCKET client_socket);			// server - wrap a client socket in the Sock class.

	Sock();
	~Sock(); // calls close
};

#endif // !defined(SOCK_H_INCLUDED)
