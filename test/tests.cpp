// this is a stand alone utility. it queries the OutNet service.
// to compile on linux/windows type "make"
#include "client.h"
#include "sock.h"
#include <iostream>
#include <sstream>
using namespace std;

/*
// To register with OutNet, your service has to query it fist "queryOutNet()"
// By doing so you advertise your intent to register the service with OutNet.
// OutNet will connect back to your service and ask you for your service description
void registerWithOutNet(HostInfo& outNet){ // outNet host/port should be filled
    Sock sock;                             // this is a server socket
    sock.listen(Sock::ANY_PORT);           // create a TCP server on any port
    uint16_t serverPort = sock.getPort();  // which port did the server pick?

    uint32_t select = SELECTION::LSVC | SELECTION::LKEY; // pick the fields you want
    vector<HostInfo> newData;          // dont really need it
    queryOutNet(select, outNet, newData, serverPort);
    // in case you want it, outNet now contains local services and local public key
    // check if your service is there - maybe it is already registered?
    cout << "Query sent to OutNet.  Waiting for callback." <<endl;

    Sock conn;             // connection from OutNet to your service
    sock.accept(conn);     // server socket "sock" will accept a new connection into "conn"
    conn.setRWtimeout(10); // optional: set read/write timeouts

    // Prepare fields to send.  All numbers have to be in network byte order
    const char* header = "HTTP/1.1 200 OK\r\n\r\n"; // required HTTP header
    uint32_t fields = htonl(SELECTION::LSVC);       // your reply will contain local service descriptions ONLY
    uint16_t count  = htons(1);                     // one service
    string servInfo = "web:tcp:http:127.0.0.1:80:/index.html"; // your service description

    conn.write(header, strlen(header) );
    conn.write(&fields, sizeof(fields) );
    conn.write(&count, sizeof(count) );
    conn.writeString(servInfo);
}
*/

void registerMyService(uint32_t outNetIP, uint16_t outNetPort, const string& servInfo){
    stringstream ss;
    ss << "GET / HTTP/1.1\r\n";
    ss << "Register: " << servInfo << "\r\n\r\n";

    Sock sock;
    sock.connect(outNetIP, outNetPort);
    sock.writeString( ss.str() );
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
    if(argc!=3){
        cerr << "Usage: " << argv[0] << " host port" << endl;
        return -1;
    }

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

    for(int32_t select = 1; select < 32*1024; ++select){
        // avoid combinations that request a signature without requesting a key
        if( (select & SELECTION::SIGN) ){
            select = select | SELECTION::LKEY;
        }
        queryService(select, service);
    }
}
