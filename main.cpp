// main service entry. Three threads:
// main one serves requests, second (crawler) collects info, 
// third subscribes services & updates config
#include "data.h"
#include "config.h"
#include "crawler.h"
#include "sock.h"
#include "http.h"
#include <vector>
#include <string>
#include <thread>
#include <iostream>
using namespace std;


int main(int argc, char* argv[]){
    LocalData ldata;  // LocalData, RemoteData  and BWLists are shared among threads
    RemoteData rdata; // They all have internal mutexes
    BWLists bwlists;  // Black and White lists

    Config config; // config is aware of service port, LocalData and BWLists
    config.loadFromDisk(ldata, bwlists); // load ldata,bwlists // TODO: check failure, notify or exit?
    // create a thread that watches files for service and BWList updates
    std::thread watch( &Config::watch, &config);

    // create the information collector thread here (there could be many in the future)
    // it searches and fills rdata while honoring BWLists
    Crawler crawler(bwlists);
    crawler.loadFromDisk(rdata); // load rdata from disk
    std::thread search( &Crawler::run, &crawler);

    // create the server returning queries
    // first time start running on a random port (ANY_PORT)
    // but if service ran before, reuse the port number
    Sock server;
    unsigned short port = config.getPort(Sock::ANY_PORT);
    if( INVALID_SOCKET == server.listen(port) ){
        cerr << "ERROR listening for connections on port " << port << ".  Exiting." << endl;
        return 1;
    }

    port = server.getBoundPort(); // get bound server port number from socket
    config.savePort(port);
    cout << "Running on port " << ntohs(port) << endl;

    Response response;
    Sock conn;
    while(true){
        if(INVALID_SOCKET == server.accept(conn) ){ continue; }

        vector<string> filters; // function + parameter pairs
        int select = Request::parseRequest(conn, filters);

        if(select > 0){ // request parsed correctly and we have a data bitmap
            response.write(conn, select, filters, ldata, rdata, bwlists);
        } else {
            Response::writeDebug(conn, select, filters);
        }
        conn.close();
    } // while()
} // main()