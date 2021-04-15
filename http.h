#ifndef HTTP_H_INCLUDED
#define HTTP_H_INCLUDED

#include "sign.h"
#include <vector>
#include <array>
#include <string>

class Sock;
struct LocalData;
struct RemoteData;


// These are query flags.  Some are "fileds" which can be selected by a query.
// Field names should be abbreviations up to 5 char long.  This way filters can use the same strings.
enum SELECTION {
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


class Request{ // parse() returns QUERY bit field
    static bool parseFilter(char* filter, std::array<std::string,3>& funcOperVal);
public:
    static int parse(Sock & conn, std::vector<std::array<std::string,3>>& filters, uint16_t& port);
};


class Response{
    Signature signer;
public:
    int write(Sock& conn, uint32_t select, std::vector<std::array<std::string,3>>& filters, LocalData& ldata, RemoteData& rdata);
    static void writeDebug(Sock& conn, uint32_t select, std::vector<std::array<std::string,3>>& filters);
    static void writeDenied(Sock& conn, const std::string& reason);
};


#endif // HTTP_H_INCLUDED