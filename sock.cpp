#include "sock.h"
#include <iostream> // cerr
#include <cstring>  // memset()
using namespace std;

#ifdef WIN32
    #pragma comment(lib, "Ws2_32.lib") // link to winsock2 library
    #define ::close(s) closesocket(s)
//    #define UNREACHABLE WSAECONNRESET
    #undef errno // do not use stdlib version in this file
    #define errno WSAGetLastError()
    typedef int socklen_t;
    typedef SOCKADDR sockaddr;
#else // posix
    #include <unistd.h> // close()
    #include <netdb.h> // gethostbyname()
    #include <sys/types.h>
    #include <sys/socket.h>
    #define SOCKET_ERROR   (-1)
//    #define UNREACHABLE    ECONNREFUSED
#endif


void initNetwork(){
#ifdef WIN32
    static bool initialized = false;
    if(initialized){ return; }
    initialized = true;
    WSADATA wsaData;
    if(WSAStartup(0x0202,&wsd)){
        cerr << "Error upon WSAStartup()" << endl;
    }
#endif
}


// ip has to be in the network byte order!!!
int Sock::connect(unsigned long ipaddr, unsigned short port){
 	s=socket(AF_INET,SOCK_STREAM,0);
	if(s==INVALID_SOCKET){
		cerr << "Error creating socket." << endl;
		return -3;
	}

	ip.sin_family      = AF_INET;
	ip.sin_port        = htons(port);
	ip.sin_addr.s_addr = ipaddr;

	if( ::connect(s, (sockaddr*)&ip, sizeof(ip) ) ){
		int err = errno;
		cerr << "error connecting to remote host via TCP: " << strerror(err) << " (" << err << ")" << endl;
		return err; // errno returns positive numbers
	}
	return 0;
}


unsigned long Sock::strToIP(const char* addr){
    hostent* ipent=gethostbyname(addr);
	if(!ipent){
		cerr << "Can't gethostbyname()" << endl;
		return 0;
	}
	unsigned long ip = *(unsigned long*) ipent->h_addr;  // h_addr is a macro for h_addr_list[0]
	if(ip == INADDR_NONE){
		cerr << "gethostbyname() returned INADDR_NONE" << endl;
		return 0;
	}
	return ip;
}


int Sock::connect(const char* addr, unsigned short port){
	unsigned long ip = strToIP(addr);
	return 0==ip ? INVALID_SOCKET : connect(ip, port);
}


SOCKET Sock::accept(){
	sockaddr_in ipr;	// used for a server
	socklen_t size = sizeof(ipr);
	SOCKET serv = ::accept(s, (sockaddr*)&ipr, &size);
	if(INVALID_SOCKET == serv ){
		int err = errno;
		cerr << "error accepting connection: " << strerror(err) << " (" << err << ")" << endl;
	}
	return serv;
}


SOCKET Sock::accept(Sock& conn){
	socklen_t size = sizeof(conn.ip);
	conn.s = ::accept(s, (sockaddr*)&conn.ip, &size);
	if(INVALID_SOCKET == conn.s ){
		int err = errno;
		cerr << "error accepting connection: " << strerror(err) << " (" << err << ")" << endl;
	}
	return conn.s;
}


int Sock::listen(unsigned short port){
 	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s==INVALID_SOCKET){
		int err = errno;
		cerr << "Error creating socket: " << strerror(err) << " (" << err << ")" << endl;
		return INVALID_SOCKET;
	}

    int reuse = 1; // allow binding to a port if previous socket is lingering
    if ( 0 > setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) ){
		int err = errno;
        cerr << "Error in setsockopt(SO_REUSEADDR): "<< strerror(err) << " (" << err << ")" << endl;
		return INVALID_SOCKET;
    }

	unsigned int size = sizeof(ip);
	ip.sin_family=AF_INET;
	ip.sin_port=htons(port);
	ip.sin_addr.s_addr = INADDR_ANY;

	if ( SOCKET_ERROR==bind(s, (sockaddr*) &ip, size)){
		int err = errno;
		cerr << "error binding to port " << port << ": " << strerror(err) << " (" << err << ")" << endl;
		return INVALID_SOCKET;
	}
	if ( SOCKET_ERROR==getsockname(s, (sockaddr*) &ip, (socklen_t*)&size)){
		int err = errno;
		cerr << "error getting bound socket information: " << strerror(err) << " (" << err << ")" << endl;
		return INVALID_SOCKET;
	}

	if( SOCKET_ERROR == ::listen(s, SOMAXCONN) ){
		int err = errno;
		cerr << "error putting the socket into listening mode: " << strerror(err) << " (" << err << ")" << endl;
		return INVALID_SOCKET;
	}

	//cout << "listening on port " << ntohs(ip.sin_port) << endl;
	return 0;
}


Sock::Sock(): s(INVALID_SOCKET) {
	memset(&ip,0,sizeof(ip));
    initNetwork();
}


Sock::Sock(SOCKET socket): s(socket) {
	memset(&ip,0,sizeof(ip)); // TODO: fill in ip from socket here???
    initNetwork();
}


Sock::~Sock(){
    if(s != INVALID_SOCKET){
        close();
    }
}


int Sock::close(void){
	if(::close(s)){
		int err = errno;
		cerr << "error closing socket: " << strerror(err) << " (" << err << ")" << endl;
	}
	s=INVALID_SOCKET; // Connect checks it
	return 0;
}


int Sock::write(const char* buff, int size){
	if(s==INVALID_SOCKET){
		cerr << "Socket not initialized - Can't write." << endl;
		return -1;
	}
	return send(s,buff,size,0);
}


int Sock::read(char* buff, int size){
	if(s==INVALID_SOCKET){
		cerr << "Socket not initialized - Can't read." << endl;
		return -1;
	}
	return recv(s, buff, size, MSG_DONTWAIT);
}


int Sock::readLine(char * buff, int maxSize){
    char* curr = buff;
	bool textSeen = false;
	while( curr-buff < maxSize-1 ){ // skip empty lines (or \r \n left from previous reads)
		if( read(curr,1) <= 0 ){ break; }
		if( *curr==0 ) { break; }
		if( *curr!='\n' && *curr!='\r' ){
			if(textSeen){ break; }
			else { ++curr; } // skip leading \r \n
		} else { textSeen = true; ++curr; }
	}
	*curr = 0;
	return curr-buff;
}

long Sock::readLong(bool& error){
    long data;
	int size = read((char*)&data, sizeof(data));
	error = size != sizeof(data);
	return ntohl(data);
}
short Sock::readShort(bool& error){
    short data;
	int size = read((char*)&data, sizeof(data) );
	error = size != sizeof(data);
	return ntohs(data);
}
