#ifndef SOCK_H_INCLUDED
#define SOCK_H_INCLUDED

#ifdef WIN32
    #include <winsock2.h>
    typedef SOCKADDR_IN sockaddr_in;
#else // posix
    #include <netinet/in.h> // sockaddr_in
    #define INVALID_SOCKET (-1)
    typedef int SOCKET;
#endif

typedef unsigned long IPADDR; // sockaddr_in::sin_addr.s_addr defines it as "unsigned long"
typedef uint16_t IPPORT;


class Sock { // TCP socket
	SOCKET s;
	sockaddr_in ip;	// ip.sin_port can be used to find the server port if listen(ANY_PORT) was used
	char peek;      // used in readLine()
public:
	Sock();
	Sock(SOCKET socket); // wrap an accepted connection socket in the Sock class
	~Sock();             // calls close()

	int conn(const char * ip,  unsigned short port); // connect to remote server
    int conn(unsigned long ip, unsigned short port); // ip has to be in the network byte order!!!
	int close(void);

	int read(char * buffer, int size);
	int readLine(char* buffer);
	int write(const char * buffer, int size);

    // server - start listening for a connection.  Use ANY_PORT if not binding to a specific port
    constexpr const static unsigned short ANY_PORT = 0;
	int listen(unsigned short port);
	// server - accept an incoming connection and return a client socket
	SOCKET accept();
	int accept(Sock& connection);
	int getBoundPort(){ return ip.sin_port; } // bound server port even if ANY_PORT was used in listen()
	SOCKET getRawSocket() { return s; }
};

#endif // SOCK_H_INCLUDED
