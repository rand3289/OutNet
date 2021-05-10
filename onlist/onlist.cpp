// This is a stand alone utility to query OutNet service.  To compile on linux/windows type "make".
#include "client.h"
#include "sock.h"
#include <iostream>
using namespace std;


int main(int argc, char* argv[]) {
    if( argc != 3 ){
        cerr << "OutNet service list utility.  Usage: " << argv[0] << " OutNetIP OutNetPort" << endl;
        return -1;
    }
    initNetwork();

    HostInfo service; // queryOutNet() connects to this OutNet service
    service.host = Sock::stringToIP(argv[1]);
    service.port = (uint16_t) strtol(argv[2], nullptr, 10); // base 10

    // we want local services, remote IP:PORT and services
    uint32_t select = SELECTION::LSVC | SELECTION::IP | SELECTION::PORT | SELECTION::RSVC;

    vector<HostInfo> newData; // results will be returned here
    vector<string> filters; // = {"AGE_LT_600"}; // = {"RPROT=ftp"}; // for FTP servers
    queryOutNet(select, service, newData, 0, 10, &filters);

    cout << argv[1] << ":" << argv[2] << endl; // TODO: print public key after every host
    for(string& s: service.services){
        cout << "\t" << s << endl;
    }

    for(HostInfo& hi: newData){
        cout << Sock::ipToString(hi.host) << ":" << hi.port << endl;
        for(string& s: hi.services){
            cout << "\t" << s << endl;
        }
    }
}
