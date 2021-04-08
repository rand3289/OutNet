#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED
#include "data.h"
#include "upnpnat.h"

// loads refresh rate, port, local services, black & white lists from disk, stores port
class Config {
    UPNPNAT upnp;
    LocalData* ldata;
    BlackList* blist;
    uint32_t refreshRate = 600; // seconds default

    int loadBlackListFiles();
    int loadServiceFiles();
    int findIPs();
public:
    void init(LocalData& ldata, BlackList& blist);
    bool forwardLocalPort(uint16_t port);
    int saveToDisk();
    int watch();
};

#endif // CONFIG_H_INCLUDED