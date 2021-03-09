// this is a stand alone utility. it queries the service.  to compile:
// g++ --std=c++20 -g -Wall -oq q.cpp sock.cpp crawler.cpp data.cpp sign.cpp utils.cpp -lpthread
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

    LocalData ldata;
    RemoteData rdata;
    BWLists bwlists;
    Crawler crawl(ldata, rdata, bwlists);

    HostInfo service;
    service.host = Sock::strToIP(argv[1]);
    service.port = atoi(argv[2] );
    vector<HostInfo> newData;
    uint32_t select = SELECTION::IP | SELECTION::PORT | SELECTION::RSVC;
    HostPort self;
    crawl.queryRemoteService(service, newData, select, self);

    for(HostInfo& hi: newData){
        cout << Sock::ipToString(hi.host) << ":" << hi.port << endl;
        for(Service& s: hi.services){
            cout << "\t" << s.originalDescription << endl;
        }
    }
}
