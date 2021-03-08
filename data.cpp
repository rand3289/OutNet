#include "data.h"
#include "http.h"
#include "sign.h"
#include "protocol.h"
#include <memory> // shared_ptr
#include <string>
#include <vector>
#include <mutex>
#include <iostream>
#include <shared_mutex> // since C++17
using namespace std;
#include <chrono>
using namespace std::chrono;


HostInfo::HostInfo(): host(0), port(0), signatureVerified(false), offlineCount(0), rating(DEFAULT_RATING) {
    seen = missed = called = system_clock::from_time_t(0); // time_point::min() is broken (overflows) !!!
}


Service* HostInfo::addService(const string& service){
    auto s = services.emplace( services.end() )->parse(service);
    if(!s){ services.pop_back(); } // if it was not parsed, delete it
    return s;
}


Service* LocalData::addService(const string& service){
    auto s = services.emplace( services.end() )->parse(service);
    if(!s){ services.pop_back(); } // if it was not parsed, delete it
    return s;
}


Service* Service::parse(const string& service){
    originalDescription = service;
// TODO: translate local to NAT IP
    fullDescription = service;
// if(notParsed){ return nullptr; }
    return this;
}


bool Service::passLocalFilters(vector<string> filters){
    return true; // TODO:
}


bool Service::passRemoteFilters(vector<string> filters){
    return true; // TODO:
}


bool HostInfo::passFilters(vector<string> filters){
    return true; // TODO:
}


void RemoteData::addContact(IPADDR ip, uint16_t port){
    unique_lock ulock(mutx);

    for( auto range = hosts.equal_range(ip); range.first != range.second; ++range.first){
        HostInfo& hi = range.first->second;
        if( port == hi.port ) { // existing host
            hi.called = system_clock::now();
            return;
        }
    }

//    HostInfo& hi = hosts.emplace( ip )->second;
    HostInfo hi;
    hi.host = ip;
    hi.port = port;
    hi.called = system_clock::now();
    hi.referrer.host = ip; // set referrer to self since that service contacted us
    hi.referrer.port = port;
    hosts.insert ( make_pair(ip, move(hi) ) );
}


// relevant "select" flags: LKEY, TIME, LSVC, LSVCF, COUNTS???
int LocalData::send(Writer& writer, uint32_t select, vector<string>& filters){
    int bytes = 0;
    if(select & SELECTION::LKEY){ // send local public key. It does not change. Do not lock.
        bytes += writer.write( &localPubKey, sizeof(localPubKey) );
    }

    // I wanted time to be a binary field and I want to use hton?() on it so uint32_t it is.
    // after 2038 there could be a glitch, but the field will stay 32 bit.
    // timestamp is important to avoid a replay attack
    if(select & SELECTION::TIME){
        uint32_t now = htonl(time(nullptr));
        bytes += writer.write( &now, sizeof(now));
    }

    if( !(select & SELECTION::LSVC) ){ return bytes; } // service list not requiested

    // filter service list (toSend). get the count.  write the count.  write list.
    vector<Service*> toSend;
    shared_lock lock(mutx);

    for(Service& s: services){
        if( s.passLocalFilters(filters) ){
            toSend.push_back(&s);
        }
    }

    uint16_t count = htons( toSend.size() );
    bytes += writer.write( &count, sizeof(count) );

    for(Service* s: toSend){
        bytes += writer.writeString(s->fullDescription);
    }
    return bytes;
}


// relevant "select" flags: IP, PORT, AGE, RKEY, RSVC, RSVCF
int RemoteData::send(Writer& writer, uint32_t select, vector<string>& filters){
    shared_lock lock(mutx);

    vector<HostInfo*> data;
    for(std::pair<const IPADDR,HostInfo>& hi: hosts){
        if( hi.second.passFilters(filters) ){
            data.push_back(&hi.second);
        }
    }

    int bytes = 0; // write record count first
    uint32_t count = htonl(data.size());
    bytes+= writer.write( &count, sizeof(count));

    for(HostInfo* hi: data){
        if( select & SELECTION::IP ){
            bytes+= writer.write( &hi->host, sizeof(hi->host) ); // writing without htonl()
        }
        if( select & SELECTION::PORT ){
            uint16_t p = htons(hi->port);
            bytes+= writer.write( &p, sizeof(p) );
        }
        if( select & SELECTION::AGE ){
            auto deltaT = system_clock::now() - hi->seen;
            auto count = deltaT.count()/60000000; // ms to minutes
            uint16_t age = count > 65500 ? 65500 : count; // reserve all above 65500
            age = htons(age);
            bytes+= writer.write( &age, sizeof(age) );
        }
        if( select & SELECTION::RKEY ){
            unsigned char keyCount = hi->key ? 1 : 0;
            bytes+= writer.write( &keyCount, sizeof(keyCount));
            if(hi->key){
                bytes+= writer.write( &*hi->key, sizeof(PubKey) );
            }
        }
        if( select & SELECTION::RSVC ){
            bool includeAllServices = !(select & SELECTION::RSVCF);
            vector<Service*> svc;
            for( Service& s: hi->services ){
                if(includeAllServices || s.passRemoteFilters(filters)){
                    svc.push_back(&s);
                }
            }
            uint16_t count = htons(svc.size());
            bytes+= writer.write( &count, sizeof(count));

            for( Service* s: svc){
                bytes+= writer.writeString(s->fullDescription);
            }
        } // if RSVC
    } // for(HostInfo
    return bytes;
}


// relevant "select" flags: BLIP, BLKEY, WLIP, WLKEY
int BWLists::send(Writer& writer, uint32_t select, vector<string>& filters){
    return 0; // TODO:
}

bool BWLists::blackListedIP(uint32_t host, uint16_t port){
    return false; // TODO:
}
