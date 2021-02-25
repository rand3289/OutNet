#include "crawler.h"
#include "data.h"
#include "sock.h"
#include <algorithm>
#include <mutex>
#include <shared_mutex> // since C++17
using namespace std;

// TODO: should HostInfo be in unordered_mmap?

// go through RemoteData entries and retrieve more data from those services.
// push new data at the end
// if Crawler is the only one modifying data,
// there is no need to lock untill ready to modify
int Crawler::run(){ 
    vector<HostInfo> newData;
// TODO: delete HostInfo entries that were offline n number of times???
// as we go through the list of hosts, if blah.offline > N, delete

    unique_lock rlock(data.rMutex);
    std::copy(newData.begin(), newData.end(), back_inserter(data.hosts));
    return 0; 
}
