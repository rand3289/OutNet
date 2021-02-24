#include "data.h"
#include "sign.h"
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
    return 0; // TODO:
}

// TODO: move to Sock ???
// writes unsigned char size (255max) + string without null
int writeString(Sock& sock, const string& str){
    constexpr static const int MAX_STR_LEN = 255;
    unsigned char iclen = str.length();
    if(str.length() > MAX_STR_LEN){
        iclen = MAX_STR_LEN;
        cerr << "WARNING: truncating string: " << str;
    }
    sock.write( (char*) &iclen, 1);
    return 1 + sock.write( (char*) str.c_str(), iclen);
}


int LocalData::send(Sock& sock, vector<string>& filters){
    sock.write((char*) &localPubKey, sizeof(localPubKey) );
    // TODO: filter services before sending
    size_t bytes = 0;
    shared_lock lock(lMutex);
    for(auto i: services){
        bytes+=writeString(sock, i);
    }
    return bytes;
}


int LocalData::addService(const string& service){
    unique_lock rlock(lMutex);
    services.push_back(service);
    servicesFields.emplace(servicesFields.end());
    servicesFields.back().parse(service);
    return 0;
}

//TODO:
string HostInfo::getHost(){ return ""; }
int HostInfo::setHost(string& ip){ return 0; }

uint16_t HostInfo::getPortHostByteOrder(){ return ntohs(port); }
void HostInfo::setPort(uint16_t portHostByteOrder){ port = htons(port); }


HostInfo& RemoteData::addEmpty(){
    unique_lock lock(rMutex);
    hosts.emplace( hosts.end() );
    return hosts.back();
}


int RemoteData::send(Sock& sock, vector<string>& filters){
    return 0; // TODO:
}
