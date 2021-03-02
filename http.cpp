#include "http.h"
#include "sign.h"
#include "protocol.h"
#include <sstream>
#include <iostream>
#include <thread>
#include <cstring>
using namespace std;


int Writer::writeString(const string& str){ // TODO: move to Sock ???
    constexpr static const int MAX_STR_LEN = 255;
    unsigned char iclen = str.length();
    if(str.length() > MAX_STR_LEN){
        iclen = MAX_STR_LEN;
        cerr << "WARNING: truncating string: " << str;
    }
    sock->write( (char*) &iclen, 1);
    return 1 + sock->write( (char*) str.c_str(), iclen);
}



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


// TODO: rewrite Sock class' read / readLine / write to accept timeout
// look for a line like: GET /?QUERY=2036&PORT_EQ_2132 HTTP/1.1
int Request::parse(Sock& conn, vector<string>& filters){
    long int query = 0;
    char buff[2048];
    for(int i=0; i< 10; ++i) { // drop if the other side is slow to send request
        if(! conn.readLine(buff, sizeof(buff) ) ) {
            this_thread::sleep_for(milliseconds(100));
            continue;
        }
        cout << buff; // DEBUGGING ONLY!!!
        cout.flush(); // DEBUGGING ONLY!!!
        if( strncmp(buff,"GET",3) ){ continue; } // we want http GET query

        char* start = buff + 3;
        const char * end = buff+strlen(buff);
        char* token;
        while( tokenize( &start, end, &token) ){
            if( strncmp(token, "QUERY=", 6) == 0 ){
                query = atol(token+6);
            } else if( strcmp(token,"HTTP") && strcmp(token,"1.1") ){ // throw away these tokens
                filters.push_back(token);
            }
        }
        if(query>0){ break; } // we got all we need
        cout << "### Query did not parse correctly. Entering HTTP request debug mode. ###" << endl;
    }
    return query;
}

void turnBitOff(int* mask, int bit){
    *mask = *mask & (0xFFFFFFFF^bit);
}

int Response::write(Sock& conn, int select, vector<string>& filters, LocalData& ldata, RemoteData& rdata, BWLists& bwlists ){
    static const string header =  "HTTP/1.1 200 OK\nServer: n3+1\n\n";
    size_t bytes = conn.write(header.c_str(), header.size() ); // no need to sign the header

    // Return "SELECT" field in the response.
    // If you don't want to share some requested fieds, turn off the bits.
    turnBitOff(&select, SELECTION::WLKEY);  // do not share your pub key white list (friends)
    turnBitOff(&select, SELECTION::WLIP);   // b&w lists are not implemented yet
    turnBitOff(&select, SELECTION::BLKEY);  // b&w lists are not implemented yet
    turnBitOff(&select, SELECTION::BLIP);   // b&w lists are not implemented yet
    bytes+= conn.write((char*) &select, sizeof(select));

    bool sign = select & SELECTION::SIGN;
    Writer* writer = sign ? &signatureWriter : &dumbWriter;
    writer->init(conn);

    bytes+= ldata.send(  *writer, select, filters);
    bytes+= rdata.send(  *writer, select, filters);
    bytes+= bwlists.send(*writer, select, filters);

    if(sign){
        PubSign* sign = writer->getSignature();
        bytes+= conn.write((char*)sign, sizeof(PubSign));
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
    const char* msg = "HTTP/1.1 403 DENIED\n";
    conn.write(msg, strlen(msg));
}
