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
    virtual int read(char* data, size_t size){ return sock->read(data, size); }
    virtual int write(char* data, size_t size){ return 0; }
    int readString(char* buff); // reads char size (255max) + string without null. Appends 0 to buff
    short readShort(bool& error){ return sock->readShort(error); }
    long  readLong( bool& error){ return sock->readLong( error); }
};


class SignatureReader: public Reader{
    SignatureVerify sign;
public:
    virtual void init(Sock& socket, PubKey& key){ sock = & socket; sign.init(key); }
    virtual bool verifySignature(PubSign& signature){ return sign.verify(signature); }
    virtual int read(char* data, size_t size){
        int ret = read(data,size);
        sign.write(data, ret); // sign the data
        return ret;
    }
    virtual int write(char* data, size_t size){ return sign.write(data,size); }
};


class Crawler {
    Reader dumbReader;
    SignatureReader signatureReader;
    BWLists& bwlists;
    RemoteData* data=nullptr;
    int merge(vector<HostInfo>& newData);
    int queryRemoteService(HostInfo& hi, vector<HostInfo>& newData, const int select, unsigned short sport);
public:
    Crawler(BWLists& bw_lists): bwlists(bw_lists) {}
    int loadFromDisk(RemoteData& rdata);
    int saveToDisk();
    int run(unsigned short sport); // sport = server port
};

#endif // CRAWLER_H_INCLUDED