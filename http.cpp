#include "http.h"
#include "sign.h"
#include "protocol.h"
#include <sstream>
#include <iostream>
#include <thread>
using namespace std;


int Request::parseRequest(Sock& conn, vector<string>& filters){
    char buff[2048];
    this_thread::sleep_for(seconds(1)); // DEBUGGING ONLY!!!

    while( conn.readLine(buff) ){       // DEBUGGING ONLY!!!
        cout << buff;
    }
    cout.flush();
    return 0;
}

//#include <chrono>
//using namespace std::chrono;
//ss << system_clock::now().time_since_epoch().count();

int Response::write(Sock& conn, int select, vector<string>& filters, LocalData& ldata, RemoteData& rdata, BWLists& lists ){
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

    this_thread::sleep_for(seconds(2)); // DEBUGGING ONLY!!!

    return 0;
}
