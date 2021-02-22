#ifndef SOCK_H_INCLUDED
#define SOCK_H_INCLUDED

#ifdef WIN32
    #include <winsock2.h>
    typedef SOCKADDR_IN sockaddr_in;
#else // posix
    #include <netinet/in.h> // sockaddr_in
    typedef int SOCKET;
#endif

//#define ANY_PORT 0
typedef unsigned long IP; // sockaddr_in::sin_addr.s_addr defines it as "unsigned long"


class Sock { // TCP socket
	SOCKET s;
	sockaddr_in ip;	// ip.sin_port can be used to find the server port if listen(ANY_PORT) was used
	char peek;      // used in readLine()
public:
    constexpr const static unsigned short ANY_PORT = 0;
	Sock();
	Sock(SOCKET socket); // wrap a socket in the Sock class
	~Sock();             // calls close()

	int conn(const char * ip, int port); // connect to remote server
	int close(void);

	int read(char * buffer, int size);
	int readLine(char* buffer);
	int write(const char * buffer, int size);

	int listen(unsigned short port = ANY_PORT); // server - start listening for a connection
	SOCKET accept(); // server - accept an incoming connection and return a client socket
	int getPort(){ return ip.sin_port; } // bound server port
};

#endif // SOCK_H_INCLUDED
