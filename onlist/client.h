#ifndef INCLUDED_CLIENT_H
#define INCLUDED_CLIENT_H
#include "sign.h" // PubKey
#include <vector>
#include <string>
#include <memory>

struct HostInfo{
    uint32_t host;                     // IPv4 address
    uint16_t port;                     // IPv4 port number (1-65535, 0=reserved)
    uint16_t age;                      // age of the record
    std::shared_ptr<PubKey> key;       // remote service's public key
    bool signatureVerified = false;    // was service queried directly or key found by a relay service?
    std::vector<std::string> services; // remote services list
};

// These are query flags.  Some are "fileds" which can be selected by a query.
// Field names are abbreviations up to 5 char long.  Filters use the same strings. Ex: AGE_LT_60
enum SELECT {
    LKEY   = (1<<0),  // local public key
    TIME   = (1<<1),  // current local datetime (to be used with signatures to avoid replay attack)
    SIGN   = (1<<2),  // signature (sign the whole message)
    LSVC   = (1<<3),  // local service list

    IP     = (1<<4),  // remote service IP
    PORT   = (1<<5),  // remote service port
    AGE    = (1<<6),  // age - how long ago (minutes) was remote service successfuly contacted 
    RKEY   = (1<<7),  // remote public key
    RSVC   = (1<<8),  // remote service list
    ISCHK  = (1<<9),  // return signatureVerified flag

    RSVCF  = (1<<10), // FILTER remote service list by protocol or send all?
}; // if records are filtered by service, we can still send all services for that record.


// local services add themselves by connecting to outnet with SPORT= set to their server port.
// when outnet connects to it, reply without signing the response.
// when OutNet detects local (non-routable) address, it adds services as its own local services.

// select contains what you want OutNet to return in your query
// outnet has to have host, port and optionally key filled in before the call
// upon return outnet.services contains local services, local key and signatureVerified flag.
// upon return peers contains a list of peers for your service to connect to
// myPort is used for registering your local service with OutNet service
// rwTimeout is a Read/Write time out in seconds for send() and recv() network operations
// filters is a list of filters you want OutNet to apply before returning the results
int queryOutNet(uint32_t select, HostInfo& outnet, std::vector<HostInfo>& peers, uint16_t myPort=0, int rwTimeout=10, std::vector<std::string>* filters = {} );


#endif // INCLUDED_CLIENT_H
