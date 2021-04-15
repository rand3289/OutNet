// this is a stand alone utility. it queries the OutNet service.
// to compile on linux: "make" or "make -f makefile.mingw64" on windows
#include "client.h"
#include "sock.h"
#include <iostream>
using namespace std;


void queryService(uint32_t select, HostInfo& service){
    vector<HostInfo> newData; // results will be returned here
    vector<string> filters; // = {"AGE_LT_600"}; // = {"RPROT=ftp"}; // for FTP servers
    queryOutNet(select, service, newData, 0, 10, &filters);

    for(string& s: service.services){
        cout << "local: " << s << endl;
    }

    for(HostInfo& hi: newData){
        cout << Sock::ipToString(hi.host) << ":" << hi.port << endl;
        for(string& s: hi.services){
            cout << "\t" << s << endl;
        }
    }
}


int main(int argc, char* argv[]) {
    if(argc!=3){
        cerr << "Usage: " << argv[0] << " host port" << endl;
        return -1;
    }

    HostInfo service; // connect to this service
    service.host = Sock::stringToIP(argv[1]);
    service.port = (uint16_t) strtol(argv[2], nullptr, 10); // base 10

    // we want local services, remote IP:PORT and services
    uint32_t sel = SELECTION::LSVC | SELECTION::IP | SELECTION::PORT | SELECTION::RSVC;
    queryService(sel, service);
    cout << "press enter to continue..." << endl;
    cin.ignore();

    for(int32_t select = 1; select < 32*1024; ++select){
        // avoid combinations that request a signature without requesting a key
        if( (select & SELECTION::SIGN) ){
            select = select | SELECTION::LKEY;
        }
        queryService(select, service);
    }
}
