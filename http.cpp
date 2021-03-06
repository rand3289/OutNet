#include "http.h"
#include "sign.h"
#include "protocol.h"
#include <sstream>
#include <iostream>
#include <thread>
#include <cstring>
using namespace std;

// TODO: accept buffer and token by reference ???
bool tokenize( char** buffer, const char* bufferEnd, char** token ){
    while(*buffer != bufferEnd){ // skip leading separators
        char c = **buffer;
        if( 0==c || '\r'==c || '\n'==c ) { // end of line - no more tokens
            return false;
        }
        if( ' '==c || '&'==c || '?'==c || '/'==c ) { // skip separators
            ++*buffer;
        } else {
            break;
        }
    }

    *token = *buffer;

    while(*buffer != bufferEnd){
        char c = **buffer; // TODO: convert this to switch(c) ???
        if( ' '==c || '&'==c || '?'==c || '/'==c || '\r'==c || '\n'==c ) { // end of token
            **buffer=0; // separate strings
            ++*buffer;
            return true;
        }
        ++*buffer; // skip to the end of token
    }

    return false;
}


// look for a line like: GET /?QUERY=2036&SPORT=33344&PORT_EQ_2132 HTTP/1.1
int Request::parse(Sock& conn, vector<string>& filters, unsigned short& port){
    long int query = 0;
    char buff[2048];
    for(int i=0; i< 10; ++i) { // drop if the other side is slow to send request
        if(! conn.readLine(buff, sizeof(buff) ) ) {
            this_thread::sleep_for(milliseconds(100));
            continue;
        }
        cout << buff << endl; // DEBUGGING ONLY!!!
        if( strncmp(buff,"GET",3) ){ continue; } // we want http GET query

        char* start = buff + 3; // skip "GET"
        const char * end = start+strlen(start);
        char* token;
        while( tokenize( &start, end, &token) ){
            if( strncmp(token, "QUERY=", 6) == 0 ){ // parse QUERY
                query = atol(token+6);
            } else if(token, "SPORT=", 6){ // parse remote server port
                port = atoi(token+6);
            } else if( strcmp(token,"HTTP") && strcmp(token,"1.1") ){ // throw away these tokens
                filters.push_back(token);
            }
        }
        if(query>0){ break; } // we got all we need
        cout << "### Query did not parse correctly. Entering HTTP request debug mode. ###" << endl;
    }
    return query;
}


void turnBitsOff(int& mask, int bits){
    mask = mask & (0xFFFFFFFF^bits); // TODO: return a new mask instead of taking a reference???
}

int Response::write(Sock& conn, int select, vector<string>& filters, LocalData& ldata, RemoteData& rdata, BWLists& bwlists ){
    static const string header =  "HTTP/1.1 200 OK\n\n";
    size_t bytes = conn.write(header.c_str(), header.size() ); // no need to sign the header

    // If you don't want to share some requested fieds, turn off the bits.
    turnBitsOff(select, SELECTION::WLKEY);  // do not share your pub key white list (friends)
    turnBitsOff(select, SELECTION::WLIP | SELECTION::BLKEY | SELECTION::BLIP ); // not implemented

    bool sign = select & SELECTION::SIGN;
    Writer* writer = sign ? &signatureWriter : &dumbWriter;
    writer->init(conn);

    unsigned int netSelect = htonl(select);
    bytes+= writer->write((char*) &netSelect, sizeof(netSelect));

    if( select & SELECTION::MYIP ){
        unsigned long remoteIP = conn.getIP();             // remote IP the way I see it
        writer->write((char*)&remoteIP, sizeof(remoteIP)); // helps other server find it's NATed IPs
    }

    bytes+= ldata.send(  *writer, select, filters);
    bytes+= rdata.send(  *writer, select, filters);
    bytes+= bwlists.send(*writer, select, filters);

    if(sign){
        const PubSign* psign = writer->getSignature();
        bytes+= conn.write((char*)psign, sizeof(PubSign));
    }
    return bytes;
}


void Response::writeDebug(Sock& conn, int select, std::vector<std::string>& filters){
    stringstream ss;
    ss << "HTTP/1.1 200 OK\n";
    ss << "Server: n3+1\n\n"; // "\n" separates headers from html
    ss << "<html><body>";
    ss << "<a href='https://github.com/rand3289/OutNet'>INFO</a><br>";
    ss << "QUERY=" <<  select << "<br>";
    for(auto f: filters){
        ss << f << "<br>";
    }
    time_t now = time(NULL); 
    ss << ctime(&now) << "<br>";
    ss << "</body> </html>";
    conn.write(ss.str().c_str(), ss.str().size() );
}


void Response::writeDenied(Sock& conn){
    const char* msg = "HTTP/1.1 403 DENIED\n\n";
    conn.write(msg, strlen(msg));
}
