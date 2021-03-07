#include "crawler.h"
#include "sock.h"
#include "protocol.h"
#include "utils.h"
#include <memory>
#include <algorithm>
#include <mutex>
#include <shared_mutex> // since C++17
#include <thread>
#include <iostream>
#include <sstream>
#include <cstring>
using namespace std;


int Crawler::queryRemoteService(HostInfo& hi, vector<HostInfo>& newData, uint32_t select, HostPort& self){
    shared_lock slock(rdata->mutx);

    cout << "Connecting to " << Sock::ipToString(hi.host) << ":" << hi.port << endl;
    Sock sock;
    if( sock.connect(hi.host, hi.port) ){
        cerr  << "Error connecting to " << hi.host << ":" << hi.port << endl;
        slock.unlock(); // there is no upgrade mechanism to unique_lock.
        unique_lock ulock(rdata->mutx); // release lock after each connection for other parts to work
        hi.missed = system_clock::now();
        ++hi.offlineCount;
        --hi.rating;
        return 0;
    }

    // if we have remote's public key, do not request it (turn it off in select)
    if( hi.signatureVerified ){ // hi.key ???
        turnBitsOff(select, SELECTION::LKEY);
    } // if signature remote sends fails to verify, next time we request the key again
// TODO: track keys/ratings of services that change IP

    // add "filter by time" if remote was contacted some time ago
    stringstream filters; // what should we use to track of "first seen" for hosts ???
    if( 0 == hi.offlineCount && hi.seen > system_clock::from_time_t(0) ){
        int ageMinutes = duration_cast<seconds>(system_clock::now() - hi.seen).count();
        filters << "&AGE_LT_" << ageMinutes;
    }

    stringstream ss;
    ss << "GET /?QUERY=" << select << "&SPORT=" << self.port << filters.str() <<" HTTP/1.1\n";
    int len = ss.str().length();
    if(len != sock.write(ss.str().c_str(), len ) ){
        cerr << "Error sending HTTP request." << endl;
        return 0;
    }

    // parse the response into unverifiedData
    // verify signature if server sent
    // if we requested signature, and the server did not send it, rating -100, dispose of data
    // read PubKey, services and wait till end of parsing / signature verification to lock and copy to "hi"

    // read HTTP/1.1 200 OK\nServer: n3+1\n\n
    char buff[256];
    int rdsize = sock.readLine(buff, sizeof(buff)-1);
    if( rdsize < 8 || nullptr == strstr(buff,"200") ) {
        cerr << "Error in queryRemoteService() while parsing: " << buff << endl;
        return 0;
    }
    while( sock.readLine(buff, sizeof(buff) ) ) {} // skip till empty line is read (HTTP protocol)

    bool error = false;
    uint32_t selectRet = sock.read32(error);
    if(error){
        cerr << "ERROR reading 'select'" << endl;
        return 0;
    }
    if( (select & SELECTION::SIGN) && !(selectRet & SELECTION::SIGN) ){ // we requested a signature but it was not returned
        cerr << "ERROR: remote refused to sign response.  Disconnecting!" << endl; // this is a security problem
        return 0;
    }

    uint32_t myip = 0;
    if( selectRet & SELECTION::MYIP ){
        myip = sock.read32(error);
        if(error){
            cerr << "ERROR reading 'my ip'" << endl;
            return 0;
        }
        if( 0 == self.host ){ // TODO: ask multiple servers before trusting it
            self.host = myip;
        }
    }

    // local data
    LocalData ld;
    if(selectRet & SELECTION::LKEY){
        rdsize = sock.read(&ld.localPubKey, sizeof(PubKey));
        if( rdsize != sizeof(PubKey) ){
            cerr << "ERROR: reading remote public key" << endl;
            return 0;
        }
    }

    bool sign = selectRet & SELECTION::SIGN; // if signature is requested, use signatureReader to read
    Reader* reader = sign ? &signatureReader : &dumbReader;
    reader->init(sock, ld.localPubKey);

    reader->write(&selectRet, sizeof(selectRet)); // we read it using sock before
    if( selectRet & SELECTION::MYIP ){
        reader->write(&myip, sizeof(myip));
    }
    if(selectRet & SELECTION::LKEY){
        reader->write(&ld.localPubKey, sizeof(ld.localPubKey));
    }

    if(selectRet & SELECTION::TIME){
        uint32_t timeRemote = reader->read32(error);
        if(error){
            cerr << "ERROR reading remote's time." << endl;
            return 0;
        }
        // check that timestamp is not too long in the past, otherwise it can be a replay attack
        uint32_t now = time(nullptr); // unix time does not depend on a timezone
        if( now - timeRemote > 10*60 ){
            cerr << "ERROR: remote time stamp is older than 10 minutes!  Discarding data." << endl;
            return 0;
        }
    }

    if( selectRet & SELECTION::LSVC ){
        uint16_t count = reader->read16(error);
        if(error){
            cerr << "ERROR reading remote service count." << endl;
            return 0;
        }
        for(int i=0; i < count; ++i){
            rdsize = reader->readString(buff, sizeof(buff));
            if(rdsize <=0){
                cerr << "ERROR reading remote serices." << endl;
                return 0;
            }
            ld.addService(buff);
        }
    }

    // remote data
    uint32_t count = reader->read32(error); // count is always there even if data is not
    if(error){
        cerr << "ERROR reading HostInfo count." << endl;
        return 0;
    }

    vector<HostInfo> unverifiedData;
    for(uint32_t i=0; i< count; ++i){
        unverifiedData.emplace_back();
        HostInfo& hi = unverifiedData.back();

        if( selectRet & SELECTION::IP ){ // do not use Sock::read32() - IP does not need ntohl()
            rdsize = reader->read( &hi.host,sizeof(hi.host));
            if(rdsize != sizeof(hi.host)){
                cerr << "ERROR reading IP." << endl;
                return 0; // discard ALL data from that server because we can not verify signature!
            }
        }

        if( selectRet & SELECTION::PORT ){
            hi.port = reader->read16(error);
            if(error){
                cerr << "ERROR reading port." << endl;
                return 0;
            }
        }

        if( selectRet&SELECTION::IP && selectRet&SELECTION::PORT && hi.host==self.host && hi.port==self.port ){
            unverifiedData.pop_back(); // just found myself in the list of IPs
            continue;
        }

        if( selectRet & SELECTION::AGE ){
            uint16_t age = reader->read16(error);
            hi.seen = system_clock::now() - minutes(age); // TODO: check some reserved values ???
            if(error){
                cerr << "ERROR reading age." << endl;
                return 0;
            }
        }

        if( selectRet & SELECTION::RKEY ){
            char keyCount = 0;
            rdsize = reader->read( &keyCount, sizeof(keyCount) );
            if( sizeof(keyCount) != rdsize ){
                cerr << "ERROR reading key count." << endl;
                return 0;
            }
            if(keyCount){
                hi.key = make_shared<PubKey> ();
                rdsize = reader->read( &*hi.key, sizeof(PubKey) );
                if(rdsize != sizeof(PubKey) ){
                    cerr << "ERROR reading public key." << endl;
                    return 0;
                }
            }
        }

        if( selectRet & SELECTION::RSVC ){
            uint16_t cnt = reader->read16(error);
            if(error){
                cerr << "ERROR reading remote service count." << endl;
                return 0;
            }
            string s;
            for(int i=0; i< cnt; ++i){
                rdsize = reader->readString(buff, sizeof(buff));
                if(rdsize <=0){
                    cerr << "ERROR reading remote serivces." << endl;
                    return 0;
                }
                hi.addService(buff);
            }
        }
    } // for (adding HostInfo)

    PubSign signature; // we checked the SIGN bit above
    if( sizeof(signature) != sock.read( &signature, sizeof(signature) ) ) {
        cerr << "Crawler: ERROR reading signature from remote host" << endl;
        return 0;
    }

    // verify signature. If signature can not be verified, discard the data and reduce hi.rating below 0
    if( !reader->verifySignature(signature) ){
        unique_lock ulock2(rdata->mutx); // release lock after each connection for other parts to work
        hi.signatureVerified = false;
        hi.rating = hi.rating < 0 ? hi.rating-1 : -1;
        hi.offlineCount = 0;
        hi.seen = system_clock::now();
        return 0;
    }

    std::copy( begin(unverifiedData), end(unverifiedData), back_inserter(newData)); // TODO: use move_iterator?

    unique_lock ulock2(rdata->mutx); // release the lock after each connection to allow other threads to work
    hi.signatureVerified = true;
    ++hi.rating;
    hi.offlineCount = 0;
    hi.seen = system_clock::now();

    return unverifiedData.size();
}


int Crawler::merge(vector<HostInfo>& newData){
    unique_lock ulock(rdata->mutx);
//    copy(make_move_iterator(newData.begin()), make_move_iterator(newData.end()), back_inserter(data->hosts));
    return 0;
}


// go through RemoteData/HostInfo entries and retrieve more data from those services.
int Crawler::run(){
    if(nullptr == rdata){ throw "ERROR: call Crawler::loadFromDisk() before calling Crawler::run()!"; }

    HostPort self;
    shared_lock slock(ldata.mutx);
    self.host = ldata.myIP;
    self.port = ldata.myPort;
    slock.unlock();

    while(true){
        vector<HostInfo> newData;
        vector<HostInfo*> callList;

        shared_lock slock(rdata->mutx); // remote data

        for(pair<const IPADDR,HostInfo>& ip_hi: rdata->hosts){
            HostInfo& hi = ip_hi.second;
            if( hi.rating < 0){ continue; }
            if( system_clock::now() - hi.seen < minutes(60) ){ continue; } // do not contact often
            // contact hosts that have been seen offline in the past less often
            if( hi.offlineCount && (system_clock::now() - hi.missed < hi.offlineCount*minutes(60) ) ){ continue; }
            if( bwlists.blackListedIP(hi.host, hi.port) ){ continue; }
            callList.push_back(&hi); // prepare to call that service up
        }

        std::sort( begin(callList), end(callList), [=](HostInfo* one, HostInfo* two){ return one->seen < two->seen; } );
        slock.unlock();

        // queryRemoteService(), main() and merge() modify individual HostInfo records
        // iterate over data, connect to each remote service, get the data and place into newData
        const uint32_t select = 0b11111111111111111; // see SELECTION in protocol.h
        for(HostInfo* hi: callList){
            queryRemoteService(*hi, newData, select, self);
        }

        int count = merge(newData);
        if( count<=0 ){ this_thread::sleep_for( seconds( (rand()%10) +60 ) ); }
        else { saveToDisk(); } // found new services
    } // while(true)
    return 0;
}


// Data is periodically saved.  When service is restarted, it is loaded back up
// RemoteData does not have to be locked here
int Crawler::loadFromDisk(RemoteData& remoteData){
    rdata = &remoteData;
    return 0;
}


int Crawler::saveToDisk(){ // save data to disk
    shared_lock slock(rdata->mutx);
    return 0;
}
