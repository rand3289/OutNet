#include "crawler.h"
#include "sock.h"
#include "http.h"
#include "utils.h"
#include "log.h"
#include "svc.h"
#include <memory>
#include <algorithm>
#include <unordered_map> // erase_if(unordered_multimap, predicate)
#include <mutex>
#include <shared_mutex> // since C++17
#include <thread>
#include <iostream>
#include <fstream> // ofstream
#include <sstream>
#include <cstring>
using namespace std;


int ERR(const string& msg){ // makes error handling code a bit shorter
    logErr() << "ERROR " << msg << endl;
    return 0;
}


// filters is optional. It defaults to nullptr.
int Crawler::queryRemoteService(HostInfo& hi, vector<HostInfo>& newData, uint32_t select, vector<string>* filters){
    shared_lock slock(rdata.mutx); // TODO: construct query in run() to avoid locking here ??? 
    uint32_t hiHost = hi.host;     // make local copies so we can release slock
    uint16_t hiPort = hi.port;

    shared_ptr<PubKey> locPubKey;
    // if we have remote's public key, do not request it (turn it off in select)
    if( hi.signatureVerified ){ // Do not go by pure existence of a key.  It has to be verified!
        locPubKey = hi.key;
        turnBitsOff(select, SELECTION::LKEY);
    } // if signature remote sends fails to verify, next time we request the key again

    stringstream ss;
    ss << "GET /?QUERY=" << select;
    // add "filter by time" if remote was contacted before. Get new data only.
    if( 0 == hi.offlineCount && hi.seen > system_clock::from_time_t(0) ){
        int ageMinutes = (int) duration_cast<minutes>(system_clock::now() - hi.seen).count();
        ss << "&AGE_LT_" << ageMinutes;
    }
    slock.unlock(); // this is the last place where we read hi.blah

    if(portCopy>0){ // ad own server port for remote to connect back to
        ss << "&SPORT=" << portCopy;
    }
    if(filters){ // did the caller include any query parameters?
        for(string& f: *filters){ ss << "&" << f; }
    }
    ss << " HTTP/1.1\r\n\r\n";

    log() << "Crawler connecting to " << Sock::ipToString(hiHost) << ":" << hiPort << endl;
    Sock sock;
    sock.setRWtimeout(ldata.timeoutCrawler); // seconds read/write timeout
    sock.setNoDelay(true); // request is one write()

    if( sock.connect(hiHost, hiPort) ){
//        logErr()  << "ERROR connecting to " << Sock::ipToString(hiHost) << ":" << hiPort << endl;
        unique_lock ulock(rdata.mutx); // release lock after each connection for other parts to work
        hi.missed = system_clock::now();
        ++hi.offlineCount;
        --hi.rating;
        return 0;
    }

    int len = (int) ss.str().length();
    if(len != sock.write(ss.str().c_str(), len ) ){
        return ERR("sending HTTP request");
    }

    // parse the response into unverifiedData
    // if we requested signature, and the server did not send it, rating -100, dispose of data
    // read PubKey, services and wait till end of parsing / signature verification to lock and copy to "hi"

    // read HTTP/1.1 200 OK\r\n\r\n
    char buff[256];
    int rdsize = sock.readLine(buff, sizeof(buff) );
    if( rdsize < 8 || nullptr == strstr(buff,"200") ) {
        logErr() << "ERROR in queryRemoteService() while parsing " << rdsize << " bytes: " << buff << endl;
        return 0;
    }
    while( sock.readLine(buff, sizeof(buff) ) > 0 ) {} // skip till empty line is read (HTTP protocol)

/************ Everything read below this line is signed *************/
    const bool sign = select & SELECTION::SIGN; // is signature requested?
    if(sign){ signer.init(); }

    uint32_t selectRet; // SELECT is always received
    rdsize = sock.read(&selectRet, sizeof(selectRet) );
    if(rdsize != sizeof(selectRet)){
        return ERR("reading 'select'");
    }
    if(sign){ signer.write(&selectRet, sizeof(selectRet)); } // signer gets data in network byte order

    selectRet = ntohl(selectRet);
    if( sign && !(selectRet & SELECTION::SIGN) ){ // we requested a signature but it was not returned
        return ERR("remote refused to sign response"); // this is a security problem
    }

    if(selectRet & SELECTION::LKEY){
        locPubKey = make_shared<PubKey>();
        rdsize = sock.read(&*locPubKey, sizeof(PubKey));
        if( rdsize != sizeof(PubKey) ){
            return ERR("reading remote public key");
        }
        if( blist.isBanned(*locPubKey) ){
            return ERR("key is banned. Disconnecting!");
        }
        if(sign){ signer.write(&*locPubKey, sizeof(PubKey)); }
    }

    if(selectRet & SELECTION::TIME){
        int32_t timeRemote;
        rdsize = sock.read(&timeRemote, sizeof(timeRemote) );
        if(rdsize != sizeof(timeRemote) ){
            return ERR("reading remote's time.");
        }
        if(sign){ signer.write(&timeRemote, sizeof(timeRemote)); }
        timeRemote = ntohl(timeRemote);

        // check that timestamp is not too long in the past, otherwise it can be a replay attack
        int32_t minOld = timeMinutes() - timeRemote; // does not depend on a timezone
        if( abs(minOld) > 5 ){
            return ERR("Remote time difference is " + to_string(minOld) + " minutes.  Discarding data.");
        }
    }

    vector<string> lservices;
    if( selectRet & SELECTION::LSVC ){
        uint16_t count;
        rdsize = sock.read(&count, sizeof(count));
        if(rdsize != sizeof(count) ){
            return ERR("reading remote service count.");
        }
        if(sign){ signer.write(&count, sizeof(count)); }
        count = ntohs(count);

        for(int i=0; i < count; ++i){
            rdsize = sock.readString(buff, sizeof(buff));
            if(rdsize <=0){
                return ERR("reading remote serices.");
            }
            lservices.push_back(buff);
            if(sign){ signer.writeString(buff); }
        }
    }

/******************** remote data *************************/
    uint32_t count;
    rdsize = sock.read(&count, sizeof(count) ); // count is always there even if data is not
    if(rdsize != sizeof(count) ){
        return ERR("reading HostInfo count.");
    }
    if(sign){ signer.write(&count, sizeof(count)); }
    count = ntohl(count);

    vector<HostInfo> unverifiedData;
    for(uint32_t i=0; i< count; ++i){
        HostInfo& hil = unverifiedData.emplace_back();
        hil.referIP = hiHost;
        hil.referPort = hiPort;
        hil.met = system_clock::now();
        bool discard = false; // if record is bad, continue reading but discard it at the end

        if( selectRet & SELECTION::IP ){ // IP does not need ntohl()
            rdsize = sock.read( &hil.host, sizeof(hil.host));
            if(rdsize != sizeof(hil.host)){
                return ERR("reading IP.");
            }
            if(sign){ signer.write(&hil.host, sizeof(hil.host) ); }
        }

        if( selectRet & SELECTION::PORT ){
            rdsize = sock.read(&hil.port, sizeof(hil.port) );
            if(rdsize != sizeof(hil.port) ){
                return ERR("reading port.");
            }
            if(sign){ signer.write(&hil.port, sizeof(hil.port)); }
            hil.port = ntohs(hil.port);
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
            uint16_t age;
            rdsize = sock.read(&age, sizeof(age) );
            if(rdsize != sizeof(age) ){
                return ERR("reading age.");
            }
            if(sign){ signer.write(&age, sizeof(age)); }
            age = ntohs(age);
            hil.seen = system_clock::now() - minutes(age); // TODO: check some reserved values ???
        }

        if( selectRet & SELECTION::RKEY ){
            unsigned char keyCount = 0;
            rdsize = sock.read( &keyCount, sizeof(keyCount) );
            if( rdsize != sizeof(keyCount) ){
                return ERR("reading key count.");
            }
            if(sign){ signer.write(&keyCount, sizeof(keyCount)); }
            if(keyCount){ // for now only one key is allowed
                hil.key = make_shared<PubKey> ();
                rdsize = sock.read( &*hil.key, sizeof(PubKey) );
                if(rdsize != sizeof(PubKey) ){
                    return ERR("reading public key.");
                }
                if(sign){ signer.write(&*hil.key, sizeof(PubKey)); }
                if( blist.isBanned(*hil.key) ) { discard = true; }
            }
        }

        if( selectRet & SELECTION::RSVC ){
            uint16_t cnt;
            rdsize = sock.read(&cnt, sizeof(cnt) );
            if(rdsize != sizeof(cnt) ){
                return ERR("reading remote service count.");
            }
            if(sign){ signer.write(&cnt, sizeof(cnt)); }
            cnt = ntohs(cnt);

            for(int i=0; i< cnt; ++i){
                rdsize = sock.readString(buff, sizeof(buff));
                if(rdsize <=0){
                    return ERR("reading remote serivces.");
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
            return ERR("reading signature from remote host");
        }

        // If signature can not be verified, discard the data and reduce hi.rating below 0
        if( !signer.verify(signature, *locPubKey) ){
            unique_lock ulock2(rdata.mutx); // release lock after each connection for other parts to work
//1 TODO: if keys are different, these are different hosts with the same IP !!!
            hi.signatureVerified = false;
            hi.key.reset(); // delete stored public key since signature verification failed
            hi.rating = hi.rating < 0 ? hi.rating-1 : -1;
            hi.offlineCount = 0;
            hi.seen = system_clock::now();
            return ERR("verifying signature");
        } else {
            auto& os = log();
            os << "Remote's public key: ";
            printHex(os, locPubKey->key, sizeof(PubKey));
        }
    }

    std::copy( move_iterator(begin(unverifiedData)), move_iterator(end(unverifiedData)), back_inserter(newData));

    // there is no upgrade mechanism to unique_lock.
    // release the lock after each connection to allow other threads to work
    unique_lock ulock2(rdata.mutx);
    if(sign) {
//1 TODO: if signatures are different, it's a different service!!!  Create a new HostInfo.
        if( !hi.signatureVerified && locPubKey ){ // (selectRet&SELECTION::LKEY) && !hi.key
            hi.key = locPubKey;
        }
        hi.signatureVerified = true; // mark signature verified
        ++hi.rating;                 // increase remote's rating for verifying signature
    }

    hi.offlineCount = 0;
    hi.seen = system_clock::now();
    for(string& ls: lservices){
        hi.addService(ls); // add newly found services to that host's service list
    }

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

    uint32_t select = 0b11111111111; // see SELECTION in http.h
    turnBitsOff(select, SELECTION::ISCHK); // signatureVerified is of interest to local services only
    turnBitsOff(select, SELECTION::RSVCF); // do not filter out remote services by protocol

    while( keepRunning() ){
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
        for(HostInfo* hi: callList){
            queryRemoteService(*hi, newData, select);
        }

        int count = merge(newData);
        if( count<=0 ){
            for(int i=0; i < ldata.sleepCrawler && keepRunning(); ++i){
                this_thread::sleep_for( seconds( 1 ) );
            }
        } else { saveRemoteDataToDisk(); } // found new services
    } // while( keepRunning() )

    log() << "Crawler thread exiting." << endl;
    return 0;
}


const string RDFile = "outnetPeers.save";
const string tempRDFile = "outnetPeers.tmp";


// Data is periodically saved.  When service is restarted, it is loaded back up
// RemoteData does not have to be locked here
int Crawler::loadRemoteDataFromDisk(){
    log() << "Loading remote data from disk." << endl;
    ifstream file(RDFile, std::ios::binary);
    if(!file.good()){
        log() << "Could not find remote data restore point " << RDFile << endl;
        return 0;
    }
    uint32_t size;
    file.read( (char*) &size, sizeof(size) );
    if(!file){
        logErr() << "Could not read data from " << RDFile << endl;
        return -1;
    }

    std::unordered_multimap<IPADDR, HostInfo> hosts;
    for(uint32_t i = 0; i < size; ++i){ // read "size" HostInfo structures
        HostInfo hi;
        file.read((char*) &hi.host, sizeof(hi.host) );
        file.read( (char*) &*hi.key,         sizeof(PubKey) );
        file.read( (char*) &hi.met,          sizeof(hi.met) );
        file.read( (char*) &hi.missed,       sizeof(hi.missed) );
        file.read( (char*) &hi.seen,         sizeof(hi.seen) );
        file.read( (char*) &hi.offlineCount, sizeof(hi.offlineCount) );
        file.read( (char*) &hi.port,         sizeof(hi.port) );
        file.read( (char*) &hi.rating,       sizeof(hi.rating) );
        file.read( (char*) &hi.referIP,      sizeof(hi.referIP) );
        file.read( (char*) &hi.referPort,    sizeof(hi.referPort) );
        unsigned char sigVer;
        file.read( (char*) &sigVer,          sizeof(sigVer) );
        hi.signatureVerified  = sigVer; // bool to uchar

        uint32_t ss;
        file.read( (char*) &ss, sizeof(ss) );
        for(uint32_t i=0; i < ss; ++i){
            string service = readString(file);
            hi.addService(service);
        }
        hosts.emplace( hi.host, move(hi) );
    } // for

    unique_lock ulock(rdata.mutx);
    rdata.hosts.swap(hosts);
    return 0;
}


// have to save everything overwriting old data since old data could be modified by merge()
int Crawler::saveRemoteDataToDisk(){ // save data to disk
    log() << "Saving remote data to disk." << endl;
    ofstream file(tempRDFile, std::ios::binary);
    shared_lock slock(rdata.mutx);
    uint32_t hs = rdata.hosts.size();
    file.write( (char*) &hs, sizeof(hs) );
    if(!file){ // check that the writes succeeded
        logErr() << "ERROR saving remote data to disk.  Do you have write permissions?" << endl;
        return -1;
    }

    for(auto& hip: rdata.hosts){
        HostInfo& hi = hip.second;
        file.write( (char*) &hi.host,         sizeof(hi.host) );
        file.write( (char*) &*hi.key,         sizeof(PubKey) );
        file.write( (char*) &hi.met,          sizeof(hi.met) );
        file.write( (char*) &hi.missed,       sizeof(hi.missed) );
        file.write( (char*) &hi.seen,         sizeof(hi.seen) );
        file.write( (char*) &hi.offlineCount, sizeof(hi.offlineCount) );
        file.write( (char*) &hi.port,         sizeof(hi.port) );
        file.write( (char*) &hi.rating,       sizeof(hi.rating) );
        file.write( (char*) &hi.referIP,      sizeof(hi.referIP) );
        file.write( (char*) &hi.referPort,    sizeof(hi.referPort) );
        unsigned char sigVer = hi.signatureVerified; // bool to uchar
        file.write( (char*) &sigVer,          sizeof(sigVer) );
        uint32_t ss = hi.services.size();
        file.write( (char*) &ss, sizeof(ss) );

        for(auto& serv: hi.services){
            writeString(file, serv.originalDescription);
        }
    }

    slock.unlock();
    if(!file){ 
        logErr() << "ERROR saving remote data to disk." << endl;
        return -2;
    }
    file.close();
    string fold = RDFile+".old";
    remove(fold.c_str());
    rename(RDFile.c_str(), fold.c_str() );
    rename(tempRDFile.c_str(), RDFile.c_str() );
    return 0;
}
