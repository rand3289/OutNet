// This is a stand alone utility to query OutNet service.  To compile on linux/windows type "make".
#include "client.h"
#include "sock.h"
#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;


std::string toString(const PubKey& key) {
    stringstream ss;
    for(unsigned int i = 0; i< sizeof(key.key); ++i){
        ss << std::setw(2) << std::setfill('0') << std::hex << (int) key.key[i];
    }
    return ss.str();
}


void printInfo(HostInfo& hi) {
    string key = hi.key ? toString(*hi.key) : "";
    cout << Sock::ipToString(hi.host) << ":" << hi.port << "\t" << key << endl;
    for(string& s: hi.services){
        cout << "\t" << s << endl;
    }
}


int main(int argc, char* argv[]) {
    if( argc != 3 ){
        cerr << "OutNet service list utility.  Usage: " << argv[0] << " OutNetIP OutNetPort" << endl;
        return -1;
    }
    initNetwork();

    HostInfo service; // queryOutNet() connects to this OutNet service
    service.host = Sock::stringToIP(argv[1]);
    service.port = (uint16_t) strtol(argv[2], nullptr, 10); // base 10

    // we want local services, remote IP:PORT and services + all public keys
    uint32_t select = SELECT::LSVC | SELECT::LKEY | SELECT::IP | SELECT::PORT | SELECT::RSVC | SELECT::RKEY;

    vector<HostInfo> newData; // results will be returned here
    vector<string> filters; // = {"AGE_LT_600"}; // = {"RPROT=ftp"}; // for FTP servers
    queryOutNet(select, service, newData, 0, 10, &filters);

    printInfo(service);
    for(HostInfo& hi: newData){
        printInfo(hi);
    }
}
