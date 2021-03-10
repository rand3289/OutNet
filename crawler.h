#ifndef CRAWLER_H_INCLUDED
#define CRAWLER_H_INCLUDED
#include "data.h"
#include "sock.h"

// Reader and SignatureReader are helper classes
// They help Crawler read data while verifying the signature
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
    uint32_t hostCopy;
    uint16_t portCopy;
    LocalData& ldata;
    RemoteData& rdata;
    BlackList& blist;
    int merge(vector<HostInfo>& newData);
public:
    int queryRemoteService(HostInfo& hi, vector<HostInfo>& newData, uint32_t select);
    Crawler(LocalData& lData, RemoteData& rData, BlackList& bList):
            hostCopy(0), portCopy(0), ldata(lData), rdata(rData), blist(bList) {}
    int loadRemoteDataFromDisk();
    int saveRemoteDataToDisk();
    int run();
};

#endif // CRAWLER_H_INCLUDED