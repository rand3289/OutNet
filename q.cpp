// this is a stand alone utility. it queries the service.  to compile on linux:
// g++ --std=c++20 -g -Wall -oq q.cpp sock.cpp crawler.cpp data.cpp sign.cpp utils.cpp -lpthread
// to compile on windows under MinGW-W64:
// g++ --std=c++20 -g -Wall -oq q.cpp sock.cpp crawler.cpp data.cpp sign.cpp utils.cpp -lpthread -lwsock32 -static
#include "data.h"
#include "crawler.h"
#include "sock.h"
#include "protocol.h"
#include <iostream>
using namespace std;

int main(int argc, char* argv[]) {
    if(argc!=3){
        cerr << "Usage: " << argv[0] << " host port" << endl;
        return -1;
    }

    LocalData ldata; // this is all dummy data to create a Crawler object
    RemoteData rdata;
    BlackList blist;
    Crawler crawl(ldata, rdata, blist);

    HostInfo service; // connect to this service
    service.host = Sock::strToIP(argv[1]);
    service.port = strtol(argv[2], nullptr, 10); // base 10

    vector<HostInfo> newData; // results will be returned here
    uint32_t select = SELECTION::IP | SELECTION::PORT | SELECTION::RSVC; // we want remote IP:PORT and services
    vector<string> filters; // = {"RPROT=ftp"}; // we are looking for FTP servers
    crawl.queryRemoteService(service, newData, select, &filters);

    for(HostInfo& hi: newData){
        cout << Sock::ipToString(hi.host) << ":" << hi.port << endl;
        for(Service& s: hi.services){
            cout << "\t" << s.originalDescription << endl;
        }
    }
}
