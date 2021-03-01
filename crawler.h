#ifndef CRAWLER_H_INCLUDED
#define CRAWLER_H_INCLUDED
#include "data.h"

class Crawler {
    BWLists& bwlists;
    RemoteData* data=nullptr;
    int merge(vector<HostInfo>& newData);
    int queryRemoteService(HostInfo& hi, vector<HostInfo>& newData);
public:
    Crawler(BWLists& bw_lists): bwlists(bw_lists) {}
    int loadFromDisk(RemoteData& rdata);
    int saveToDisk();
    int run();
};

#endif // CRAWLER_H_INCLUDED