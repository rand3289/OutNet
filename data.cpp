#include "data.h"
#include "http.h"
#include "sign.h"
#include "sock.h"
#include "protocol.h"
#include "utils.h"
#include <memory> // shared_ptr
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <mutex>
#include <shared_mutex> // since C++17
using namespace std;
#include <chrono>
using namespace std::chrono;


HostInfo::HostInfo(): host(0), port(0), signatureVerified(false), offlineCount(0),
        rating(DEFAULT_RATING), referIP(0), referPort(0) {
    met = seen = missed = system_clock::from_time_t(0); // time_point::min() is broken (overflows) !!!
}


Service* HostInfo::addService(const string& service){
    auto s = services.emplace( services.end() )->parse(service, host); // parsing remote service
    if(!s){ services.pop_back(); } // if it was not parsed, delete it
    return s;
}


Service* LocalData::addService(const string& service){
    auto s = services.emplace( services.end() )->parse(service, myIP); // parsing local service
    if(!s){ services.pop_back(); } // if it was not parsed, delete it
    return s;
}


// helper free function to merge vectors of services from destination to source
void mergeServices(vector<Service>& dest, vector<Service>& source){
    for(auto& src: source){
        auto it = find(begin(dest), end(dest), src);
        if(it!= end(dest) ){ continue; } // exists
        dest.push_back(move(src));
    }
}

// parse service description into fields separated by ":"
// example - "printer:ipp:8.8.8.8:54321:2nd floor printer"
Service* Service::parse(const string& servStr, uint32_t myIP){
    originalDescription = servStr;

    char* start = const_cast<char*>( servStr.c_str() ); // living dangerous :)
    const char* end = start+service.length();
    char* token;

    if( tokenize(start, end, token, ":") ){
        service = token; // TODO: enforce max len ???
        if(service.length() < 1){ return nullptr; }
    } else { return nullptr; }

    if( tokenize(start, end, token, ":") ){
        protocol = token; // TODO: enforce max len ???
        if(protocol.length() < 1){ return nullptr; }
    } else { return nullptr; }

    if( tokenize(start, end, token, ":") ){
        ip = Sock::strToIP(token);
        if(0==ip){ return nullptr; }
    } else { return nullptr; }

    if( tokenize(start, end, token, ":") ){
        port = atoi(token);
        if(0==port){ return nullptr; }
    } else { return nullptr; }

    if( tokenize(start, end, token, ":") ){
        other = token;
    } else { return nullptr; }

    // Translate local to NAT (router) IP
    uint32_t routableIP = Sock::isRoutable(ip) ? ip : myIP;
    stringstream ss;
    ss << service << ":" << protocol << ":" << Sock::ipToString(routableIP) << ":" << port << ":" << other;
    fullDescription = ss.str();
    return this;
}


// Decides if a service passes a filter.
// There can be two types of service related filters specified in the HTTP GET Query string:
// local filters apply to local services (start with L) and remote (start with R)
bool Service::passFilters(vector<string> filters, bool remote){
    char buff[64];
    for(const string& filter: filters){
        strncpy(buff, filter.c_str(), sizeof(buff));
        char* function, *oper, *value, *b = buff;
        if(tokenize(b, b+sizeof(buff), function, "_") ){
            if(  remote && 'R' != *function){ continue; }
            if( !remote && 'L' != *function){ continue; }
            ++function; // we checked the first letter
            string func(function);
            if(tokenize(b, b+sizeof(buff), oper, "_") ){
                if(tokenize(b, b+sizeof(buff), value, "_") ){
                    if( 0 == func.compare("SVC") ){
                        if(service != value) { return false; }
                    } else if( 0 == func.compare("PROT") ){
                        if(protocol != value){ return false; }
                    } else if( 0 == func.compare("PORT") && string("EQ").compare(oper) == 0 ){ // TODO: implement LT, GT
                        if(port != atoi(value)){ return false; }
                    }
                }
            }
        }
    }
    return true;
}


// filter by: host, port, key, 
bool HostInfo::passFilters(vector<string> filters){
    return true; // TODO:
}


void RemoteData::addContact(IPADDR ip, uint16_t port){
    unique_lock ulock(mutx);

    for( auto range = hosts.equal_range(ip); range.first != range.second; ++range.first){
        if( port == range.first->second.port ) { // existing host
            return;
        }
    }

    HostInfo hi;
    hi.host = ip;
    hi.port = port;
    hi.referIP = ip;     // set referrer to itself since that service contacted us
    hi.referPort = port; // or it was added through command line
    hosts.emplace( ip, move(hi) );
}


// relevant "select" flags: LKEY, TIME, LSVC, LSVCF, COUNTS???
int LocalData::send(Sock& sock, uint32_t select, vector<string>& filters, Signature& signer){
    bool sign = select & SELECTION::SIGN;

    int bytes = 0;
    if(select & SELECTION::LKEY){ // send local public key. It does not change. Do not lock.
        bytes += sock.write( &localPubKey, sizeof(localPubKey) );
        if(sign){ signer.write(&localPubKey, sizeof(localPubKey) ); }
    }

    // I wanted time to be a binary field and I want to use hton?() on it so uint32_t it is.
    // after 2038 there could be a glitch, but the field will stay 32 bit.
    // timestamp is important to avoid a replay attack
    if(select & SELECTION::TIME){
        uint32_t now = htonl(time(nullptr));
        bytes += sock.write( &now, sizeof(now));
        if(sign){ signer.write(&now, sizeof(now)); }
    }

    if( !(select & SELECTION::LSVC) ){ return bytes; } // service list not requiested

    // filter service list (toSend). get the count.  write the count.  write list.
    vector<Service*> toSend;
    shared_lock lock(mutx);

    for(Service& s: services){
        if( s.passFilters(filters, false) ){
            toSend.push_back(&s);
        }
    }

    uint16_t count = htons( toSend.size() );
    bytes += sock.write( &count, sizeof(count) );
    if(sign){ signer.write(&count, sizeof(count)); }

    for(Service* s: toSend){
        bytes += sock.writeString(s->fullDescription);
        if(sign){ signer.writeString(s->fullDescription);}
    }
    return bytes;
}


// relevant "select" flags: IP, PORT, AGE, RKEY, RSVC, RSVCF
int RemoteData::send(Sock& sock, uint32_t select, vector<string>& filters, Signature& signer){
    bool sign = select & SELECTION::SIGN;
    shared_lock lock(mutx);

    vector<HostInfo*> data;
    for(std::pair<const IPADDR,HostInfo>& hi: hosts){
        if( hi.second.passFilters(filters) ){
            data.push_back(&hi.second);
        }
    }

    int bytes = 0; // write record count first
    uint32_t count = htonl(data.size());
    bytes+= sock.write( &count, sizeof(count));
    if(sign){ signer.write(&count, sizeof(count)); }

    for(HostInfo* hi: data){
        if( select & SELECTION::IP ){
            bytes+= sock.write( &hi->host, sizeof(hi->host) ); // writing without htonl()
            if(sign){ signer.write(&hi->host, sizeof(hi->host)); }
        }
        if( select & SELECTION::PORT ){
            uint16_t p = htons(hi->port);
            bytes+= sock.write( &p, sizeof(p) );
            if(sign){ signer.write(&p, sizeof(p)); }
        }
        if( select & SELECTION::AGE ){
            auto deltaT = system_clock::now() - hi->seen;
            auto count = deltaT.count()/60000000; // ms to minutes
            uint16_t age = count > 65500 ? 65500 : count; // reserve all above 65500
            age = htons(age);
            bytes+= sock.write( &age, sizeof(age) );
            if(sign){ signer.write(&age, sizeof(age) ); }

        }
        if( select & SELECTION::RKEY ){
            unsigned char keyCount = hi->key ? 1 : 0;
            bytes+= sock.write( &keyCount, sizeof(keyCount));
            if(sign){ signer.write(&keyCount, sizeof(keyCount)); }

            if(hi->key){
                bytes+= sock.write( &*hi->key, sizeof(PubKey) );
                if(sign){ signer.write(&hi->key, sizeof(PubKey)); }
            }
        }
        if( select & SELECTION::RSVC ){
            bool includeAllServices = !(select & SELECTION::RSVCF);
            vector<Service*> svc;
            for( Service& s: hi->services ){
                if(includeAllServices || s.passFilters(filters, true)){
                    svc.push_back(&s);
                }
            }
            uint16_t count = htons(svc.size());
            bytes+= sock.write( &count, sizeof(count));
            if(sign){ signer.write(&count, sizeof(count)); }

            for( Service* s: svc){
                bytes+= sock.writeString(s->fullDescription);
                signer.writeString(s->fullDescription);
            }
        } // if RSVC
    } // for(HostInfo
    return bytes;
}


// is this ip in our black list
bool BlackList::isBanned(IPADDR host){ return false; }


// is this public key in our black list
bool BlackList::isBanned(PubKey& key){ return false; }
