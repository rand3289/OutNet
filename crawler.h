#ifndef CRAWLER_H_INCLUDED
#define CRAWLER_H_INCLUDED
#include "config.h"
#include "data.h"

class Crawler {
    Config& config; // TODO: does clawler need access to config? (remove #include "config.h" also)
    RemoteData& data;
    BWLists& lists;
    int saveRemoteDataToDisk();
public:
    Crawler(Config& configuration, RemoteData& rdata, BWLists& bwlists): config(configuration), data(rdata), lists(bwlists) {}
    int loadRemoteDataFromDisk();
    int run();
};

#endif // CRAWLER_H_INCLUDED