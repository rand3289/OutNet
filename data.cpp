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

int Service::parse(const string& service){
    fullDescription = service;
    return 0; // TODO:
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

HostInfo::HostInfo(): host(0), port(0), signatureVerified(false), offlineCount(0), rating(DEFAULT_RATING) {
    seen = missed = called = system_clock::from_time_t(0); // min() is broken!!!
}


int LocalData::addService(const string& service){
    return services.emplace( services.end() )->parse(service);
}


void RemoteData::addContact(IPADDR ip, unsigned short port){
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
    hosts.insert ( make_pair(ip, hi) );
}


// relevant "select" flags: LKEY, TIME, LSVC, LSVCF, COUNTS???
int LocalData::send(Writer& writer, int select, vector<string>& filters){
    size_t bytes = 0;
    if(select & SELECTION::LKEY){ // send local public key. It does not change. Do not lock.
        bytes += writer.write((char*) &localPubKey, sizeof(localPubKey) );
    }

    // I wanted time to be a binary field and I want to use hton?() on it so uint32_t it is.
    // after 2038 there could be a glitch, but the field will stay 32 bit.
    // timestamp is important to avoid a replay attack
    if(select & SELECTION::TIME){
        unsigned long now = htonl(time(nullptr));
        bytes += writer.write( (char*)&now, sizeof(now));
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

    unsigned short count = htons( toSend.size() );
    bytes += writer.write((char*)&count, sizeof(count) );

    for(Service* s: toSend){
        bytes += writer.writeString(s->fullDescription); // TODO: translate local to NAT IP if needed
    }
    return bytes;
}


// relevant "select" flags: IP, PORT, AGE, RKEY, RSVC, RSVCF
int RemoteData::send(Writer& writer, int select, vector<string>& filters){
    shared_lock lock(mutx);

    vector<HostInfo*> data;
    for(std::pair<const IPADDR,HostInfo>& hi: hosts){
        if( hi.second.passFilters(filters) ){
            data.push_back(&hi.second);
        }
    }

    size_t bytes = 0; // write record count first
    unsigned long count = htonl(data.size());
    bytes+= writer.write((char*) &count, sizeof(count));

    for(HostInfo* hi: data){
        if( select & SELECTION::IP ){
            bytes+= writer.write( (char*)&hi->host, sizeof(hi->host) ); // writing without htonl()
        }
        if( select & SELECTION::PORT ){
            unsigned short p = htons(hi->port);
            bytes+= writer.write( (char*)&p, sizeof(p) );
        }
        if( select & SELECTION::AGE ){
            auto deltaT = system_clock::now() - hi->seen;
            auto count = deltaT.count()/60000000; // ms to minutes
            unsigned short age = count > 65500 ? 65500 : count; // reserve all above 65500
            age = htons(age);
            bytes+= writer.write( (char*)&age, sizeof(age) );
        }
        if( select & SELECTION::RKEY ){
            if(hi->key){
                char keyCount = 1;
                bytes+= writer.write( (char*)&keyCount, sizeof(keyCount));
                bytes+= writer.write( (char*)&*hi->key, sizeof(PubKey) );
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
            unsigned short count = htons(svc.size());
            bytes+= writer.write((char*) &count, sizeof(count));

            for( Service* s: svc){
                bytes+= writer.writeString(s->fullDescription);
            }
        } // if RSVC
    } // for(HostInfo
    return bytes;
}


// relevant "select" flags: BLIP, BLKEY, WLIP, WLKEY
int BWLists::send(Writer& writer, int select, vector<string>& filters){
    return 0; // TODO:
}

bool BWLists::blackListedIP(unsigned long host, unsigned short port){
    return false; // TODO:
}
