#ifndef HTTP_H_INCLUDED
#define HTTP_H_INCLUDED

#include "sign.h"
#include "data.h"
#include "sock.h"
#include <vector>
#include <string>


// Writer and SignatureWriter are helper classes to help components send data
// when it needs to be signed or not
struct Writer {
    Sock* sock = nullptr;
public:
    inline virtual int write(char* data, size_t size){ return sock->write(data, size); }
    inline virtual PubSign* getSignature(){ return nullptr; }
    inline virtual void init(Sock& socket){ sock = & socket; }
    int writeString(const string& str); // writes unsigned char size (255max) + string without null
};


class SignatureWriter: public Writer {
    Signature sign;
public:
    inline virtual PubSign* getSignature(){ return &sign.getSignature(); }
    inline virtual void init(Sock& socket){ sock = & socket; sign.init(); }
    inline virtual int write(char* data, size_t size){
        sign.write(data, size);        // sign the data
        return sock->write(data, size); // send data to remote client
    }
};


class Request{
public:
    static int parseRequest(Sock & conn, std::vector<std::string>& filters); // returns QUERY bit field
};


class Response{
    Writer dumbWriter;
    SignatureWriter signatureWriter;
public:
    int write(Sock& conn, int select, std::vector<std::string>& filters, LocalData& ldata, RemoteData& rdata, BWLists& lists);
    static void writeDebug(Sock& conn, int select, std::vector<std::string>& filters);
};


#endif // HTTP_H_INCLUDED