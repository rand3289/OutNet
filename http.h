#ifndef HTTP_H_INCLUDED
#define HTTP_H_INCLUDED

#include "sign.h"
#include <vector>
#include <string>

class Sock;
class LocalData;
class RemoteData;

struct Request{ // parse() returns QUERY bit field
    static int parse(Sock & conn, std::vector<std::string>& filters, uint16_t& port);
};


class Response{
    Signature signer;
public:
    int write(Sock& conn, uint32_t select, std::vector<std::string>& filters, LocalData& ldata, RemoteData& rdata);
    static void writeDebug(Sock& conn, uint32_t select, std::vector<std::string>& filters);
    static void writeDenied(Sock& conn, const std::string& reason);
};


#endif // HTTP_H_INCLUDED