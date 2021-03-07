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


struct Service {
    string fullDescription; // should have external routable IP
    string originalDescription; // original - copied here in parse
    string service;
    string protocol;
    uint32_t ip; // real IP (local/non-routable IP)
    uint16_t port;
//    string userDefinedField; // Do not parse ???
//    Service(const string& service);
    Service* parse(const string& service);
    bool passLocalFilters(vector<string> filters);
    bool passRemoteFilters(vector<string> filters);
};


struct LocalData {
    shared_mutex mutx;
//    HostPort self; // public ip of the local service and port it is running on // TODO:
    uint32_t myIP;    // public ip of the local service
    uint32_t myPort; // local service is running on this port
    PubKey localPubKey;
    vector<Service> services;

    int send(Writer& writer, uint32_t select, vector<string>& filters);
    Service* addService(const string& service);
};


struct HostPort {
    uint32_t host;
    uint16_t port;
    HostPort(): host(0), port(0) {}
    HostPort(const HostPort& rhs): host(rhs.host), port(rhs.port) {}
    HostPort& operator=(const HostPort& rhs){ host=rhs.host; port=rhs.port; return *this; }
    bool operator==(const HostPort& rhs){ return host==rhs.host && port == rhs.port; }
};


// TODO: replace host/port with HostPort ???
// TODO: replace seen, missed with lastContact time.  offlineCount tells us if connect succeeded
// TODO: delete called
struct HostInfo {                    // host/port fields are in the network byte order
    constexpr static const int DEFAULT_RATING = 100;
    uint32_t host;                   // IPv4 address
    uint16_t port;                   // IPv4 port number (1-65535, 0=reserved)
    shared_ptr<PubKey> key;          // remote service's public key
    vector<Service> services;        // remote services list
// these are remote service properties related to interaction with that service
    bool signatureVerified = false;  // was service queried directly or key found by a relay service?
    int offlineCount = 0;            // server has been found offline this many times IN A ROW
    int rating = 100;                // our interaction rating for this service
    time_point<system_clock> met;    // first time I learned about this host
    time_point<system_clock> seen;   // last time I successfully connected to it
    time_point<system_clock> missed; // last time of unsuccessful connect attempt
    time_point<system_clock> called; // last time this service connected to us
    HostPort referrer;               // preserve where this information came from (for rating that service)

    HostInfo();
    bool passFilters(vector<string> filters);
    Service* addService(const string& service);
};


#ifndef IPADDR
    typedef uint32_t IPADDR; // sockaddr_in::sin_addr.s_addr defines it as "unsigned long"
#endif

struct RemoteData {
    shared_mutex mutx;
    unordered_multimap<IPADDR, HostInfo> hosts;

    // send HostInfo records through the writer to a remote host
    int send(Writer& writer, uint32_t select, vector<string>& filters);
    // Remember this host/port for crawler to contact
    void addContact(IPADDR ip, uint16_t port);
};


// Black list and White list structures
struct BWLists {
    shared_mutex mutx;
    vector<HostPort> hostBlackList;
    vector<HostPort> hostWhiteList;
    vector<PubKey>   keyBlackList;
    vector<PubKey>   keyWhiteList;
    int send(Writer& writer, uint32_t select, vector<string>& filters);
    bool blackListedIP(uint32_t host, uint16_t port);
};


#endif // DATA_H_INCLUDED
