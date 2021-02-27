#ifndef UPNP_H_INCLUDED
#define UPNP_H_INCLUDED
#include "sock.h"

class UPnP {
public:
    UPnP(){}
    int openPort(IPADDR ip, unsigned short port){ return 0; }
};

#endif // UPNP_H_INCLUDED