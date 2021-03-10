#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED
// everything related to protocol


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

    RSVCF  = (1<<9), // FILTER remote service list by protocol or send all?
    MYIP   = (1<<10)  // return my IP where my crawler is connecting from
};


#endif // PROTOCOL_H_INCLUDED