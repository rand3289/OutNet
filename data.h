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

class Sock;


struct Service {
    string fullDescription;     // should have external routable IP (this is what we send)
    string originalDescription; // original (this is what we get from files)
    string service;             // general type of service
    string protocol;            // service protocol such as http, ftp etc...
    uint32_t ip;                // real IP (local/non-routable IP) from originalDescription
    uint16_t port;              // port number for this service

    Service* parse(const string& service); // Do not parse userDefinedFiled ???
    bool passLocalFilters(vector<string> filters);
    bool passRemoteFilters(vector<string> filters);
    bool operator==(const Service& rhs){ return fullDescription == rhs.fullDescription; }
};
void mergeServices(vector<Service>& dest, vector<Service>& source); // helper free function


struct LocalData {
    shared_mutex mutx;  // This datastructure is accessed by several threads.  Lock mutex before access.
    uint32_t myIP;      // public ip of the local service
    uint32_t myPort;    // local service is running on this port
    PubKey localPubKey; // local service public key
    vector<Service> services; // a list of local services we are "advertising" // TODO: should it be a set?

    int send(Sock& sock, uint32_t select, vector<string>& filters, Signature& signer);
    Service* addService(const string& service);
};


struct HostInfo {                    // Information about remote services from one host
    constexpr static const int DEFAULT_RATING = 100;
    uint32_t host;                   // IPv4 address
    uint16_t port;                   // IPv4 port number (1-65535, 0=reserved)
    shared_ptr<PubKey> key;          // remote service's public key
    vector<Service> services;        // remote services list
// these are remote service properties related to interaction with that service
    bool signatureVerified = false;  // was service queried directly or key found by a relay service?
    int offlineCount;                // server has been found offline this many times IN A ROW
    int rating;                      // our interaction rating for this service
    time_point<system_clock> met;    // first time I learned about this host
    time_point<system_clock> seen;   // last time I successfully connected to it
    time_point<system_clock> missed; // last time of unsuccessful connect attempt
    uint32_t referIP;                // preserve where this information came from 
    uint16_t referPort;              // port of the service where we got this HostInfo record

    HostInfo();
    bool passFilters(vector<string> filters);
    Service* addService(const string& service);
};


#ifndef IPADDR
    typedef uint32_t IPADDR; // sockaddr_in::sin_addr.s_addr defines it as "unsigned long"
#endif

struct RemoteData {
    shared_mutex mutx;
    unordered_multimap<IPADDR, HostInfo> hosts; // IP to HostInfo map

    // send HostInfo records through the writer to a remote host
    int send(Sock& sock, uint32_t select, vector<string>& filters, Signature& signer);
    // Remember this host/port for crawler to contact
    void addContact(IPADDR ip, uint16_t port);
};


// A way to ban some IPs and Public keys
struct BlackList {
    shared_mutex mutx;
    vector<IPADDR> badHosts;
    vector<PubKey> badKeys;
    bool isBanned(IPADDR host);
    bool isBanned(PubKey& key);
};


#endif // DATA_H_INCLUDED
