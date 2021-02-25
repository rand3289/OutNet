#ifndef CRAWLER_H_INCLUDED
#define CRAWLER_H_INCLUDED
#include "config.h"
#include "data.h"

class Crawler {
    Config& config;
    RemoteData& data;
public:
    Crawler(Config& configuration, RemoteData& rdata): config(configuration), data(rdata) {}
    int run();
};

#endif // CRAWLER_H_INCLUDED