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


bool Service::passFilters(vector<string> filters){
    return true; // TODO:
}


bool HostInfo::passFilters(vector<string> filters){
    return true; // TODO:
}


//TODO:
string HostInfo::getHost(){ return ""; }
int HostInfo::setHost(string& ip){ return 0; }
uint16_t HostInfo::getPortHostByteOrder(){ return ntohs(port); }
void HostInfo::setPort(uint16_t portHostByteOrder){ port = htons(port); }


int LocalData::addService(const string& service){
//    unique_lock rlock(lMutex);
    return services.emplace( services.end() )->parse(service);
}


// relevant "select" flags: LKEY, TIME, LSVC, LSVCF, COUNTS???
int LocalData::send(Writer& writer, int select, vector<string>& filters){
    size_t bytes = 0;
    if(select & SELECTION::LKEY){ // send local public key
        bytes += writer.write((char*) &localPubKey, sizeof(localPubKey) );
    }

    // I wanted time to be a binary field and I want to use hton?() on it so uint32_t it is.
    // after 2038 there could be a glitch, but the field will stay 32 bit.
    // timestamp is important to avoid a replay attack
    if(select & SELECTION::TIME){
        unsigned long now = time(nullptr);
        bytes += writer.write( (char*)&now, sizeof(now));
    }

    if( !(select & SELECTION::LSVC) ){ return bytes; } // service list not requiested

    // filter service list (toSend). get the count.  write the count.  write list.
    vector<Service*> toSend;
    bool sendAll = !(select & SELECTION::LSVCF); // TODO: does it make sense to have this flag?
    shared_lock lock(lMutex);

    for(Service& s: services){
        if(sendAll || s.passFilters(filters) ){
            toSend.push_back(&s);
        }
    }

    unsigned long int count = htonl( toSend.size() );
    bytes += writer.write((char*)&count, sizeof(count) );

    for(Service* s: toSend){
        bytes += writer.writeString(s->fullDescription);
    }
    return bytes;
}


// relevant "select" flags: IP, PORT, AGE, RKEY, RSVC, RSVCF
int RemoteData::send(Writer& writer, int select, vector<string>& filters){
    size_t bytes = 0;
    for(HostInfo& hi: hosts){
        if( hi.passFilters(filters) ){
            if( select & SELECTION::IP ){
                unsigned long ip = htonl(hi.host);
                bytes+= writer.write( (char*)&ip, sizeof(ip) );
            }
            if( select & SELECTION::PORT ){
                unsigned short p = htons(hi.port);
                bytes+= writer.write( (char*)&p, sizeof(p) );
            }
            if( select & SELECTION::AGE ){
                auto deltaT = system_clock::now() - hi.seen;
                auto count = deltaT.count()/60000000; // ms to minutes
                unsigned short age = count > 65500 ? 65500 : count; // reserve all above 65500
                age = htons(age);
                bytes+= writer.write( (char*)&age, sizeof(age) );
            }
            if( select & SELECTION::RKEY ){
                bytes+= writer.write( (char*)&hi.key, sizeof(hi.key) );
            }
            if( select & SELECTION::RSVC ){
                bool includeAllServices = !(select & SELECTION::RSVCF);
                for( Service& s: hi.services ){
                    if(includeAllServices || s.passFilters(filters)){
                        bytes+= writer.writeString(s.fullDescription);
                    }
                }
            }
        }
    }
    return 0; // TODO:
}


// relevant "select" flags: BLIST, WLIST (BLPROT, BLIP, BLKEY, WLIP, WLKEY) [WLPROT does not exist]
int BWLists::send(Writer& writer, int select, vector<string>& filters){
    return 0; // TODO:
}
