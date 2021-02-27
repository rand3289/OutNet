#ifndef CRAWLER_H_INCLUDED
#define CRAWLER_H_INCLUDED
#include "data.h"

class Crawler {
    BWLists& bwlists;
    RemoteData* data=nullptr;
public:
    Crawler(BWLists& bw_lists): bwlists(bw_lists) {}
    int loadFromDisk(RemoteData& rdata);
    int saveToDisk();
    int run();
};

#endif // CRAWLER_H_INCLUDED