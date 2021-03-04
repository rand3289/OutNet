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

class Writer; // from http.h (http.h is not included)


struct Service{
    string fullDescription;
    string service;
    string protocol;
    uint32_t ip;
    uint16_t port;
//    string userDefinedField; // Do not parse
//    Service(const string& service);
    int parse(const string& service);
    bool passLocalFilters(vector<string> filters);
    bool passRemoteFilters(vector<string> filters);
};


// TODO: key from Signature class gets filled in
struct LocalData {
    shared_mutex mutx;
    PubKey localPubKey;
    vector<Service> services;

    int send(Writer& writer, int select, vector<string>& filters);
    int addService(const string& service);
};


struct HostPort {
    uint32_t host;
    uint16_t port;
    HostPort(): host(0), port(0) {}
    bool operator==(const HostPort& rhs){ return host==rhs.host && port == rhs.port; }
    HostPort& operator=(const HostPort& rhs){ host=rhs.host; port=rhs.port; return *this; }
};


struct HostInfo {                    // host/port fields are in the network byte order
    uint32_t host;                   // IPv4 address
    uint16_t port;                   // IPv4 port number (1-65535, 0=reserved)
    shared_ptr<PubKey> key;          // remote service's public key
    vector<Service> services;        // remote services list
// these are remote service properties related to interaction with that service
    bool signatureVerified = false;  // was service queried directly or key found by a relay service?
    int offlineCount = 0;            // server has been found offline this many times IN A ROW
    time_point<system_clock> seen;   // last time we successfully connected to it
    time_point<system_clock> missed; // last time of unsuccessful connect
    time_point<system_clock> called; // last time this service connected to us
    int rating = 100;                // our interaction rating for this service
    HostPort referrer;               // preserve where this information came from (for rating that service)

    bool passFilters(vector<string> filters);
    void addService(const string& service){
        services.emplace( services.end() )->parse(service);
    }
};


#ifndef IPADDR
    typedef unsigned long IPADDR; // sockaddr_in::sin_addr.s_addr defines it as "unsigned long"
#endif

struct RemoteData {
    shared_mutex mutx;
    unordered_multimap<IPADDR, HostInfo> hosts;

    // send HostInfo records through the writer to a remote host
    int send(Writer& writer, int select, vector<string>& filters);
    // Remember this host/port.  Return last contact time we saw it before
    time_point<system_clock> addContact(IPADDR ip, unsigned short port);
};


// Black list and White list structures
struct BWLists {
    shared_mutex mutx;
    vector<HostPort> hostBlackList;
    vector<HostPort> hostWhiteList;
    vector<PubKey>   keyBlackList;
    vector<PubKey>   keyWhiteList;
    int send(Writer& writer, int select, vector<string>& filters);
    bool blackListedIP(unsigned long host, unsigned short port){ return false; } // TODO:
};


#endif // DATA_H_INCLUDED
