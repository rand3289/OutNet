#ifndef HTTP_H_INCLUDED
#define HTTP_H_INCLUDED

#include "sign.h"
#include "data.h"
#include "sock.h"
#include <vector>
#include <string>


// Writer and SignatureWriter are helper classes.
// They help components send data whether it needs to be signed or not.
class Writer {
protected:
    Sock* sock = nullptr;
public:
    virtual void init(Sock& socket){ sock = & socket; }
    virtual const PubSign* getSignature(){ return nullptr; }
    virtual int write(char* data, size_t size){ return sock->write(data, size); }
    virtual int writeString(const string& str){ return sock->writeString(str); }
};


class SignatureWriter: public Writer {
    Signature sign;
public:
    virtual void init(Sock& socket){ sock = & socket; sign.init(); }
    virtual const PubSign* getSignature(){ return &sign.getSignature(); }
    virtual int write(char* data, size_t size){
        sign.write(data, size);         // sign the data
        return sock->write(data, size); // send data to remote client
    }
    virtual int writeString(const string& str){
        sign.write(str.c_str(), str.length() );
        return sock->writeString(str); }
};


struct Request{ // parse() returns QUERY bit field
    static int parse(Sock & conn, std::vector<std::string>& filters, unsigned short& port);
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