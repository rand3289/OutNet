#include "data.h"
#include "config.h"
#include "crawler.h"
#include "sock.h"
#include "http.h"
#include "utils.h"
#include "svc.h"
#include "log.h"
#include <vector>
#include <map> // #include <unordered_map>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <thread>
using namespace std;


std::string selectStr(uint32_t sel){
    std::stringstream ss;
    ss << sel << ": ";
    if(sel & SELECTION::LKEY){ ss << "LKEY "; }
    if(sel & SELECTION::TIME){ ss << "TIME "; }
    if(sel & SELECTION::SIGN){ ss << "SIGN "; }
    if(sel & SELECTION::LSVC){ ss << "LSVC "; }
    if(sel & SELECTION::IP  ){ ss << "IP ";   }
    if(sel & SELECTION::PORT){ ss << "PORT "; }
    if(sel & SELECTION::AGE ){ ss << "AGE ";  }
    if(sel & SELECTION::RKEY){ ss << "RKEY "; }
    if(sel & SELECTION::RSVC){ ss << "RSVC "; }
    if(sel & SELECTION::RSVCF){ss << "RSVCF ";}
    return ss.str();
}


bool startUp(RemoteData& rdata){
    const string filename = "start.url";
    ifstream urlf (filename);
    if( !urlf ){
        log() << filename << " NOT found." << endl;
        return false;
    }
    log() << "Loading " << filename << endl;

    vector<string> lines;
    parseLines(urlf, lines); // line format is IP:PORT
    int added = 0;

    for(string& line: lines){
        auto col = line.find(":");
        if(string::npos == col) { continue; }

        string ips = line.substr(0,col);
        string ports = line.substr(col+1);
        uint32_t ip = Sock::stringToIP(ips.c_str());
        uint16_t portInt = (uint16_t) strtol(ports.c_str(), nullptr, 10); // base 10

        if( 0!=ip && 0!=portInt ){
            log() << "Adding " << Sock::ipToString(ip) << ":" << portInt << " to list of IPs to scan." << endl;
            rdata.addContact(ip, portInt);
            ++added;
        }
    }

    return added > 0;
}


void run();
// Three threads: main one serves requests, second (crawler) collects info, 
// third thread subscribes services & loads black lists.
int main(int argc, char* argv[]){
    if( !initNetwork() ){ // WSAStartup() on windows or set ignore SIGPIPE on unix
        logErr() << "Error initializing network." << endl;
        return 4;
    }
    return registerService(&run);
} // main


void run(){
    log() << "OutNet service version 0.1 (" << __DATE__ << ")" << endl;
    // LocalData, RemoteData and BlackLists are shared among threads.  They all have internal mutexes.
    LocalData ldata;  // info about local services and local public key
    RemoteData rdata; // information gathered about remote services
    BlackList blist;  // Black and White lists
    startUp(rdata); // load remote outnet service location to initialize from
    Config config; // config is aware of service port, LocalData and BWLists
    config.init(ldata, blist); // load ldata,bwlists

    if( !Signature::loadKeys(ldata.localPubKey) ){ // load public key from disk into ldata
        logErr() << "ERROR loading keys.  Exiting." << endl;
        return;
    }
    auto& os = log();
    os << "My public key: ";
    printHex(os, ldata.localPubKey.key, sizeof(ldata.localPubKey) );
    os << endl;

    // create the server returning all queries
    // first time it starts running on a random port (ANY_PORT)
    // but if service ran before, reuse the port number.  It was saved by Config class.
    Sock server;
    if( server.listen(ldata.myPort) < 0 ){
        logErr() << "ERROR listening for connections on port " << ldata.myPort << ".  Exiting." << endl;
        return;
    }

    uint16_t port = server.getPort(); // get bound server port number from socket
    if( ldata.myPort != port ){       // Assuming there was no config file since ports are different.
        ldata.myPort = port;          // no need to lock since only one thread is running so far
        config.saveToDisk();          // this will overwrite the whole config file!
    }
    log() << "Running OutNet service on port " << port << endl;

    if( !Sock::isRoutable(ldata.localIP) ){ // could have a non-private (routable) IP
        if( config.forwardLocalPort(port) ){
            log() << "Opened port " << port << " on the router (enabled port forwarding)." << endl;
        } else {
            logErr() << "If you have a NAT router, forward port " << port << " to local host manually!" << endl;
        }
    }

    Crawler crawler(ldata, rdata, blist);
    crawler.loadRemoteDataFromDisk(); // load rdata
    log() << "============================================================" << endl; // starting threads

    // create the information collector thread here (there could be many in the future)
    // it searches and fills rdata while honoring BlackLists
    std::thread search( &Crawler::run, &crawler);

    // create a thread that watches files for service and BWList updates
    std::thread watch( &Config::watch, &config);

    // unordered_map::operator[] crashes.  Switching to map for now.
    // unordered_map<uint32_t, system_clock::time_point> connTime; // keep track of when someone connects to us
    map<uint32_t, system_clock::time_point> connTime; // keep track of when someone connects to us
    Response response;
    while( keepRunning() ){
        Sock conn;
        if( INVALID_SOCKET == server.accept(conn) ){ continue; }
        conn.setRWtimeout(ldata.timeoutServer); // seconds read/write timeout

        uint32_t ip = conn.getIP();
        if( blist.isBanned(ip) ){
            log() << "Denying REQUEST from " << Sock::ipToString(ip) << " (BANNED)" << endl;
            Response::writeDenied(conn, "BANNED");
            continue;
        }

        // prevent abuse if host connects too often but allow local IPs repeated queries
        bool routable = Sock::isRoutable(ip);
        auto now = system_clock::now();
        auto& time = connTime[ip];
        if( time > now - minutes(10) && routable ){
            log() << "Denying REQUEST from " << Sock::ipToString(ip) << " (connects too often)" << endl;
            Response::writeDenied(conn, "DENIED");
            continue;
        }
        time = now;

        // parse remote server port, "SELECT field bitmap" and filters from remote's "HTTP get" query
        vector<array<string,3>> filters; // function + parameter pairs
        uint16_t port = 0;
        int select = Request::parse(conn, filters, port);

        if(!routable){ // local services can ask to register themselves in HTTP request when querying
            Request::registerServices(conn, ldata);
        }

        if(port > 0){ // remote sent us its server port.  Add accepted connection to RemoteData
            rdata.addContact(conn.getIP(), port );
        }

        if(select > 0){ // request parsed correctly and we have a "data selection bitmap"
            log() << "REQUEST from " << Sock::ipToString(ip) << " (" << selectStr(select) << ")" << endl;
            response.write(conn, select, filters, ldata, rdata);
        } else {
            log() << "Debug REQUEST from " << Sock::ipToString(ip) << endl;
            Response::writeDebug(conn, select, filters);
        }

        // windblows freaks out in recv() if you close connection
        // TODO: keep a list of active sockets for a few seconds. for now, this is the solution:
        for(int i=0; i < ldata.sleepServer; ++i){
            if( !keepRunning() ){ return; }
            this_thread::sleep_for(seconds(1));
        }
    } // while()
} // run()
