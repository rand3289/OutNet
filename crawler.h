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
    virtual int write(char* data, size_t size){ return 0; }
    virtual int read(char* data, size_t size){ return sock->read(data, size); }
    virtual int readString(char* buff){ return sock->readString(buff); }
    virtual short readShort(bool& error){ return sock->readShort(error); }
    virtual long  readLong( bool& error){ return sock->readLong( error); }
};


class SignatureReader: public Reader{
    SignatureVerify sign;
public:
    virtual void init(Sock& socket, PubKey& key){ sock = & socket; sign.init(key); }
    virtual bool verifySignature(PubSign& signature){ return sign.verify(signature); }
    virtual int write(char* data, size_t size){ return sign.write(data,size); }
    virtual int read(char* data, size_t size){
        int ret = sock->read(data,size);
        sign.write(data, ret); // sign the data
        return ret;
    }
    virtual int readString(char* buff){
        int ret = sock->readString(buff);
        sign.write(buff,ret);
        return ret;
    }
    virtual short readShort(bool& error){
        short ret = sock->readShort(error);
        sign.write((char*)&ret, sizeof(ret));
        return ret;
    }
    virtual long readLong( bool& error){
        long  ret = sock->readLong( error);
        sign.write((char*)&ret, sizeof(ret));
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
    int queryRemoteService(HostInfo& hi, vector<HostInfo>& newData, const int select, HostPort& self);
public:
    Crawler(LocalData& locData, BWLists& bw_lists): ldata(locData), bwlists(bw_lists) {}
    int loadFromDisk(RemoteData& rdata);
    int saveToDisk();
    int run();
};

#endif // CRAWLER_H_INCLUDED