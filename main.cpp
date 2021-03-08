// main service entry. Three threads:
// main one serves requests, second (crawler) collects info, 
// third subscribes services & loads black lists
#include "data.h"
#include "config.h"
#include "crawler.h"
#include "sock.h"
#include "http.h"
#include <vector>
#include <string>
#include <thread>
#include <iostream>
#include <cstring> // strstr()
using namespace std;


void parseCmd(RemoteData& rdata, int argc, char* exe, char* host, char* port){
    if(1 == argc) { return; } // no cmd line parameters
    if(3 == argc){
        uint32_t ip = Sock::strToIP(host);
        uint16_t portInt = atoi(port);
        if( 0!=ip && 0!=portInt ){
            return rdata.addContact(ip, portInt);
        }
    }
    cerr << "ERROR parsing command line parameters.  ";
    cerr << "usage: " << exe << " host port" << endl;
}


int main(int argc, char* argv[]){
    // LocalData, RemoteData  and BWLists are shared among threads.  They all have internal mutexes.
    LocalData ldata;  // info about local services and local public key
    RemoteData rdata; // information gathered about remote services
    BWLists bwlists;  // Black and White lists

    parseCmd(rdata, argc, argv[0], argv[1], argv[2]);

    char* pkey = ldata.localPubKey.loadFromDisk(); // load public key from disk into ldata
    if(!pkey){
        cerr << "ERROR loading public key!  Exiting." << endl;
        return -1;
    }

    Config config; // config is aware of service port, LocalData and BWLists
    config.loadFromDisk(ldata, bwlists); // load ldata,bwlists
    // create a thread that watches files for service and BWList updates
    std::thread watch( &Config::watch, &config);

    // create the server returning all queries
    // first time start running on a random port (ANY_PORT)
    // but if service ran before, reuse the port number
    Sock server;
    if( INVALID_SOCKET == server.listen(ldata.myPort) ){
        cerr << "ERROR listening for connections on port " << ldata.myPort << ".  Exiting." << endl;
        return 1;
    }

    ldata.myPort = server.getPort(); // get bound server port number from socket
    config.saveToDisk();
    cout << "Running on port " << ldata.myPort << endl;

    // create the information collector thread here (there could be many in the future)
    // it searches and fills rdata while honoring BWLists
    Crawler crawler(ldata, bwlists);
    crawler.loadFromDisk(rdata); // load rdata from disk
    std::thread search( &Crawler::run, &crawler);

    unordered_map<uint32_t, system_clock::time_point> connTime; // keep track of when someone connects to us
    Response response;
    while(true){
        Sock conn;
        if(INVALID_SOCKET == server.accept(conn) ){ continue; }

        auto& time = connTime[conn.getIP()]; // not taking port into account
        if( time > system_clock::now() - minutes(10) && Sock::isRoutable(conn.getIP()) ){
            Response::writeDenied(conn); // preventing abuse
            conn.close();                // if this host connects too often
            continue;                    // but allow local IPs repeated queries
        }
        time = system_clock::now();

        // parse remote server port, "SELECT field bitmap" and filters from remote's "HTTP get" query
        vector<string> filters; // function + parameter pairs
        uint16_t port = 0;
        uint32_t select = Request::parse(conn, filters, port);

        if(port > 0){ // remote sent us its server port.  Add accepted connection to RemoteData
            rdata.addContact(conn.getIP(), port );
        }

        if(select > 0){ // request parsed correctly and we have a "data selection bitmap"
            response.write(conn, select, filters, ldata, rdata, bwlists);
        } else {
            Response::writeDebug(conn, select, filters);
        }
    } // while()
} // main()