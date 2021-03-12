#ifndef CRAWLER_H_INCLUDED
#define CRAWLER_H_INCLUDED
#include "data.h"
#include "sign.h"
#include <vector>


class Crawler {
    SignatureVerify signer;
    uint32_t hostCopy;
    uint16_t portCopy;
    LocalData& ldata;
    RemoteData& rdata;
    BlackList& blist;
    int merge(std::vector<HostInfo>& newData);
public:
    int queryRemoteService(HostInfo& hi, std::vector<HostInfo>& newData, uint32_t select);
    Crawler(LocalData& lData, RemoteData& rData, BlackList& bList):
            hostCopy(0), portCopy(0), ldata(lData), rdata(rData), blist(bList) {}
    int loadRemoteDataFromDisk();
    int saveRemoteDataToDisk();
    int run();
};

#endif // CRAWLER_H_INCLUDED