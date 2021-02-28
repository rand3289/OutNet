#include "data.h"
#include "http.h"
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


int LocalData::send(Writer& writer, int select, vector<string>& filters){
    writer.write((char*) &localPubKey, sizeof(localPubKey) );
    // TODO: delete "local" filters here???

    // TODO: filter services before sending
    size_t bytes = 0;
    shared_lock lock(lMutex);
    for(auto i: services){
        bytes+=writer.writeString(i);
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


int RemoteData::send(Writer& writer, int select, vector<string>& filters){
    return 0; // TODO:
}


int BWLists::send(Writer& writer, int select, vector<string>& filters){
    return 0; // TODO:
}
