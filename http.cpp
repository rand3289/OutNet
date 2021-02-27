#include "http.h"
#include "sign.h"
#include "protocol.h"
#include <sstream>
#include <iostream>
#include <thread>
#include <cstring>
using namespace std;


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
        char c = **buffer; // TODO: conver this to switch(c) ???
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
int Request::parseRequest(Sock& conn, vector<string>& filters){
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
            // TODO: throw away "HTTP" and "1.1" tokens here???
            if( strncmp(token, "QUERY=", 6) ){
                filters.push_back(token);
            } else {
                query = atol(token+6);
            }
        }
    }
    return query;
}


//#include <chrono>
//using namespace std::chrono;
//ss << system_clock::now().time_since_epoch().count();

int Response::write(Sock& conn, int select, vector<string>& filters, LocalData& ldata, RemoteData& rdata, BWLists& lists ){
    string header = "HTTP/1.1 200 OK\
Server: n3t1\
Transfer-Encoding: chunked\r\n\r\n";
    stringstream ss;
    ss << "<!DOCTYPE HTML PUBLIC \"\"\"\"><html><body>";
    ss << "QUERY=" <<  select << " Tokens:<br>";
    for(auto f: filters){
        ss << f << "<br>";
    }
    ss << "random challenge string: " << rand() << "<br>";
    ss << "</body> </html>";

    conn.write(header.c_str(), header.size());
    conn.write(ss.str().c_str(), ss.str().size());

    // TODO: sort out local and remote filters here ???
    ldata.send(conn,filters);
    // TODO: delete "local" filters before sending rdata???
    rdata.send(conn,filters);

    this_thread::sleep_for(seconds(2)); // DEBUGGING ONLY!!!

    return 0;
}
