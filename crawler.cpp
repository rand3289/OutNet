#include "crawler.h"
#include "sock.h"
#include <memory>
#include <algorithm>
#include <mutex>
#include <shared_mutex> // since C++17
#include <thread>
#include <iostream>
#include <sstream>
using namespace std;


int Crawler::merge(vector<HostInfo>& newData){
    unique_lock ulock(data->mutx);
//    copy(make_move_iterator(newData.begin()), make_move_iterator(newData.end()), back_inserter(data->hosts));
    return 0;
}


int Crawler::queryRemoteService(HostInfo& hi, vector<HostInfo>& newData){
    stringstream ss;
    Sock sock;
    unique_lock ulock(data->mutx); // release lock after each connection for other parts to work

    if( sock.connect(hi.host, hi.port) ){
        ss << "Error connecting to " << hi.host << ":" << hi.port << endl;
        cerr << ss.str();
        ++hi.offline;
        return 0;
    }
     hi.offline = 0;
     hi.resetTime();

    const int SELECTION_ALL = 0b111111111; // see SELECTION in protocol.h
    ss << "GET /?QUERY=" << SELECTION_ALL << " HTTP/1.1\n";
    sock.write(ss.str().c_str(), ss.str().length());

    HostPort self;
    // TODO: prevent putting itself into the list

     hi.verified = true; // wait till info is received to mark it verified
    return 0; // TODO:
}


// go through RemoteData/HostInfo entries and retrieve more data from those services.
int Crawler::run(){
    if(nullptr == data){ throw "ERROR: call Crawler::loadFromDisk() before calling Crawler::run()!"; }

    while(true){
        vector<HostInfo> newData;
        vector<HostInfo*> callList;
        { // shared_lock scope
            shared_lock slock(data->mutx);

            for(pair<const IPADDR,HostInfo>& ip_hi: data->hosts){
                HostInfo& hi = ip_hi.second;
                if( bwlists.blackListedIP(hi.host, hi.port) ){ continue; }
                if( system_clock::now() - hi.seen < minutes(60) ){ continue; } // do not contact often
                // contact hosts that have been seen offline in the past less often
                if( hi.offline && (system_clock::now() - hi.missed < hi.offline*minutes(60) ) ){ continue; }
                callList.push_back(&hi); // prepare to call that service up
            }

            std::sort( begin(callList), end(callList), [=](HostInfo* one, HostInfo* two){ return one->seen < two->seen; } );
        } // end shared_lock scope

        // queryRemoteService(), main() and merge() modify individual HostInfo records
        // iterate over data, connect to each remote service, get the data and place into newData
        for(HostInfo* hi: callList){
            queryRemoteService(*hi, newData);
        }

// iterate over data->connections which are not in HostInfo lists (these services connected to us)
        int count = merge(newData);
        saveToDisk();
        if( count<=0 ){ this_thread::sleep_for(seconds(60)); }
    }
    return 0;
}


// Data is periodically saved.  When service is restarted, it is loaded back up
// RemoteData does not have to be locked here
int Crawler::loadFromDisk(RemoteData& rdata){
    data = &rdata;
    return 0;
}

int Crawler::saveToDisk(){ // save data to disk
    shared_lock slock(data->mutx);
    return 0;
}