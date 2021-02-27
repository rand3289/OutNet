#include "sock.h"
#include <iostream>
#include <cstring> // memset()
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
int Sock::conn(unsigned long ip, unsigned short port){
 	s=socket(AF_INET,SOCK_STREAM,0);
	if(s==INVALID_SOCKET){
		cerr << "Error creating socket." << endl;
		return -3;
	}

	sockaddr_in addr;
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=ip;

	if( connect(s, (sockaddr*)&addr, sizeof(addr) )){
		int err = errno;
		cerr << "error connecting to remote host via TCP: " << strerror(err) << " (" << err << ")" << endl;
		return err; // errno returns positive numbers
	}
	return 0;
}


int Sock::conn(const char* addr, unsigned short port){
	hostent* ipent=gethostbyname(addr);
	if(!ipent){
		cerr << "Can't gethostbyname()" << endl;
		return -1;
	}
	unsigned long ip = *(unsigned long*) ipent->h_addr;  // h_addr is a macro for h_addr_list[0]
	if(ip == INADDR_NONE){
		cerr << "gethostbyname() returned INADDR_NONE" << endl;
		return -2;
	}

	return conn(ip, port);
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


Sock::Sock(): s(INVALID_SOCKET), peek(0) {
	memset(&ip,0,sizeof(ip));
    initNetwork();
}


Sock::Sock(SOCKET socket): s(socket), peek(0) {
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
	return recv(s,buff,size,MSG_DONTWAIT);
}


int Sock::readLine(char* buff){
		if(peek){
			*buff = peek;
			++buff;
			peek = 0;
		}

		bool newLine = false;
		while(true){
			if( read(buff,1) > 0 ){
				newLine =  newLine || *buff=='\n' || *buff=='\r';
				if(newLine && *buff!='\n' && *buff!='\r'){
					peek = *buff; 
					*buff = 0;
					return 1;
				}
				++buff;
			} else {
				return 0;
			}

		}
		return 1;
}
