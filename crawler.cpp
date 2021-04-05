#include "crawler.h"
#include "sock.h"
#include "http.h"
#include "utils.h"
#include <memory>
#include <algorithm>
#include <unordered_map> // erase_if(unordered_multimap, predicate)
#include <mutex>
#include <shared_mutex> // since C++17
#include <thread>
#include <iostream>
#include <sstream>
#include <cstring>
using namespace std;


// filters is optional. It defaults to nullptr.
int Crawler::queryRemoteService(HostInfo& hi, vector<HostInfo>& newData, uint32_t select, vector<string>* filters){
    shared_lock slock(rdata.mutx);

    cout << "Crawler connecting to " << Sock::ipToString(hi.host) << ":" << hi.port << endl;
    Sock sock;

    if( sock.connect(hi.host, hi.port) ){
        cerr  << "ERROR connecting to " << Sock::ipToString(hi.host) << ":" << hi.port << endl;
        slock.unlock(); // there is no upgrade mechanism to unique_lock.
        unique_lock ulock(rdata.mutx); // release lock after each connection for other parts to work
        hi.missed = system_clock::now();
        ++hi.offlineCount;
        --hi.rating;
        return 0;
    }

    sock.setRWtimeout(5); // seconds read/write timeout // TODO: add it to config???

    // if we have remote's public key, do not request it (turn it off in select)
    if( hi.signatureVerified ){ // Do not go by pure existence of a key.  It has to be verified!
        turnBitsOff(select, SELECTION::LKEY);
    } // if signature remote sends fails to verify, next time we request the key again

    stringstream ss;
    ss << "GET /?QUERY=" << select;
    if(portCopy>0){ // ad own server port for remote to connect back to
        ss << "&SPORT=" << portCopy;
    }
    // add "filter by time" if remote was contacted before. get new data only
    if( 0 == hi.offlineCount && hi.seen > system_clock::from_time_t(0) ){
        int ageMinutes = (int) duration_cast<seconds>(system_clock::now() - hi.seen).count();
        ss << "&AGE_LT_" << ageMinutes;
    }
    if(filters){ // did the caller include any query parameters?
        for(string& f: *filters){ ss << "&" << f; }
    }
    ss << " HTTP/1.1\r\n";


    int len = (int) ss.str().length();
    if(len != sock.write(ss.str().c_str(), len ) ){
        cerr << "ERROR sending HTTP request." << endl;
        return 0;
    }

    // parse the response into unverifiedData
    // if we requested signature, and the server did not send it, rating -100, dispose of data
    // read PubKey, services and wait till end of parsing / signature verification to lock and copy to "hi"

    // read HTTP/1.1 200 OK\r\n\r\n
    char buff[256];
    int rdsize = sock.readLine(buff, sizeof(buff) );
    if( rdsize < 8 || nullptr == strstr(buff,"200") ) {
        cerr << "ERROR in queryRemoteService() while parsing: " << buff << endl;
        return 0;
    }
    while( sock.readLine(buff, sizeof(buff) ) ) {} // skip till empty line is read (HTTP protocol)

/************ Everything read below this line is signed *************/
    const bool sign = select & SELECTION::SIGN; // is signature requested?
    if(sign){ signer.init(); }

    bool error = false;
    uint32_t selectRet = sock.read32(error);
    if(error){
        cerr << "ERROR reading 'select'" << endl;
        return 0;
    }
    signer.write(&selectRet, sizeof(selectRet)); // always received

// TODO: allow local services to add themselves by connecting to outnet with SPORT= set to their port
// when outnet connects to it, allow replies without signing the response. (sign=false;)
// when local (non-routable) address is detected, do not enter it as a remote service but local.

    if( sign && !(selectRet & SELECTION::SIGN) ){ // we requested a signature but it was not returned
        cerr << "ERROR: remote refused to sign response." << endl; // this is a security problem
        return 0;
    }

    shared_ptr<PubKey> locPubKey;
    if(selectRet & SELECTION::LKEY){
        locPubKey = make_shared<PubKey>();
        rdsize = sock.read(&*locPubKey, sizeof(PubKey));
        if( rdsize != sizeof(PubKey) ){
            cerr << "ERROR: reading remote public key" << endl;
            return 0;
        }
        if( blist.isBanned(*locPubKey) ){
            cerr << "ERROR: key is banned. Disconnecting!" << endl;
            return 0;
        }
        if(sign){ signer.write(&*locPubKey, sizeof(PubKey)); }
    }

    if(selectRet & SELECTION::TIME){
        uint32_t timeRemote = sock.read32(error);
        if(error){
            cerr << "ERROR reading remote's time." << endl;
            return 0;
        }
        if(sign){ signer.write(&timeRemote, sizeof(timeRemote)); }
        // check that timestamp is not too long in the past, otherwise it can be a replay attack
        uint32_t now = (uint32_t) time(nullptr); // unix time does not depend on a timezone
        if( now - timeRemote > 10*60 ){
            cerr << "ERROR: remote time stamp is older than 10 minutes!  Discarding data." << endl;
            return 0;
        }
    }

    vector<string> lservices;
    if( selectRet & SELECTION::LSVC ){
        uint16_t count = sock.read16(error);
        if(error){
            cerr << "ERROR reading remote service count." << endl;
            return 0;
        }
        if(sign){ signer.write(&count, sizeof(count)); }

        for(int i=0; i < count; ++i){
            rdsize = sock.readString(buff, sizeof(buff));
            if(rdsize <=0){
                cerr << "ERROR reading remote serices." << endl;
                return 0;
            }
            lservices.push_back(buff);
            if(sign){ signer.writeString(buff); }
        }
    }

    // remote data
    uint32_t count = sock.read32(error); // count is always there even if data is not
    if(error){
        cerr << "ERROR reading HostInfo count." << endl;
        return 0;
    }
    if(sign){ signer.write(&count, sizeof(count)); }

    vector<HostInfo> unverifiedData;
    for(uint32_t i=0; i< count; ++i){
        HostInfo& hil = unverifiedData.emplace_back();
        hil.referIP = hi.host;
        hil.referPort = hi.port;
        hil.met = system_clock::now();
        bool discard = false; // if record is bad, continue reading but discard it at the end

        if( selectRet & SELECTION::IP ){ // do not use Sock::read32() - IP does not need ntohl()
            rdsize = sock.read( &hil.host, sizeof(hil.host));
            if(rdsize != sizeof(hil.host)){
                cerr << "ERROR reading IP." << endl;
                return 0; // discard ALL data from that server because we can not verify signature!
            }
            if(sign){ signer.write(&hil.host, sizeof(hil.host) ); }
        }

        if( selectRet & SELECTION::PORT ){
            hil.port = sock.read16(error);
            if(error){
                cerr << "ERROR reading port." << endl;
                return 0;
            }
            if(sign){ signer.write(&hil.port, sizeof(hil.port)); }
        }

        if( selectRet&SELECTION::IP && selectRet&SELECTION::PORT ){
            if( hil.host==0 || hil.port==0 || !Sock::isRoutable(hil.host) ){
                discard = true; // throw away services with invalid host or port
            }

            if( hil.host==hostCopy && hil.port==portCopy){
                discard = true; // just found myself in the list of IPs
            }
        }

        if( selectRet & SELECTION::AGE ){
            uint16_t age = sock.read16(error);
            if(error){
                cerr << "ERROR reading age." << endl;
                return 0;
            }
            if(sign){ signer.write(&age, sizeof(age)); }
            hil.seen = system_clock::now() - minutes(age); // TODO: check some reserved values ???
        }

        if( selectRet & SELECTION::RKEY ){
            char keyCount = 0;
            rdsize = sock.read( &keyCount, sizeof(keyCount) );
            if( sizeof(keyCount) != rdsize ){
                cerr << "ERROR reading key count." << endl;
                return 0;
            }
            if(sign){ signer.write(&keyCount, sizeof(keyCount)); }
            if(keyCount){ // for now only one key is allowed
                hil.key = make_shared<PubKey> ();
                rdsize = sock.read( &*hil.key, sizeof(PubKey) );
                if(rdsize != sizeof(PubKey) ){
                    cerr << "ERROR reading public key." << endl;
                    return 0;
                }
                if(sign){ signer.write(&*hil.key, sizeof(PubKey)); }
                if( blist.isBanned(*hil.key) ) { discard = true; }
            }
        }

        if( selectRet & SELECTION::RSVC ){
            uint16_t cnt = sock.read16(error);
            if(error){
                cerr << "ERROR reading remote service count." << endl;
                return 0;
            }
            if(sign){ signer.write(&cnt, sizeof(cnt)); }
            for(int i=0; i< cnt; ++i){
                rdsize = sock.readString(buff, sizeof(buff));
                if(rdsize <=0){
                    cerr << "ERROR reading remote serivces." << endl;
                    return 0;
                }
                if(sign){ signer.writeString(buff); }
                hil.addService(buff);
            }
        }

        if( discard ){
            unverifiedData.pop_back();
        }
    } // for (adding HostInfo)

    if(sign){
        PubSign signature;
        if( sizeof(signature) != sock.read( &signature, sizeof(signature) ) ) {
            cerr << "ERROR reading signature from remote host" << endl;
            return 0;
        }

        PubKey* key = nullptr;
        if(selectRet & SELECTION::LKEY) { // use new key if available
            key = locPubKey.get();
        } else if(hi.key) { // otherwise use the old key if it exists
            key = hi.key.get(); // shared_ptr to regular pointer
        } else {
            cerr << "ERROR no key available for signature verification." << endl;
            return 0;
        }

        // If signature can not be verified, discard the data and reduce hi.rating below 0
        if( !signer.verify(signature, *key) ){
            slock.unlock(); // there is no upgrade mechanism to unique_lock.
            unique_lock ulock2(rdata.mutx); // release lock after each connection for other parts to work
// TODO: if keys are different, these are different hosts with the same IP !!!
            hi.signatureVerified = false;
            hi.key.reset(); // delete stored public key since signature verification failed
            hi.rating = hi.rating < 0 ? hi.rating-1 : -1;
            hi.offlineCount = 0;
            hi.seen = system_clock::now();
            return 0;
        }
    }

    slock.unlock();
    std::copy( move_iterator(begin(unverifiedData)), move_iterator(end(unverifiedData)),
            back_inserter(newData));
    unique_lock ulock2(rdata.mutx); // there is no upgrade mechanism to unique_lock.
    // release the lock after each connection to allow other threads to work

    if(sign) {
// TODO: if signatures are different, it's a different service!!!  Create a new HostInfo.
        if( !hi.signatureVerified && locPubKey ){ // (selectRet&SELECTION::LKEY) && !hi.key
            hi.key = locPubKey;
        }
        hi.signatureVerified = true; // mark signature verified
        ++hi.rating;                 // increase remote's rating for verifying signature
    }

    for(string& ls: lservices){
        hi.addService(ls); // add newly found services to that host's service list
    }

    hi.offlineCount = 0;
    hi.seen = system_clock::now();

    return (int) unverifiedData.size();
}


// merge new HostInfo from newData into rdata.hosts
// keep track of services that change IP by key and merge them with new IP
int Crawler::merge(vector<HostInfo>& newData){
    int newCount = 0;
    unique_lock ulock(rdata.mutx);

    for(HostInfo& hiNew: newData){
        bool found = false;

        // iterate through mmap values with the same key(IP)
        for(auto hi = rdata.hosts.find(hiNew.host); hi != end(rdata.hosts); ++hi){
            if(hi->second.port == hiNew.port){      // existing service (ip/port match) - merge info
                if( !hi->second.key && hiNew.key ){ // existing record does not have a key
                    hi->second.key = hiNew.key;     // keys are shared_ptr
                }
                mergeServices(hi->second.services, hiNew.services);
                found = true;
                break;
            }
        }
        if( found ){ continue; } // continue iterating over newData

        auto it = rdata.hosts.emplace( hiNew.host, move(hiNew) ); // insert new HostInfo record
        auto hinew = it->second; // hiNew is no longer valid after move. use hinew

        // try matching services by public key if IP or port changed
        // since it has a new ip, use inserted entry (hinew) and delete old
        for(auto& hi_pit: rdata.hosts){
            auto hi = hi_pit.second;
            if( hi.host != hinew.host && *hi.key == *hinew.key ){ // keys are shared_ptr
                mergeServices(hinew.services, hi.services);
                hinew.signatureVerified = hi.signatureVerified;
                hinew.offlineCount = hi.offlineCount;
                hinew.rating = hi.rating;
                hinew.met = move(hi.met);
                hinew.seen = move(hi.seen);
                hinew.missed = move(hi.missed);
                hinew.referIP = hi.referIP;
                hinew.referPort = hi.referPort;
                found = true;
                hi.host = 0; // mark the old HostInfo entry for deletion
                break;
            }
        }

        if(found){ // if found, old HostInfo was found by KEY and it was marked for deletion
            erase_if(rdata.hosts, [](auto& hi){ return 0==hi.second.host; } ); // delete old entries
        } else {
            ++newCount;
        }
    }

    return newCount;
}


// go through RemoteData/HostInfo entries and retrieve more data from those services.
int Crawler::run(){
    shared_lock slock(ldata.mutx);
    hostCopy = ldata.myIP;
    portCopy = ldata.myPort;
    slock.unlock();

    while(true){
        vector<HostInfo> newData;
        vector<HostInfo*> callList;

        shared_lock slock(rdata.mutx); // remote data

        for(pair<const IPADDR,HostInfo>& ip_hi: rdata.hosts){
            HostInfo& hi = ip_hi.second;
            if( hi.rating < 0){ continue; }
            if( system_clock::now() - hi.seen < minutes(60) ){ continue; } // do not contact often
            // contact hosts that have been seen offline in the past less often
            if( hi.offlineCount && (system_clock::now() - hi.missed < hi.offlineCount*minutes(60) ) ){ continue; }
            if( blist.isBanned(hi.host) ){ continue; }
            callList.push_back(&hi); // prepare to call that service up
        }

        // Sort pointers to rdata.hosts  No need for exclusive_lock.
        // only queryRemoteService(), main() and merge() modify individual HostInfo records
        std::sort( begin(callList), end(callList), [=](HostInfo* one, HostInfo* two){ return one->seen < two->seen; } );
        slock.unlock(); // remote data

        // iterate over data, connect to each remote service, get the data and place into newData
        const uint32_t select = 0b11111111111; // see SELECTION in http.h
        for(HostInfo* hi: callList){
            queryRemoteService(*hi, newData, select);
        }

        int count = merge(newData);
        if( count<=0 ){ this_thread::sleep_for( seconds( (rand()%10) +60 ) ); }
        else { saveRemoteDataToDisk(); } // found new services // TODO: save only modified/new records
    } // while(true)
    return 0;
}


// Data is periodically saved.  When service is restarted, it is loaded back up
// RemoteData does not have to be locked here
int Crawler::loadRemoteDataFromDisk(){
    cout << "Loading remote data from disk. (TODO)" << endl;
    return 0; // TODO:
}


int Crawler::saveRemoteDataToDisk(){ // save data to disk
    cout << "Saving remote data to disk. (TODO)" << endl;
    shared_lock slock(rdata.mutx);
    return 0; // TODO:
}
