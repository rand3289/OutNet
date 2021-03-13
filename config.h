#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED
#include "data.h"
#include "upnp.h"

// loads refresh rate, port, local services, black & white lists from disk, stores port
class Config {
    UPnP upnp;
    LocalData* ldata;
    BlackList* blist;
    uint32_t refreshRate = 60; // seconds default

    int loadBlackListFiles();
    int loadServiceFiles();
public:
    int loadFromDisk(LocalData& ldata, BlackList& blist);
    int saveToDisk();
    int watch();
};

#endif // CONFIG_H_INCLUDED