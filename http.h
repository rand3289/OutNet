#ifndef HTTP_H_INCLUDED
#define HTTP_H_INCLUDED

#include "data.h"
#include "sign.h"
#include "sock.h"
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <thread>
using namespace std;


class Request{
    Sock& conn;
public:
    Request(Sock& connection): conn(connection) {}
    int parseRequest(vector<string>& filters); // returns QUERY bit field
};

class Response{
    Sock& conn;
public:
    Response(Sock& connection): conn(connection) {}
    int write(int select, vector<string>& filters, LocalData& ldata, RemoteData& rdata);
};


int Request::parseRequest(vector<string>& filters){
    char buff[2048];
    this_thread::sleep_for(seconds(1)); // DEBUGGING ONLY!!!
    while( conn.readLine(buff) ){ // DEBUGGING ONLY!!!
        cout << buff;
    }
    cout.flush();
    return 0;
}

//#include <chrono>
//using namespace std::chrono;
//ss << system_clock::now().time_since_epoch().count();

int Response::write(int select, vector<string>& filters, LocalData& ldata, RemoteData& rdata){
    string header = "HTTP/1.1 200 OK\
Server: n3t1\
Transfer-Encoding: chunked\r\n\r\n";
    stringstream ss; ss << rand();
    string data = "<!DOCTYPE HTML PUBLIC \"\"\"\"><html><body> Hello World! from n3+1<br>" +
        ss.str() + "</body> </html>";

    conn.write(header.c_str(), header.size());
    conn.write(data.c_str(), data.size());

    // TODO: sort out local and remote filters here ???
    ldata.send(conn,filters);
    // TODO: delete "local" filters before sending rdata???
    rdata.send(conn,filters);

    return 0;
}

#endif // HTTP_H_INCLUDED