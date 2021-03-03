#ifndef CRAWLER_H_INCLUDED
#define CRAWLER_H_INCLUDED
#include "data.h"
#include "sock.h"
#include <cstring> // memcmp()

class Reader{
protected:
    Sock* sock = nullptr;
public:
    inline virtual void init(Sock& socket){ sock = & socket; }
    inline virtual bool verifySignature(PubSign& signature){ return true; }
    inline virtual int read(char* data, size_t size){ return sock->read(data, size); }
    int readString(string& str); // reads unsigned char size (255max) + string without null
    short readShort(bool& error){ return sock->readShort(error); }
    long  readLong( bool& error){ return sock->readLong( error); }
};


class SignatureReader: public Reader{
    Signature sign;
public:
    inline virtual void init(Sock& socket){ sock = & socket; sign.init(); }
    inline virtual bool verifySignature(PubSign& signature)
        { return 0==memcmp( &signature, &sign.getSignature(), sizeof(PubSign) ); }
    inline virtual int read(char* data, size_t size){
        int ret = read(data,size);
        sign.write(data, ret); // sign the data
        return ret;
    }
};


class Crawler {
    Reader dumbReader;
    SignatureReader signatureReader;
    BWLists& bwlists;
    RemoteData* data=nullptr;
    int merge(vector<HostInfo>& newData);
    int queryRemoteService(HostInfo& hi, vector<HostInfo>& newData);
public:
    Crawler(BWLists& bw_lists): bwlists(bw_lists) {}
    int loadFromDisk(RemoteData& rdata);
    int saveToDisk();
    int run();
};

#endif // CRAWLER_H_INCLUDED