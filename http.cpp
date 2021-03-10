#include "http.h"
#include "sign.h"
#include "protocol.h"
#include "utils.h"
#include <sstream>
#include <iostream>
#include <thread>
#include <cstring>
using namespace std;


// look for a line like: GET /?QUERY=2036&SPORT=33344&PORT_EQ_2132 HTTP/1.1
int Request::parse(Sock& conn, vector<string>& filters, uint16_t& port){
    uint32_t query = 0;
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
            } else if( strncmp(token, "SPORT=", 6) == 0){ // parse remote server port
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


int Response::write(Sock& conn, uint32_t select, vector<string>& filters, LocalData& ldata, RemoteData& rdata){
    static const string header =  "HTTP/1.1 200 OK\n\n";
    int bytes = conn.write(header.c_str(), header.size() ); // no need to sign the header

    bool sign = select & SELECTION::SIGN;
    Writer* writer = sign ? &signatureWriter : &dumbWriter;
    writer->init(conn);

    uint32_t netSelect = htonl(select);
    bytes+= writer->write( &netSelect, sizeof(netSelect));

    if( select & SELECTION::MYIP ){
        uint32_t remoteIP = conn.getIP();                    // remote IP the way I see it
        bytes+=writer->write( &remoteIP, sizeof(remoteIP)); // helps other server find it's NATed IPs
    }

    bytes+= ldata.send(  *writer, select, filters);
    bytes+= rdata.send(  *writer, select, filters);

    if(sign){
        const PubSign* psign = writer->getSignature();
        bytes+= conn.write( psign, sizeof(PubSign));
    }
    return bytes;
}


void Response::writeDebug(Sock& conn, uint32_t select, std::vector<std::string>& filters){
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


void Response::writeDenied(Sock& conn, const string& reason){
    string msg = string("HTTP/1.1 403 ").append(reason).append("\n\n");
    conn.write(msg.c_str(), msg.length() );
}
