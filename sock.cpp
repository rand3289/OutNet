#include "sock.h"
#include <sstream>  // stringstream
#include <iostream> // cerr
#include <cstring>  // memset()
using namespace std;

#ifdef _WIN32
    #define close(s) closesocket(s)
    #undef errno // do not use stdlib version in this file
    #define errno WSAGetLastError()
    typedef int socklen_t;
    typedef SOCKADDR sockaddr;
#else // posix
    #include <unistd.h> // close()
    #include <netdb.h> // gethostbyname()
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <arpa/inet.h> // inet_ntop()
    #define SOCKET_ERROR   (-1)
#endif


void initNetwork(){
#ifdef _WIN32
    static bool initialized = false;
    if(initialized){ return; }
    initialized = true;
    WSADATA wsaData;
    if(WSAStartup(0x0202,&wsaData)){
        cerr << "Error upon WSAStartup()" << endl;
    }
#endif
}

int err(const string& msg) { // errno returns positive numbers
	int error = errno;
#ifdef _WIN32
	char buff[128];
	strerror_s(buff, error);
	cerr << "ERROR " << msg << buff << " (" << error << ")" << endl;
#else
	cerr << "ERROR " << msg << strerror(error) << " (" << error << ")" << endl;
#endif
	return error;
}

// set read()/write() timeout otherwise a remote client/server can get our process stuck!
int Sock::setRWtimeout(int seconds){
	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = seconds;
	if( setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof tv) ){ // RCV
	    return err("connecting to remote host via TCP: "); // errno returns positive numbers
	}
	if( setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof tv) ){ // SND
		return err("connecting to remote host via TCP: "); // errno returns positive numbers
    }
	return 0;
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
		return err("connecting to remote host via TCP: ");
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
	return 0==ip ? -1 : connect(ip, port);
}


SOCKET Sock::accept(){
	sockaddr_in ipr;	// used for a server
	socklen_t size = sizeof(ipr);
	SOCKET serv = ::accept(s, (sockaddr*)&ipr, &size);
	if(INVALID_SOCKET == serv ){
		err("accepting connection: ");
	}
	return serv;
}


SOCKET Sock::accept(Sock& conn){
	socklen_t size = sizeof(conn.ip);
	conn.s = ::accept(s, (sockaddr*)&conn.ip, &size);
	if(INVALID_SOCKET == conn.s ){
		err("accepting connection: ");
	}
	return conn.s;
}


int Sock::listen(uint16_t port){
 	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s==INVALID_SOCKET){
		err("creating socket: ");
		return -1;
	}

    int reuse = 1; // allow binding to a port if previous socket is lingering
    if ( 0 > setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) ){
        err("in setsockopt(SO_REUSEADDR): ");
		return -2;
    }

	int size = sizeof(ip);
	ip.sin_family=AF_INET;
	ip.sin_port=htons(port);
	ip.sin_addr.s_addr = INADDR_ANY;

	if ( SOCKET_ERROR==bind(s, (sockaddr*) &ip, size)){
		err("binding to port ");
		return -3;
	}
	if ( SOCKET_ERROR==getsockname(s, (sockaddr*) &ip, (socklen_t*)&size)){
		err("getting bound socket information: ");
		return -4;
	}

	if( SOCKET_ERROR == ::listen(s, SOMAXCONN) ){
		err("putting the socket into listening mode: ");
		return -5;
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
        closeSock();
    }
}


int Sock::closeSock(void){
	if(::close(s)){
		err("closing socket: ");
	}
	s=INVALID_SOCKET; // Connect checks it
	return 0;
}


int Sock::write(const void* buff, size_t size){
	cout << "WRITE: " << size << endl; // DEBUGGING!!!
	if(s==INVALID_SOCKET){
		cerr << "Socket not initialized - Can't write." << endl;
		return -1;
	}
	return send(s, (char*) buff, size, 0);
}


int Sock::read(void* buff, size_t size){
	cout << "READ: " << size << endl; // DEBUGGING!!!
	if(s==INVALID_SOCKET){
		cerr << "Socket not initialized - Can't read." << endl;
		return -1;
	}
	return recv(s, (char*) buff, size, 0);
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
    unsigned char iclen = (unsigned char) str.length();
    if(str.length() > MAX_STR_LEN){
        iclen = MAX_STR_LEN;
        cerr << "WARNING: writeString() truncating string: " << str << endl;
    }
    if( 1 != write( &iclen, 1) ){ return -1; }
    return 1 + write( str.c_str(), iclen);
}


int Sock::readString(string& str){
    unsigned char size; // since size is an unsigned char it can not be illegal
    int rdsize = read( &size, sizeof(size) );
    if( 1!=rdsize ){ return -1; } // ERROR
	str.resize(size+1); // +1 for null char
    rdsize = read( &str[0], size);
    if( rdsize!=size ){ return -2; } // ERROR
	return size;
}


int Sock::readString(void* buff, const size_t buffSize){ // make sure buff is at least 256 char long
    unsigned char size; // since size is an unsigned char it can not be illegal
    int rdsize = read( &size, sizeof(size) );
    if( 1!=rdsize ){ return -1; } // ERROR
	int original = size;
	size = (unsigned char) (size < buffSize ? size : buffSize-1);
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


string Sock::ipToString(uint32_t ip){
#ifdef _WIN32
    unsigned char* ipc = (unsigned char*) &ip;
    stringstream ss; // TODO: is ip in network byte order?
    ss << ipc[0] << "." << ipc[1] << "." << ipc[2] << "." << ipc[3];
    return ss.str();
#else
    char buff[16];
    inet_ntop(AF_INET, &ip, buff, sizeof(buff));
    return string(buff);
#endif
}
