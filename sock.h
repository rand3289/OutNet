#ifndef SOCK_H_INCLUDED
#define SOCK_H_INCLUDED

#include <string>
#ifdef WIN32
    #include <winsock2.h>
    typedef SOCKADDR_IN sockaddr_in;
#else // posix
    #include <netinet/in.h> // sockaddr_in
    #define INVALID_SOCKET (-1)
    typedef int SOCKET;
#endif

// typedef uint32_t IPADDR; // but sockaddr_in::sin_addr.s_addr is defined as "unsigned long"
// typedef uint16_t IPPORT;

class Sock { // TCP socket
	SOCKET s;
	sockaddr_in ip;	// ip.sin_port can be used to find the server port if listen(ANY_PORT) was used
public:
	Sock();
	Sock(SOCKET socket); // wrap an accepted connected socket in the Sock class
	~Sock();             // calls close()

	int connect(const char * ip,  uint16_t port); // connect to remote server
    int connect(uint32_t ip, uint16_t port); // ip has to be in the network byte order???
    int setRWtimeout(int seconds); // set read() and write() timeouts
	int close(void);

	int read(void* buffer, size_t size);
	int readLine(void* buffer, const size_t size);
	int readString(void* buff, const size_t size);
	int readString(std::string& str);
	uint16_t read16(bool& error);
	uint32_t read32(bool& error);

	int write(const void* buffer, size_t size);
	int write32(uint32_t data);
	int write16(uint16_t data);
	int writeString(const std::string& str);

    // server - start listening for a connection.  Use ANY_PORT if not binding to a specific port
    constexpr const static uint16_t ANY_PORT = 0;
	int listen(uint16_t port);
	// accept an incoming connection on a listening server socket and return a client socket
	SOCKET accept();
	int accept(Sock& connection); // recommended way of accepting connection

	// bound server port even if ANY_PORT was used in listen() or remote port after accept() or connect()
	uint16_t getPort() const { return ntohs(ip.sin_port); }
	// remote IP after connect() or after object was returned by accept()
	uint32_t getIP() const { return ip.sin_addr.s_addr; } 
	SOCKET getRawSocket() const { return s; }

	static uint32_t strToIP(const char* addr);
	static std::string ipToString(uint32_t ip);
	static bool isRoutable(uint32_t ip);
};

#endif // SOCK_H_INCLUDED
