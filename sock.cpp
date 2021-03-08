#include "sock.h"
#include <sstream>  // stringstream
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
int Sock::connect(uint32_t ipaddr, uint16_t port){
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


uint32_t Sock::strToIP(const char* addr){
    hostent* ipent=gethostbyname(addr);
	if(!ipent){
		cerr << "Can't gethostbyname()" << endl;
		return 0;
	}
	uint32_t ip = *(uint32_t*) ipent->h_addr;  // h_addr is a macro for h_addr_list[0]
	if(ip == INADDR_NONE){
		cerr << "gethostbyname() returned INADDR_NONE" << endl;
		return 0;
	}
	return ip;
}


int Sock::connect(const char* addr, uint16_t port){
	uint32_t ip = strToIP(addr);
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


int Sock::listen(uint16_t port){
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

	int size = sizeof(ip);
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

	return 0;
}


Sock::Sock(): s(INVALID_SOCKET) {
	memset(&ip,0,sizeof(ip));
    initNetwork();
}


Sock::Sock(SOCKET socket): s(socket) {
	initNetwork();
	socklen_t size = sizeof(ip);
	if( -1 == getpeername(socket, (sockaddr*)&ip, &size) ){ // if it fails, just clear ip
	    memset( &ip, 0,sizeof(ip) ); // getsockname() instead ???
	}
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


// TODO: rewrite read()/write() to accept a timeout

int Sock::write(const void* buff, size_t size){
	cout << "WRITE: " << size << endl; // DEBUGGING!!!
	if(s==INVALID_SOCKET){
		cerr << "Socket not initialized - Can't write." << endl;
		return -1;
	}
	return send(s,buff,size,0);
}


int Sock::read(void* buff, size_t size){
	cout << "READ: " << size << endl; // DEBUGGING!!!
	if(s==INVALID_SOCKET){
		cerr << "Socket not initialized - Can't read." << endl;
		return -1;
	}
	return recv(s, buff, size, 0);
//	return recv(s, buff, size, MSG_DONTWAIT);
}


int Sock::readLine(void* buff, const size_t maxSize){
    char* curr = (char*)buff;
	while( (size_t)(curr-(char*)buff) < maxSize-1 ){
		if( read(curr,1) <= 0 ){ break; }
		if( *curr==0 ) { break; }
		if( *curr=='\n'){ break; }
		if( *curr!='\r' ){ ++curr; }  // skip \r
	}
	*curr = 0;
	return curr-(char*)buff;
}

uint32_t Sock::read32(bool& error){
    uint32_t data;
	int size = read(&data, sizeof(data));
	error = (size != sizeof(data) );
	return ntohl(data);
}

uint16_t Sock::read16(bool& error){
    uint16_t data;
	int size = read(&data, sizeof(data) );
	error = (size != sizeof(data) );
	return ntohs(data);
}

int Sock::write32(uint32_t data){
    data = htonl(data);
    return write( &data, sizeof(data) );
}

int Sock::write16(uint16_t data){
    data = htons(data);
    return write( &data, sizeof(data) );
}


int Sock::writeString(const string& str){
    constexpr static const int MAX_STR_LEN = 255;
    unsigned char iclen = str.length();
    if(str.length() > MAX_STR_LEN){
        iclen = MAX_STR_LEN;
        cerr << "WARNING: writeString() truncating string: " << str << endl;
    }
    if( 1 != write( &iclen, 1) ){ return -1; }
    return 1 + write( str.c_str(), iclen);
}


int Sock::readString(void* buff, const size_t buffSize){ // make sure buff is at least 256 char long
    unsigned char size; // since size is an unsigned char it can not be illegal
    int rdsize = read( &size, sizeof(size) );
    if( 1!=rdsize ){ return -1; } // ERROR
	int original = size;
	size = size < buffSize ? size : buffSize-1;
    rdsize = read( buff, size);
    if( rdsize!=size ){ return -2; } // ERROR
    ((char*)buff)[size] = 0; // null terminate the string

	if(original> size){ // if buffer is too small, read the rest from stream and discard
		cerr << "WARNING: readString() buffer too small." << endl;
		char localBuff[256];
        rdsize = read( localBuff, original-size);
        if( rdsize!=size ){ return -3; } // ERROR
	}
    return original; // return the number of bytes read from socket
}


// https://en.wikipedia.org/wiki/Private_network
bool Sock::isRoutable(uint32_t ip){ // is it routable or non-routable IP ?
    unsigned char* ipc = (unsigned char*) &ip;
    if( 10 == ipc[0] ){ return false; }
    if( 192 == ipc[0] && 168 == ipc[1] ){ return false; }
    if( 172 == ipc[0] && ipc[1] >= 16 && ipc[1] <=31 ){ return false; }
    return true; // TODO: make this portable
}


#include <arpa/inet.h> // inet_ntop()
string Sock::ipToString(uint32_t ip){
    char buff[16];
    inet_ntop(AF_INET, &ip, buff, sizeof(buff));
	return string(buff);
//	unsigned char* ipc = (unsigned char*) &ip;
//	stringstream ss; // TODO: is ip in network byte order?
//	ss << ipc[0] << "." << ipc[1] << "." << ipc[2] << "." << ipc[3];
//	return ss.str();
}
