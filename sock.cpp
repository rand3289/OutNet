#include <iostream>
using namespace std;
#include "sock.h"


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

	SOCKADDR_IN ip;
	ip.sin_family=AF_INET;
	ip.sin_port=htons(port);
	ip.sin_addr.s_addr=*((unsigned long *)ipent->h_addr); // h_addr is macro for h_addr_list[0]
	if(ip.sin_addr.s_addr == INADDR_NONE){
		cerr << "gethostbyname() returned INADDR_NONE" << endl;
		return 3;
	}

	if( connect(s,(SOCKADDR*)&ip, sizeof(ip) )){
		int error = WSAGetLastError();
		return error;
	}
	return 0;
}


SOCKET Sock::accept(){
	int size = sizeof(ip);
	SOCKET serv = ::accept(s,(sockaddr*)&ip,&size);
	if(INVALID_SOCKET == serv ){
		cerr << "error accepting connection:" << WSAGetLastError() << endl;
	}
	return serv;
}


Sock::Sock(): peek(0){
	if(WSAStartup(0x0202,&wsd)){
		cerr << "Error upon WSAStartup()" << endl;
	}
}


int Sock::listen(unsigned short port){
 	s=socket(AF_INET,SOCK_STREAM,0);
	if(s==INVALID_SOCKET){
		cerr << "Error creating socket:"<< WSAGetLastError() << endl;
		return INVALID_SOCKET;
	}

	int size = sizeof(ip);
	ip.sin_family=AF_INET;
	ip.sin_port=htons(port);
	ip.sin_addr.s_addr = INADDR_ANY;
	int err = 0;

	if ( SOCKET_ERROR==bind(s, (sockaddr*) &ip, size)){
		cerr << "error binding to port " << port << " error:" << WSAGetLastError() << endl;
		return INVALID_SOCKET;
	}
	if ( SOCKET_ERROR==getsockname(s, (sockaddr*) &ip, &size)){
		cerr << "error getting bound socket information:" << WSAGetLastError() << endl;
		return INVALID_SOCKET;
	}

	if( SOCKET_ERROR == ::listen(s, SOMAXCONN) ){
		cerr << "error putting the socket into listening mode:" << WSAGetLastError() << endl;
		return INVALID_SOCKET;
	}

	//cout << "listening on port " << ntohs(ip.sin_port) << endl;
	return 0;
}


Sock::Sock(SOCKET client_socket): peek(0){ // if tcp is FALSE, use UDP
	if(WSAStartup(0x0202,&wsd)){
		cerr << "Error upon WSAStartup()" << endl;
	}
	s = client_socket;
}


Sock::~Sock(){
	close();
	WSACleanup(); // unloads the dll
}


int Sock::close(void){
	closesocket(s);
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
