#include "config.h"
#include <thread>
using namespace std;


// ServiceWatch looks for files in the data directory (specified by config)  of the following types:
// *.blacklist - is a black list of IP:port pairs, public keys or protocols
// *.whitelist - overrides blacklist entries and stores keys of interest
// *.service - is a single or multiple service description (one per line)
// *.service file can contain remote service urls which can be used to bootstrap the network.

int Config::watch(){
    while(true){
        std::this_thread::sleep_for( seconds(getRefreshRate()) );
        vector<string> services;
        if( checkRegistration(services) ){
            updateLocalData(services);
            openPorts(services);
        }
    }
}


int Config::loadFromDisk(LocalData& lData, BWLists& bwLists){
    ldata = &lData;
    bwlists = &bwLists;
    return 0;
}
