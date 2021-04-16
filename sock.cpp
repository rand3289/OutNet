#include "sock.h"
#include <sstream>  // stringstream
#include <iostream> // cerr
#include <cstring>  // memset()
using namespace std;

#ifdef _WIN32
    #include <ws2tcpip.h> //    typedef int socklen_t;
    #define close(s) closesocket(s)
	#define SHUT_RDWR SD_BOTH // shutdown(socket, how) 
    #define MSG_NOSIGNAL 0    // send() param
#else // posix
    #include <signal.h> // signal() 
    #include <unistd.h> // close()
    #include <netdb.h>  // gethostbyname()
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>   // inet_ntop()
    #include <netinet/tcp.h> // TCP_CORK, TCP_NODELAY
    #define SOCKET_ERROR   (-1)
#endif


int err(const string& msg) { // display error on cerr
#ifdef _WIN32
	int error = WSAGetLastError();
#else
	int error = errno;
#endif

#ifdef _MSC_VER // Visual Studio only. MinGW-w64 has a different strerror_s() definition
	char buff[128];
	strerror_s(buff, error);
	cerr << "ERROR " << msg << buff << " (" << error << ")" << endl;
#else
	cerr << "ERROR " << msg << strerror(error) << " (" << error << ")" << endl;
#endif
	return error; // errno returns positive numbers
}


bool initNetwork(){
    static bool initialized = false;
    if(initialized){ return true; }
#ifdef _WIN32
    WSADATA wsaData;
    if(WSAStartup(0x0202, &wsaData)){
        return err("WSAStartup() failed: ");
    }
#else // TODO: use sigaction()?  set a handler and log received signals?
    signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE signal which occurs in send()
#endif
    initialized = true;
    return true;
}


// set read()/write() timeout otherwise a remote client/server can get our process stuck!
int Sock::setRWtimeout(int seconds){
#ifdef _WIN32
	DWORD tv = seconds*1000; // milliseconds // fucking MSFT
#else
	timeval tv = { seconds, 0 }; // tv.tv_sec = seconds; tv.tv_usec = 0;
#endif
	if( setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv) ) ){ // RCV
	    return err("setting read timeout: "); // errno returns positive numbers
	}
	if( setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv) ) ){ // SND
		return err("setting write timeout: "); // errno returns positive numbers
    }
	return 0;
}


int Sock::setNoDelay(bool state){
	int onoff = state ? 1 : 0;
	return setsockopt(s, SOL_SOCKET, TCP_NODELAY, (char*) &onoff, sizeof(onoff) );
}


int Sock::setCork(bool state){
#ifdef _WIN32
	return 0; // no TCP_CORK on windblows
#else
	int onoff = state ? 1 : 0;
	return setsockopt(s, SOL_SOCKET, TCP_CORK, (char*) &onoff, sizeof(onoff) );
#endif
}


// ip has to be in the network byte order!!!
int Sock::connect(uint32_t ipaddr, uint16_t port){
	ip.sin_family      = AF_INET;
	ip.sin_addr.s_addr = ipaddr;
	ip.sin_port        = htons(port);

	if( ::connect(s, (sockaddr*)&ip, sizeof(ip) ) ){
		return err("connecting to remote host via TCP: ");
	}
	return 0;
}


int Sock::connect(const char* addr, uint16_t port){
	uint32_t ip = stringToIP(addr);
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


Sock::Sock() {
	initNetwork();
	memset(&ip,0,sizeof(ip));
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s==INVALID_SOCKET){
		err("creating socket: ");
	}
}


Sock::Sock(SOCKET socket): s(socket) {
	socklen_t size = sizeof(ip);
	if( SOCKET_ERROR == getpeername(socket, (sockaddr*)&ip, &size) ){
		memset( &ip, 0, sizeof(ip) ); // if getpeername() fails, just clear ip
	}
}


Sock::~Sock(){
    if(s != INVALID_SOCKET){
        closeSock();
    }
}


int Sock::closeSock(void){
	shutdown(s, SHUT_RDWR);
//	if( shutdown(s, SHUT_RDWR) ){
//		err("shutting down socket: ");
//	}
	if(::close(s)){
		err("closing socket: ");
	}
	s=INVALID_SOCKET; // Connect checks it
	return 0;
}


int Sock::write(const void* buff, size_t size){
//	cout << "WRITE: " << size << endl; // DEBUGGING!!!
	return send(s, (char*) buff, size, MSG_NOSIGNAL);
}

//#include "utils.h"  // printHex()
int Sock::read(void* buff, size_t size){
	int ret = recv(s, (char*) buff, size, 0); //	return recv(s, buff, size, MSG_DONTWAIT);
//	int wserr = WSAGetLastError();
//	cout << "ERRNO: " << wserr << " READ: " << ret << " bytes: "; // DEBUGGING!!!
//	printHex( (const unsigned char*)buff, ret); // 10060=WSAETIMEDOUT
	return ret;
}


int Sock::readLine(void* buff, size_t maxSize){
    char* curr = (char*)buff;
	while( maxSize > (size_t) (curr-(char*)buff) ){
		if( read(curr,1) <= 0 ){
			if(curr == buff){ return -1; }
			break;
		}
		if( *curr==0 ) { break; }
		if( *curr=='\n'){ break; }
		if( *curr!='\r' ){ ++curr; }  // skip \r only
	}
	*curr = 0;
	return (int) (curr-(char*)buff);
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


int Sock::readString(void* buff, size_t buffSize){ // make sure buff is at least 256 char long
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
// is it a routable or a non-routable IP ?
bool Sock::isRoutable(uint32_t ip){ // static
    unsigned char* ipc = (unsigned char*) &ip;
    if( 10 == ipc[0] ){ return false; }
    if( 192 == ipc[0] && 168 == ipc[1] ){ return false; }
    if( 172 == ipc[0] && ipc[1] >= 16 && ipc[1] <=31 ){ return false; }
    if( 127 == ipc[0] && ipc[1] == 0 && ipc[2] == 0 && ipc[3] == 1 ){ return false; } // localhost
    if( 169 == ipc[0] && ipc[1] == 254 ){ return false; } // APIPA
//    if( 224 <= ipc[0] && ipc[0] <= 239 ){ return false; } // multicast RFC 5771
    return true;
}


uint32_t Sock::stringToIP(const char* addr){ // static
    hostent* ipent=gethostbyname(addr);
	if(!ipent){
		err ("Can't gethostbyname(): ");
		return 0;
	}
	uint32_t ip = *(uint32_t*) ipent->h_addr;  // h_addr is a macro for h_addr_list[0]
	if(ip == INADDR_NONE){
		err("gethostbyname() returned INADDR_NONE: ");
		return 0;
	}
	return ip;
}


// turn ip address into a string.  ip has to be in network byte order
string Sock::ipToString(uint32_t ip){ // static
#ifdef _WIN32
    unsigned char* ipc = (unsigned char*) &ip;
    stringstream ss;
    ss << (int)ipc[0] << "." << (int)ipc[1] << "." << (int)ipc[2] << "." << (int)ipc[3];
    return ss.str();
#else
    char buff[INET_ADDRSTRLEN];
    if( NULL == inet_ntop(AF_INET, &ip, buff, sizeof(buff) ) ){
        err("Error converting IP to string: ");
        return "";
    }
    return string(buff);
#endif
}
