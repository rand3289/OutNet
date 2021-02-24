#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED
#include "data.h"

class Config{
public:
    int loadFromDisk(LocalData& ldata, RemoteData& rdata){ return 0; }
    int getPort(unsigned short defaultPort){ return 33344; } // DEBUGGING !!!
    int getWatcherRefreshRate(){ return 0; } // seconds for ServiceWatcher to look for new services
    int save(unsigned short port){ return 0; }
    int save(LocalData& ldata){ return 0; }
    int save(RemoteData& rdata){ return 0; }
};

#endif // CONFIG_H_INCLUDED