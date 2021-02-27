#include "crawler.h"
#include "sock.h"
#include <algorithm>
#include <mutex>
#include <shared_mutex> // since C++17
using namespace std;


// go through RemoteData entries and retrieve more data from those services.
// push new data at the end
// if Crawler is the only one modifying data,
// there is no need to lock untill ready to modify
int Crawler::run(){ 
    vector<HostInfo> newData;
// TODO: delete HostInfo entries that were offline n number of times???
// as we go through the list of hosts, if blah.offline > N, delete

    unique_lock rlock(data->rMutex);
    std::copy(newData.begin(), newData.end(), back_inserter(data->hosts));
    return 0; 
}

// Data is periodically saved.  When service is restarted, it is loaded back up
// RemoteData does not have to be locked here
int Crawler::loadFromDisk(RemoteData& rdata){
    data = &rdata;
    return 0;
}

int Crawler::saveToDisk(){ // save data to disk
    return 0;
}