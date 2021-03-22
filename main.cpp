#include "data.h"
#include "config.h"
#include "crawler.h"
#include "sock.h"
#include "http.h"
#include <vector>
#include <array>
#include <map> // #include <unordered_map>
#include <string>
#include <thread>
#include <iostream>
using namespace std;


// Three threads: main one serves requests, second (crawler) collects info, 
// third thread subscribes services & loads black lists.
int main(int argc, char* argv[]){
    // LocalData, RemoteData  and BWLists are shared among threads.  They all have internal mutexes.
    LocalData ldata;  // info about local services and local public key
    RemoteData rdata; // information gathered about remote services
    BlackList blist;  // Black and White lists

    if(3 == argc){ // parse command line parameters IP and port
        uint32_t ip = Sock::strToIP(argv[1]);
        uint16_t portInt = strtol(argv[2], nullptr, 10); // base 10
        if( 0!=ip && 0!=portInt ){
            rdata.addContact(ip, portInt);
        }
    } else if(1 != argc){
        cerr << "ERROR parsing command line parameters.  ";
        cerr << "usage: " << argv[0] << " host port" << endl;
        return -1;
    }


    Config config; // config is aware of service port, LocalData and BWLists
    config.loadFromDisk(ldata, blist); // load ldata,bwlists

    char* pkey = ldata.localPubKey.loadFromDisk(); // load public key from disk into ldata
    if(!pkey){
        cerr << "ERROR loading public key!  Exiting." << endl;
        return -1;
    }

    // create the server returning all queries
    // first time start running on a random port (ANY_PORT)
    // but if service ran before, reuse the port number
    Sock server;
    if( server.listen(ldata.myPort) < 0 ){
        cerr << "ERROR listening for connections on port " << ldata.myPort << ".  Exiting." << endl;
        return 1;
    }

    ldata.myPort = server.getPort(); // get bound server port number from socket
    config.saveToDisk();
    cout << "Running on port " << ldata.myPort << endl;

    // create a thread that watches files for service and BWList updates
    std::thread watch( &Config::watch, &config);

    // create the information collector thread here (there could be many in the future)
    // it searches and fills rdata while honoring BWLists
    Crawler crawler(ldata, rdata, blist);
    crawler.loadRemoteDataFromDisk(); // load rdata
    std::thread search( &Crawler::run, &crawler);

    // unordered_map::operator[] crashes.  Switching to map for now.
    // unordered_map<uint32_t, system_clock::time_point> connTime; // keep track of when someone connects to us
    map<uint32_t, system_clock::time_point> connTime; // keep track of when someone connects to us
    Response response;
    while(true){
        Sock conn;
        if( INVALID_SOCKET == server.accept(conn) ){ continue; }
        conn.setRWtimeout(3); // 3 second read/write timeout // TODO: add it to config???

        uint32_t ip = conn.getIP();
        if( blist.isBanned(ip) ){
            cout << "Denying REQUEST from " << Sock::ipToString(ip) << " (BANNED)" << endl;
            Response::writeDenied(conn, "BANNED");
            continue;
        }

        // prevent abuse if host connects too often but allow local IPs repeated queries
        auto& time = connTime[ip];
        if( time > system_clock::now() - minutes(10) && Sock::isRoutable(ip) ){
            cout << "Denying REQUEST from " << Sock::ipToString(ip) << " (connects too often)" << endl;
            Response::writeDenied(conn, "DENIED");
            continue;
        }
        time = system_clock::now();

        // parse remote server port, "SELECT field bitmap" and filters from remote's "HTTP get" query
        vector<array<string,3>> filters; // function + parameter pairs
        uint16_t port = 0;
        uint32_t select = Request::parse(conn, filters, port);

        if(port > 0){ // remote sent us its server port.  Add accepted connection to RemoteData
            rdata.addContact(conn.getIP(), port );
        }

        if(select > 0){ // request parsed correctly and we have a "data selection bitmap"
            cout << "Normal REQUEST from " << Sock::ipToString(ip) << " select=" << select << endl;
            response.write(conn, select, filters, ldata, rdata);
        } else {
            cout << "Debug REQUEST from " << Sock::ipToString(ip) << endl;
            Response::writeDebug(conn, select, filters);
        }
    } // while()
} // main()
