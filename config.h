#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED
#include "data.h"
#include "upnp.h"
#include <vector>
#include <string>
#include <thread>
using namespace std;

// loads refresh rate, port, local services, black & white lists from disk
// stores port on disk

class Config{
    LocalData& data;
    BWLists& lists;
    UPnP upnp;

    int checkRegistration(vector<string>& services) { return 0; }
    int updateLocalData(vector<string>& services) { return 0; }
    int openPorts(vector<string>& services){ return 0; } // open router port using UPnP
    int getRefreshRate(){ return 10; } // seconds for Config to look for files
public:
    Config(LocalData& ldata, BWLists& bwlists):data(ldata), lists(bwlists) {}
    int loadFromDisk(){ return 0; }
    int getPort(unsigned short defaultPort){ return 33344; } // DEBUGGING !!!
    int savePort(unsigned short port){ return 0; }
    int watch();
};

#endif // CONFIG_H_INCLUDED