#ifndef CRAWLER_H_INCLUDED
#define CRAWLER_H_INCLUDED
#include "data.h"
#include "sock.h"


class Reader{
protected:
    Sock* sock = nullptr;
public:
    virtual void init(Sock& socket, PubKey& key){ sock = & socket; }
    virtual bool verifySignature(PubSign& signature){ return true; }
    virtual int write(void* data, size_t size){ return 0; }
    virtual int read(void* data, size_t size){ return sock->read(data, size); }
    virtual int readString(void* buff, size_t size){ return sock->readString(buff, size); }
    virtual uint16_t read16(bool& error){ return sock->read16(error); }
    virtual uint32_t read32(bool& error){ return sock->read32(error); }
};


class SignatureReader: public Reader{
    SignatureVerify sign;
public:
    virtual void init(Sock& socket, PubKey& key){ sock = & socket; sign.init(key); }
    virtual bool verifySignature(PubSign& signature){ return sign.verify(signature); }
    virtual int write(void* data, size_t size){ return sign.write(data,size); }
    virtual int read(void* data, size_t size){
        int ret = sock->read(data,size);
        sign.write(data, ret); // sign the data
        return ret;
    }
    virtual int readString(void* buff, size_t size){
        int ret = sock->readString(buff, size);
        sign.write(buff,ret);
        return ret;
    }
    virtual uint16_t read16(bool& error){
        uint16_t ret = sock->read16(error);
        sign.write(&ret, sizeof(ret));
        return ret;
    }
    virtual uint32_t read32( bool& error){
        uint32_t  ret = sock->read32( error);
        sign.write(&ret, sizeof(ret));
        return ret;
    }
};


class Crawler {
    Reader dumbReader;
    SignatureReader signatureReader;
    LocalData& ldata;
    BWLists& bwlists;
    RemoteData* data=nullptr;
    int merge(vector<HostInfo>& newData);
    int queryRemoteService(HostInfo& hi, vector<HostInfo>& newData, const uint32_t select, HostPort& self);
public:
    Crawler(LocalData& locData, BWLists& bw_lists): ldata(locData), bwlists(bw_lists) {}
    int loadFromDisk(RemoteData& rdata);
    int saveToDisk();
    int run();
};

#endif // CRAWLER_H_INCLUDED