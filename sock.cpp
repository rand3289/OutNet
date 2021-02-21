#include <iostream>
using namespace std;
#include "sock.h"

#ifdef WIN32
    #pragma comment(lib, "Ws2_32.lib") // link to winsock2 library
    #define close(s) closesocket(s)
    #define UNREACHABLE WSAECONNRESET
    #undef errno // do not use stdlib version in this file
    #define errno WSAGetLastError()
    typedef int socklen_t;
    typedef SOCKADDR sockaddr;
#else // posix
    #include <unistd.h> // close()
    #include <netdb.h> // gethostbyname()
    #include <sys/types.h>
    #include <sys/socket.h>
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR   (-1)
    #define UNREACHABLE    ECONNREFUSED
    #define WSACleanup()
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


int Sock::conn(const char* addr, int port){
 	s=socket(AF_INET,SOCK_STREAM,0);
	if(s==INVALID_SOCKET){
		cerr << "Error creating socket." << endl;
		return 1;
	}

	hostent* ipent=gethostbyname(addr);
	if(!ipent){
		cerr << "Can't gethostbyname()" << endl;
		return 2;
	}

	sockaddr_in ip;
	ip.sin_family=AF_INET;
	ip.sin_port=htons(port);
	ip.sin_addr.s_addr=*((unsigned long *)ipent->h_addr); // h_addr is macro for h_addr_list[0]
	if(ip.sin_addr.s_addr == INADDR_NONE){
		cerr << "gethostbyname() returned INADDR_NONE" << endl;
		return 3;
	}

	if( connect(s,(sockaddr*)&ip, sizeof(ip) )){
		int error = errno;
		return error;
	}
	return 0;
}


SOCKET Sock::accept(){
	sockaddr_in ipr;	// used for a server
	unsigned int size = sizeof(ipr);
	SOCKET serv = ::accept(s, (sockaddr*)&ipr, (socklen_t*)&size);
	if(INVALID_SOCKET == serv ){
		cerr << "error accepting connection:" << errno << endl;
	}
	return serv;
}


Sock::Sock(): peek(0){
    initNetwork();
}


int Sock::listen(unsigned short port){
 	s=socket(AF_INET,SOCK_STREAM,0);
	if(s==INVALID_SOCKET){
		cerr << "Error creating socket:"<< errno << endl;
		return INVALID_SOCKET;
	}

	unsigned int size = sizeof(ip);
	ip.sin_family=AF_INET;
	ip.sin_port=htons(port);
	ip.sin_addr.s_addr = INADDR_ANY;
//	int err = 0;

	if ( SOCKET_ERROR==bind(s, (sockaddr*) &ip, size)){
		cerr << "error binding to port " << port << " error:" << errno << endl;
		return INVALID_SOCKET;
	}
	if ( SOCKET_ERROR==getsockname(s, (sockaddr*) &ip, (socklen_t*)&size)){
		cerr << "error getting bound socket information:" << errno << endl;
		return INVALID_SOCKET;
	}

	if( SOCKET_ERROR == ::listen(s, SOMAXCONN) ){
		cerr << "error putting the socket into listening mode:" << errno << endl;
		return INVALID_SOCKET;
	}

	//cout << "listening on port " << ntohs(ip.sin_port) << endl;
	return 0;
}


Sock::Sock(SOCKET socket): peek(0) {
    initNetwork();
    s = socket;
}


Sock::~Sock(){
	close();
//	WSACleanup(); // unloads ws2_32.dll on windows
}


int Sock::close(void){
	::close(s);
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
	return recv(s,buff,size,0);
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
