#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED
#include "data.h"
#include "upnp.h"
#include <vector>
#include <string>
#include <thread>

// loads refresh rate, port, local services, black & white lists from disk, stores port
class Config {
    UPnP upnp;
    LocalData* ldata;
    BWLists* bwlists;
    uint32_t refreshRate = 30; // seconds default

    int loadBlackListFiles();
    int loadServiceFiles();
    int openPort(uint32_t ip, uint16_t port){ return 0; } // open router port using UPnP
public:
    int loadFromDisk(LocalData& ldata, BWLists& bwlists);
    int saveToDisk();
    int watch();
};

#endif // CONFIG_H_INCLUDED