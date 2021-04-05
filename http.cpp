#include "data.h"
#include "http.h"
#include "sock.h"
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
            return true;
        }
    }
    return false;
}


// look for a line like: GET /?QUERY=2036&SPORT=33344&PORT_EQ_2132 HTTP/1.1
int Request::parse(Sock& conn, vector<array<string,3>>& filters, uint16_t& port){
    char buff[2048];
    int rd = conn.readLine(buff, sizeof(buff));
    if(rd < 0){ return -1; } // error reading more data (connection closed/timed out)
    if( strncmp(buff,"GET",3) ){ // not an http GET query
        cout << buff << endl; // DEBUGGING ONLY!!!
        return -1;
    }
    string line = buff; // save it for debugging

    int query = 0;
    char* start = buff + 3; // skip "GET"
    const char * end = start+strlen(start);
    char* token = nullptr;
    while( tokenize( start, end, token, " &?/") ){
        if( strncmp(token, "QUERY=", 6) == 0 ){ // parse QUERY
            query = strtol(token+6, nullptr, 10); // base 10
        } else if( strncmp(token, "SPORT=", 6) == 0){ // parse remote server port
            port = (uint16_t) strtol(token+6, nullptr, 10); // base 10
        } else if( strcmp(token,"HTTP") && strcmp(token,"1.1") ){ // throw away these tokens
            array<string,3> funcOperVal; // auto funcOperVal = filters.emplace_back();
            if( parseFilter(token, funcOperVal) ){
                filters.push_back( move(funcOperVal) );
            }
        }
    }
    if( query <= 0 ){
        cout << "ERROR parsing query: " << line << endl;
    }
    return query;
}


int Response::write(Sock& conn, uint32_t select, vector<array<string,3>>& filters, LocalData& ldata, RemoteData& rdata){
    static const string header =  "HTTP/1.1 200 OK\r\n\r\n"; // TODO: need full header???
    int bytes = conn.write(header.c_str(), header.size() ); // no need to sign the header

    bool sign = select & SELECTION::SIGN;
    if(sign){ signer.init(); }

    bytes+= conn.write32(select);
    if(sign) { signer.write(&select, sizeof(select) ); }

    bytes+= ldata.send(conn, select, filters, signer);
    bytes+= rdata.send(conn, select, filters, signer);

    if(sign){
        PubSign psign;
        signer.generate(psign);
        bytes+= conn.write( &psign, sizeof(psign) );
    }
    return bytes;
}


void Response::writeDebug(Sock& conn, uint32_t select, std::vector<array<string,3>>& filters){
    stringstream ss;
    ss << "<html><body>";

    time_t now = time(NULL);
    std::tm lnow;
#ifdef _WIN32
    localtime_s(&lnow, &now); // fucking MSFT
#else
    localtime_r(&now, &lnow);
#endif
    ss << put_time(&lnow,"%c %Z") << "<br>";

    ss << "<a href='https://github.com/rand3289/OutNet'>OutNet INFO</a><br>";
    ss << "QUERY=" <<  select << "<br>";
    for(auto f: filters){
        ss << f[0] << "_" << f[1] << "_" << f[2] << "<br>";
    }
    ss << "</body> </html>";

    stringstream ssh; // ss header
    ssh << "HTTP/1.1 200 OK\r\n"; // second "\r\n" separates headers from html
    ssh << "Content-Type: text/html\r\n";
    ssh << "Content-Length: " << ss.str().length() << "\r\n";
    ssh << "\r\n";
    ssh << ss.str();

    conn.write(ssh.str().c_str(), ssh.str().size() );
}


void Response::writeDenied(Sock& conn, const string& reason){
    string msg = string("HTTP/1.1 403 ").append(reason).append("\r\n\r\n");
    conn.write(msg.c_str(), msg.length() );
}
