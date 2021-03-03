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

// typedef unsigned long IPADDR; // sockaddr_in::sin_addr.s_addr defines it as "unsigned long"
// typedef uint16_t IPPORT;


// TODO: rewrite Sock class' read() / read*() / write() to accept timeout
class Sock { // TCP socket
	SOCKET s;
	sockaddr_in ip;	// ip.sin_port can be used to find the server port if listen(ANY_PORT) was used
public:
	Sock();
	Sock(SOCKET socket); // wrap an accepted connected socket in the Sock class
	~Sock();             // calls close()

	int connect(const char * ip,  unsigned short port); // connect to remote server
    int connect(unsigned long ip, unsigned short port); // ip has to be in the network byte order???
	int close(void);

	int read(char * buffer, int size);
	int readLine(char* buffer, int size);
	short readShort(bool& error);
	long readLong(bool& error);
	int write(const char * buffer, int size);

    // server - start listening for a connection.  Use ANY_PORT if not binding to a specific port
    constexpr const static unsigned short ANY_PORT = 0;
	int listen(unsigned short port);
	// accept an incoming connection on a listening server socket and return a client socket
	SOCKET accept();
	int accept(Sock& connection); // recommended way of accepting connection
	// bound server port even if ANY_PORT was used in listen() or remote port after accept() or connect()
	unsigned short getPort(){ return ntohs(ip.sin_port); }
	// remote IP after connect() or after object was returned by accept()
	unsigned long  getIP()  { return ntohl(ip.sin_addr.s_addr); } 
	SOCKET getRawSocket()   { return s; }
};

#endif // SOCK_H_INCLUDED
