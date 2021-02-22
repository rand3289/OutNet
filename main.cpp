// main service entry.
// three threads: one serves requests, second collects info, third subscribes services.
#include "data.h"
#include "http.h"
#include "sock.h"
#include "config.h"
#include "collect.h"
#include "servwatch.h"
#include <vector>
#include <string>
#include <thread>
using namespace std;
//#include <chrono>
//using namespace std::chrono;


int main(int argc, char* argv[]){
    LocalData ldata;
    RemoteData rdata;
    Config config; // config is aware of service port, LocalData and RemoteData
    config.loadFromDisk(ldata, rdata);
    // TODO: check success/fail, notify user on failure

    // create the information collector thread here (there could be many in the future)
    // when rdata is updated, it calls config.save(rdata);
    Collector collect(config, rdata);
    std::thread search( &Collector::run, &collect);

    // create a thread that registers services (from files and via connections)
    // when ldata is updated, it calls config.save(ldata);
    ServiceWatch watcher(config, ldata);
    std::thread watch( &ServiceWatch::run, &watcher);

    // create the server
    // first time start running on a random port (ANY_PORT)
    // but if service ran before, reuse the port number
    unsigned short port = config.getPort(Sock::ANY_PORT);
    Sock socket = Sock(port);
    port = socket.getPort(); // get bound server port number from socket
    config.save(port);
    socket.listen();

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