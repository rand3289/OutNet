#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED
// everything related to protocol


// These are flags.  Some are defined as "fileds" which can be selected by a query
// field names should be abbreviations of up to 5 char this way filters can use the same strings
// when evaluating filters ex: PORT_EQ_3214  "PORT_EQ" part can be switch() on as if it was uint64_t
enum SELECTION {
    LKEY   = (1<<0),  // local public key
    TIME   = (1<<1),  // current local datetime (to be used with signatures to avoid replay attack)
    SIGN   = (1<<2),  // signature (sign the whole message)
//  COUNTS = (1<<3),  // total records, non-null public keys, local services, remote services etc...

    LSVC   = (1<<3),  // local service list
    IP     = (1<<4),  // remote service IP
    PORT   = (1<<5),  // remote service port
    AGE    = (1<<6),  // age - how long ago (minutes) was remote service successfuly contacted 
    RKEY   = (1<<7),  // remote public key
    RSVC   = (1<<8),  // remote service list
    RSVCF  = (1<<9),  // remote service list filtered by service type/protocol or send all

    BLPROT = (1<<10), // black list for protocols.  WLPROT does not exist
    BLIP   = (1<<11), // black list for IPs
    BLKEY  = (1<<12), // black list for Public Keys
    WLIP   = (1<<13), // white list for IPs
    WLKEY  = (1<<14)  // white list for Public Keys
};


#endif // PROTOCOL_H_INCLUDED