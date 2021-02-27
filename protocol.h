// everything related to protocol
#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED


// these are flags.  Some are defined as "fileds" which can be selected by a query
// field names should be abbreviations of up to 5 char this way filters can use the same strings
// when evaluating filters ex: PORT_EQ_3214  "PORT_EQ" part can be switch() on as if it was uint64_t
enum SELECTION {
// counts of filtered records? IP:port:age, non-null public keys, local services, remote services etc... (COUNTS=8)
    LKEY  = 1,    // local public key
    TIME  = 2,    // current local datetime (to be used with signatures to avoid replay attack)
    LSVC  = 4,    // local service list
    IP    = 16,   // remote service IP
    PORT  = 32,   // remote service port
    AGE   = 64,   // age - how long ago (minutes) was remote service contacted 
    RKEY  = 128,  // remote public key
    RSVC  = 256,  // remote service list
    SIGN  = 512,  // signature (sign the whole message)
    RSVCL = 1024, // remote service list filtered by service type/protocol
    LSVCL = 2048, // local service list filtered by service type/protocol
    BLIST = 4096, // black list
    WLIST = 8192  // white list
};


#endif // PROTOCOL_H_INCLUDED