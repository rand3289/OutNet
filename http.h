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
    Sock& sock;
public:
    Writer(Sock& socket): sock(socket) {}
    inline virtual int write(char* data, size_t size){ return sock.write(data, size); }
    inline virtual PubSign* getSignature(){ return nullptr; }
    int writeString(const string& str); // writes unsigned char size (255max) + string without null
};


class SignatureWriter: public Writer {
    Signature sign;
public:
    SignatureWriter(Sock& socket): Writer(socket) {}
    inline virtual PubSign* getSignature(){ return &sign.getSignature(); }
    inline virtual int write(char* data, size_t size){
        sign.write(data, size);        // sign the data
        return sock.write(data, size); // send data to remote client
    }
};


class Request{
public:
    static int parseRequest(Sock & conn, std::vector<std::string>& filters); // returns QUERY bit field
};


class Response{
public:
    static int write(Sock& conn, int select, std::vector<std::string>& filters, LocalData& ldata, RemoteData& rdata, BWLists& lists);
    static void writeDebug(Sock& conn, int select, std::vector<std::string>& filters);
};


#endif // HTTP_H_INCLUDED