// main service entry. Three threads:
// main one serves requests, second (crawler) collects info, 
// third subscribes services & updates config
#include "data.h"
#include "http.h"
#include "sock.h"
#include "config.h"
#include "crawler.h"
#include <vector>
#include <string>
#include <thread>
#include <iostream>
using namespace std;
//#include <chrono>
//using namespace std::chrono;

//#include <string.h> // strerror()
int main(int argc, char* argv[]){
//errno = 98;
//perror("testing perror:");
//cout << "strerror output:" << strerror(98) << endl;
//cout.flush();

    LocalData ldata;  // LocalData, RemoteData  and BWLists are shared among threads
    RemoteData rdata; // They all have internal mutexes
    BWLists lists;    // Black and White lists

    Config config(ldata, lists); // config is aware of service port, LocalData and BWLists
    config.loadFromDisk(); // load ldata // TODO: check failure, notify user or exit?
    // create a thread that watches files for service and BWList updates
    std::thread watch( &Config::watch, &config);

    // create the information collector thread here (there could be many in the future)
    // it searches and fills rdata while honoring BWLists
    // when rdata is updated, it calls config.save(rdata);
    Crawler crawler(config, rdata, lists);
    crawler.loadRemoteDataFromDisk(); // load rdata from disk
    std::thread search( &Crawler::run, &crawler);

    // create the server returning queries
    // first time start running on a random port (ANY_PORT)
    // but if service ran before, reuse the port number
    unsigned short port = config.getPort(Sock::ANY_PORT);
    Sock server;
    if( INVALID_SOCKET == server.listen(port) ){
        cerr << "ERROR listening for connections on port " << port << ".  Exiting." << endl;
        return 1;
    }

    port = server.getBoundPort(); // get bound server port number from socket
    config.savePort(port);
    cout << "Running on port " << ntohs(port) << endl;

    while(true){
        Sock conn;
        if(INVALID_SOCKET == server.accept(conn) ){ continue; }

        vector<string> filters; // function + parameter pairs
        int select = Request::parseRequest(conn, filters);

        if(select > 0){ // request parsed correctly and we have a data bitmap
            Response::write(conn, select, filters, ldata, rdata, lists);
        }
        conn.close();
    }
}