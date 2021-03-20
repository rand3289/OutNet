#include "data.h"
#include "http.h"
#include "sock.h"
#include "protocol.h"
#include "utils.h"
#include <iomanip> // put_time()
#include <ctime> // time(), localtime()
#include <sstream>
#include <iostream>
#include <thread>
#include <cstring>
using namespace std;
#include <chrono>
using namespace std::chrono;


// take a query string filter ex: AGE_LT_600 and parse it in 3 strings separated by "_"
bool Request::parseFilter(char* filter, array<string,3>& funcOperVal){ // reference to array of strings
    char* end   = filter + strlen(filter);
    char* token = nullptr;
    if( !tokenize(filter, end, token, "_") ){ return false; }
    funcOperVal[0] = token; // function

    if( tokenize(filter, end, token, "_") ){ // operator or value might not be present ex: "KEY"
        funcOperVal[1] = token; // operator (EQ/LT/GT)

        if( tokenize(filter, end, token, "_") ){
            funcOperVal[2] = token; // value
        }
    }
    return true;
}


// look for a line like: GET /?QUERY=2036&SPORT=33344&PORT_EQ_2132 HTTP/1.1
int Request::parse(Sock& conn, vector<array<string,3>>& filters, uint16_t& port){
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
        char* token = nullptr;
        while( tokenize( start, end, token, " &?/") ){
            if( strncmp(token, "QUERY=", 6) == 0 ){ // parse QUERY
                query = atol(token+6);
            } else if( strncmp(token, "SPORT=", 6) == 0){ // parse remote server port
                port = atoi(token+6);
            } else if( strcmp(token,"HTTP") && strcmp(token,"1.1") ){ // throw away these tokens
                array<string,3> funcOperVal; // auto funcOperVal = filters.emplace_back();
                if( parseFilter(token, funcOperVal) ){
                    filters.push_back( move(funcOperVal) );
                }
            }
        }
        if(query>0){ break; } // we got all we need
        cout << "### Query did not parse correctly. Entering HTTP request debug mode. ###" << endl;
    }
    return query;
}


int Response::write(Sock& conn, uint32_t select, vector<array<string,3>>& filters, LocalData& ldata, RemoteData& rdata){
    static const string header =  "HTTP/1.1 200 OK\n\n";
    int bytes = conn.write(header.c_str(), header.size() ); // no need to sign the header

    bool sign = select & SELECTION::SIGN;
    if(sign){ signer.init(); }

    bytes+= conn.write32(select);
    if(sign) { signer.write(&select, sizeof(select) ); }

    if( select & SELECTION::MYIP ){
        uint32_t remoteIP = conn.getIP();                // remote IP the way I see it
        bytes+=conn.write( &remoteIP, sizeof(remoteIP)); // helps other server find it's NATed IPs
        if(sign){ signer.write(&remoteIP, sizeof(remoteIP)); }
    }

    bytes+= ldata.send(conn, select, filters, signer);
    bytes+= rdata.send(conn, select, filters, signer);

    if(sign){
        const PubSign& psign = signer.getSignature();
        bytes+= conn.write( &psign, sizeof(PubSign) );
    }
    return bytes;
}


void Response::writeDebug(Sock& conn, uint32_t select, std::vector<array<string,3>>& filters){
    stringstream ss;
    ss << "HTTP/1.1 200 OK\n";
    ss << "Server: n3+1\n\n"; // "\n" separates headers from html
    ss << "<html><body>";
    ss << "<a href='https://github.com/rand3289/OutNet'>INFO</a><br>";
    ss << "QUERY=" <<  select << "<br>";
    for(auto f: filters){
        ss << f[0] << "_" << f[1] << "_" << f[2] << "<br>";
    }
    time_t now = time(NULL);
    std::tm lnow = *localtime(&now);
    ss << put_time(&lnow,"%c %Z") << "<br>";
    ss << "</body> </html>";
    conn.write(ss.str().c_str(), ss.str().size() );
}


void Response::writeDenied(Sock& conn, const string& reason){
    string msg = string("HTTP/1.1 403 ").append(reason).append("\n\n");
    conn.write(msg.c_str(), msg.length() );
}
