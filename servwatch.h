#ifndef SERVWATCH_H_INCLUDED
#define SERVWATCH_H_INCLUDED
#include "data.h"
#include "config.h"
#include <thread>
#include <chrono>
using namespace std;
using namespace std::chrono;


class ServiceWatch{
    Config& config;
    LocalData data;
public:
    ServiceWatch(Config& configuration, LocalData& ldata): config(configuration), data(ldata) {}
    int checkRegistration();
    int updateLocalData(LocalData& data);
    int run(){
        while(true){
            std::this_thread::sleep_for( seconds(config.getWatcherRefreshRate()) );
            if( checkRegistration() ){
                updateLocalData(data);
                config.save(data);  // save config in case service shuts down
            }
        }
    }
};

#endif // SERVWATCH_H_INCLUDED