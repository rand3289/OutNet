#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED
#include "data.h"

class Config{
public:
    int loadFromDisk(LocalData& ldata, RemoteData& rdata);
    int getPort(unsigned short defaultPort);
    int getWatcherRefreshRate(); // seconds for ServiceWatcher to look for new services
    int save(unsigned short port);
    int save(LocalData& ldata);
    int save(RemoteData& rdata);
};

#endif // CONFIG_H_INCLUDED