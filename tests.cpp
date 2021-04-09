// this is a stand alone utility. it queries the service.  to compile on linux:
// g++ --std=c++20 -g -Wall -otests tests.cpp sock.cpp crawler.cpp data.cpp sign.cpp utils.cpp -lpthread
// to compile on windows under MinGW-W64:
// g++ --std=c++20 -g -Wall -otests tests.cpp sock.cpp crawler.cpp data.cpp sign.cpp utils.cpp -lpthread -lwsock32 -static
// using make utility: "make tests" or "make -f makefile.mingw64 tests" on windows
#include "data.h"
#include "crawler.h"
#include "sock.h"
#include "http.h"
#include <iostream>
using namespace std;


void queryService(Crawler& crawl, HostInfo& service, uint32_t select){
    vector<HostInfo> newData; // results will be returned here
    vector<string> filters; // = {"AGE_LT_600"}; // = {"RPROT=ftp"}; // for FTP servers
    crawl.queryRemoteService(service, newData, select, &filters);

    for(auto& s: service.services){
        cout << "local: " << s.fullDescription << endl;
    }

    for(HostInfo& hi: newData){
        cout << Sock::ipToString(hi.host) << ":" << hi.port << endl;
        for(Service& s: hi.services){
            cout << "\t" << s.originalDescription << endl;
        }
    }
}


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
    service.host = Sock::stringToIP(argv[1]);
    service.port = (uint16_t) strtol(argv[2], nullptr, 10); // base 10

    // we want local services, remote IP:PORT and services
    uint32_t sel = SELECTION::LSVC | SELECTION::IP | SELECTION::PORT | SELECTION::RSVC;
    queryService(crawl, service, sel);
    cout << "press enter to continue..." << endl;
    cin.ignore();

    for(int32_t select = 1; select < 32*1024; ++select){
        // avoid combinations that request a signature without requesting a key
        if( (select & SELECTION::SIGN) ){
            select = select | SELECTION::LKEY;
        }
        queryService(crawl, service, select);
    }
}
