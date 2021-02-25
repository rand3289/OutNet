// main service entry.
// three threads: one serves requests, second collects info, third subscribes services.
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


int main(int argc, char* argv[]){
    LocalData ldata;
    RemoteData rdata;
    BWLists lists; // Black, White lists

    Config config(ldata, lists); // config is aware of service port, LocalData and BWLists
    config.loadFromDisk(); // TODO: check success/fail, notify user on failure or exit?
    // create a thread that watches files and watches for BWList updates
    std::thread watch( &Config::watch, &config);


    // create the information collector thread here (there could be many in the future)
    // it searches and fills rdata while honoring BWLists
    // when rdata is updated, it calls config.save(rdata);
    Crawler crawler(config, rdata, lists);
    crawler.loadRemoteDataFromDisk();
    // TODO: should crawler load rdata from disk instead of config?
    std::thread search( &Crawler::run, &crawler);


    // create the server
    // first time start running on a random port (ANY_PORT)
    // but if service ran before, reuse the port number
    unsigned short port = config.getPort(Sock::ANY_PORT);
    Sock socket = Sock();
    socket.listen(port);

    port = socket.getBoundPort(); // get bound server port number from socket
    config.savePort(port);
    cout << "Running on port " << ntohs(port) << endl;

    while(true){
        SOCKET connection = socket.accept();
        Sock conn(connection);
        
        Request req(conn);
        vector<string> filters; // function + parameter pairs
        int select = req.parseRequest(filters);
        // TODO: handle errors (select <= 0)

        Response resp(conn);
        resp.write(select, filters, ldata, rdata);

        conn.close();
    }
}