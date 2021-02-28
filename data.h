#ifndef DATA_H_INCLUDED
#define DATA_H_INCLUDED
#include "sign.h" // PubKey
#include <memory> // shared_ptr
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <shared_mutex> // since C++17
using namespace std;
#include <chrono>
using namespace std::chrono;

class Writer; // from http.h


struct Service{
    string fullDescription;
    string service;
    string protocol;
    uint32_t ip;
    uint16_t port;
//    string userDefinedField; // Do not parse
//    Service(const string& service);
    int parse(const string& service);
};


// TODO: key from Signature class gets filled in
struct LocalData {
    shared_mutex lMutex;
    PubKey localPubKey;
    vector<Service> services;

    int send(Writer& writer, int select, vector<string>& filters);
    int addService(const string& service);
};


struct HostPort {
    uint32_t host;
    uint16_t port;
    bool operator==(const HostPort& rhs){ return host==rhs.host && port == rhs.port; }
    HostPort& operator=(const HostPort& rhs){ host=rhs.host; port=rhs.port; return *this; }
};


struct HostInfo {                  // host/port fields are in the network byte order
    uint32_t host;                 // IPv4 address
    uint16_t port;                 // IPv4 port number (1-65535, 0=reserved)
    shared_ptr<PubKey> key;        // remote service's public key
    vector<string> services;       // remote services list

    time_point<system_clock> seen; // the server has been seen on line
    int offline = 0;               // server has been checked but found offline this many times
    bool verified = false;         // was service queried directly or key found by a relay service?
    HostPort referrer;             // preserve where this information come from

    string getHost();
    int setHost(string& ip);
    void setHost(unsigned long ip){ host = ip; } // ip has to be in the network byte order
    uint16_t getPortHostByteOrder(); // convert from network byte order to host byte order
    void setPort(uint16_t portHostByteOrder); // convert from host byte order to network byte order

    void resetTime(){ seen = system_clock::now(); }
    void addService(string& service){ services.push_back(service); }
};


// these are remote service properties related to interaction with that service
class Connections { // TODO: put a limit on connTimes (only last 10 or so remembered).
    unsigned short port;
    vector<time_point<system_clock> > connTimes; // when did this service connect to us
    int rating = 100;              // quality of service for this service
};


struct RemoteData {
    shared_mutex rMutex;
    vector<HostInfo> hosts;
    unordered_multimap<unsigned long int,Connections> connections;
    int send(Writer& writer, int select, vector<string>& filters);
};


// Black list and White list structures
struct BWLists{
    shared_mutex mutx;
    typedef string Protocol;
    vector<Protocol> protocolBlackList;
    vector<HostPort> hostBlackList;
    vector<HostPort> hostWhiteList;
    vector<PubKey>   keyBlackList;
    vector<PubKey>   keyWhiteList;
    int send(Writer& writer, int select, vector<string>& filters);
};


#endif // DATA_H_INCLUDED
