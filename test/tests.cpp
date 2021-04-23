// this is a stand alone utility. it queries the OutNet service.  To compile on linux/windows type "make".
#include "client.h"
#include "sock.h"
#include <iostream>
#include <sstream>
using namespace std;


void registerMyService(uint32_t outNetIP, uint16_t outNetPort, const string& servInfo){
    stringstream ss;
    ss << "GET / HTTP/1.1\r\n";
    ss << "Register: " << servInfo << "\r\n\r\n";

    Sock sock;
    sock.connect(outNetIP, outNetPort);
    sock.write( ss.str().c_str(), ss.str().length() );
}


void queryService(uint32_t select, HostInfo& service){
    vector<HostInfo> newData; // results will be returned here
    vector<string> filters; // = {"AGE_LT_600"}; // = {"RPROT=ftp"}; // for FTP servers
    queryOutNet(select, service, newData, 0, 10, &filters);

    for(string& s: service.services){
        cout << "local service: " << s << endl;
    }

    for(HostInfo& hi: newData){
        cout << Sock::ipToString(hi.host) << ":" << hi.port << endl;
        for(string& s: hi.services){
            cout << "\t" << s << endl;
        }
    }
}


int main(int argc, char* argv[]) {
    if( argc<3 || argc > 4){
        cerr << "Usage: " << argv[0] << " host port" << endl;
        return -1;
    }
    initNetwork();

    HostInfo service; // connect to this OutNet service
    service.host = Sock::stringToIP(argv[1]);
    service.port = (uint16_t) strtol(argv[2], nullptr, 10); // base 10

    cout << "Registering with OutNet" << endl;
    string servInfo = "web:tcp:http:127.0.0.1:80:/index.html"; // your service description
    registerMyService(service.host, service.port, servInfo);
    cout << "press enter to check registration..." << endl;
    cin.ignore();

    // we want local services, remote IP:PORT and services
    uint32_t sel = SELECTION::LSVC | SELECTION::IP | SELECTION::PORT | SELECTION::RSVC;
    queryService(sel, service);
    cout << "press enter to run tests..." << endl;
    cin.ignore();

    int i = argc==4 ? strtol(argv[3], nullptr, 10) : 1;
    for(; i <= 2048; ++i){
        int select = i;
        // avoid combinations that request a signature without requesting a key
        if( select & SELECTION::SIGN ){
            select = select | SELECTION::LKEY;
        }
        service.services.clear(); // clear results of previous queries
        cout << "[" << select << "] ";
        queryService(select, service);
        // TODO: sleep()
    }
}
