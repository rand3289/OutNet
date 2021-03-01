#ifndef HTTP_H_INCLUDED
#define HTTP_H_INCLUDED

#include "sign.h"
#include "data.h"
#include "sock.h"
#include <vector>
#include <string>


// Writer and SignatureWriter are helper classes.
// They help components send data whether it needs to be signed or not.
struct Writer {
    Sock* sock = nullptr;
public:
    inline virtual void init(Sock& socket){ sock = & socket; }
    inline virtual PubSign* getSignature(){ return nullptr; }
    inline virtual int write(char* data, size_t size){ return sock->write(data, size); }
    int writeString(const string& str); // writes unsigned char size (255max) + string without null
};


class SignatureWriter: public Writer {
    Signature sign;
public:
    inline virtual void init(Sock& socket){ sock = & socket; sign.init(); }
    inline virtual PubSign* getSignature(){ return &sign.getSignature(); }
    inline virtual int write(char* data, size_t size){
        sign.write(data, size);        // sign the data
        return sock->write(data, size); // send data to remote client
    }
};


struct Request{
    static int parse(Sock & conn, std::vector<std::string>& filters); // returns QUERY bit field
};


class Response{
    Writer dumbWriter;
    SignatureWriter signatureWriter;
public:
    int write(Sock& conn, int select, std::vector<std::string>& filters, LocalData& ldata, RemoteData& rdata, BWLists& lists);
    static void writeDebug(Sock& conn, int select, std::vector<std::string>& filters);
    static void writeDenied(Sock& conn);
};


#endif // HTTP_H_INCLUDED