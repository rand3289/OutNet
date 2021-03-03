#include "crawler.h"
#include "sock.h"
#include "protocol.h"
#include <memory>
#include <algorithm>
#include <mutex>
#include <shared_mutex> // since C++17
#include <thread>
#include <iostream>
#include <sstream>
#include <cstring>
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
        hi.missed = system_clock::now();
        ++hi.offlineCount;
        return 0;
    }
    hi.offlineCount = 0;
    hi.seen = system_clock::now();
    ulock.unlock();

    const int SELECTION_ALL = 0b111111111; // see SELECTION in protocol.h
    // do we have the service's key? no need to request it.   do we have services???
    ss << "GET /?QUERY=" << SELECTION_ALL << " HTTP/1.1\n";
    sock.write(ss.str().c_str(), ss.str().length());


// parse the response into unverifiedData
// verify signature if server sent
// if we requested signature, and the server did not send it, rating -100, dispose of data
// read PubKey, services and wait till end of parsing / signature verification to lock and copy to "hi"
    
    // TODO: read HTTP/1.1 200 OK\nServer: n3+1\n\n
    char buff[128];
    sock.readLine(buff, sizeof(buff)); // TODO: check for errors
    if( nullptr == strstr(buff,"200") ) {
        cerr << "Error in queryRemoteService() while parsing: " << buff << endl;
        return 0;
    }
    sock.readLine(buff, sizeof(buff)); // skip empty line
    unsigned int select;
    sock.read( (char*) &select, sizeof(select)); // TODO: check for errors
    select = ntohl(select);

    // local data
    LocalData ld;
    if(select & SELECTION::LKEY){ // send local public key. It does not change. Do not lock.
        sock.read((char*)&ld.localPubKey, sizeof(PubKey)); // TODO: check for errors
    }
    if(select & SELECTION::TIME){ // send local public key. It does not change. Do not lock.
        unsigned long time = 0;
        sock.read((char*)&time, sizeof(time)); // TODO: check for errors
        time = ntohl(time);
        // TODO: check that time is not too long in the past ... otherwise it is a replay attack
    }

    if( select & SELECTION::LSVC ){
        unsigned short count;
        sock.read((char*)&count, sizeof(count)); // TODO: check for errors
        count = ntohs(count);
        string s;
        for(int i=0; i < count; ++i){
            readString(s);
            ld.addService(s);
        }
    }

    // remote data
    vector<HostInfo> unverifiedData;
    HostPort self; // our public/ routable ip  // TODO: fill it in !!!
    unsigned long count = 0; // count is always there even if data is not
    sock.read((char*)&count, sizeof(count)); // TODO: check for errors
    count = ntohl(count);
    for(int i=0; i< count; ++i){
        unverifiedData.emplace_back();
        HostInfo& hi = unverifiedData.back();

        if( select & SELECTION::IP ){
            sock.read((char*)&hi.host,sizeof(hi.host)); // TODO: check for errors
            hi.host = ntohl(hi.host); // TODO: should we do this to IP?
        }

        if( select & SELECTION::PORT ){
            sock.read((char*)&hi.port,sizeof(hi.port)); // TODO: check for errors
            hi.port = ntohs(hi.port);
        }

        if( hi.host == self.host && hi.port == self.port ){ // just found myself in the list of IPs
            unverifiedData.pop_back();
            continue;
        }
        if( select & SELECTION::AGE ){
            unsigned short age = 0;
            sock.read((char*)&age, sizeof(age)); // TODO: check for errors
            age = ntohs(age);
            hi.seen = system_clock::now() - minutes(age); // TODO: check some reserved values ???
        }
        if( select & SELECTION::RKEY ){
            sock.read( (char*)&hi.key, sizeof(hi.key) );
        }
        if( select & SELECTION::RSVC ){
            unsigned long cnt = 0;
            sock.read((char*)&cnt, sizeof(cnt)); // TODO: check for errors
            cnt = ntohl(cnt);
            string s;
            for(int i=0; i< cnt; ++i){
                readString(s); // TODO: check for errors
                hi.addService(s);
            }
        }
    } // for ( adding HostInfo)

    unique_lock ulock2(data->mutx); // release lock after each connection for other parts to work
    hi.signatureVerified = true; // TODO: wait till info is received to mark it verified
    return 0; // TODO:
}


// go through RemoteData/HostInfo entries and retrieve more data from those services.
int Crawler::run(){
    if(nullptr == data){ throw "ERROR: call Crawler::loadFromDisk() before calling Crawler::run()!"; }

    while(true){
        vector<HostInfo> newData;
        vector<HostInfo*> callList;

        shared_lock slock(data->mutx);

        for(pair<const IPADDR,HostInfo>& ip_hi: data->hosts){
            HostInfo& hi = ip_hi.second;
            if( bwlists.blackListedIP(hi.host, hi.port) ){ continue; }
            if( system_clock::now() - hi.seen < minutes(60) ){ continue; } // do not contact often
            // contact hosts that have been seen offline in the past less often
            if( hi.offlineCount && (system_clock::now() - hi.missed < hi.offlineCount*minutes(60) ) ){ continue; }
            callList.push_back(&hi); // prepare to call that service up
        }

        std::sort( begin(callList), end(callList), [=](HostInfo* one, HostInfo* two){ return one->seen < two->seen; } );
        slock.unlock();

        // queryRemoteService(), main() and merge() modify individual HostInfo records
        // iterate over data, connect to each remote service, get the data and place into newData
        for(HostInfo* hi: callList){
            queryRemoteService(*hi, newData);
        }

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