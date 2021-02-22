#ifndef COLLECTOR_H_INCLUDED
#define COLLECTOR_H_INCLUDED
#include "config.h"
#include "data.h"

class Collector{
    Config& config;
    RemoteData& data;
public:
    Collector(Config& configuration, RemoteData& rdata): config(configuration), data(rdata) {}
    int run(){ return 0; }
};

#endif // COLLECTOR_H_INCLUDED