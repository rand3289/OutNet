#ifndef HTTP_H_INCLUDED
#define HTTP_H_INCLUDED

#include "sign.h"
#include <vector>
#include <array>
#include <string>

class Sock;
class LocalData;
class RemoteData;

class Request{ // parse() returns QUERY bit field
    static bool parseFilter(char* filter, std::array<std::string,3>& funcOperVal);
public:
    static int parse(Sock & conn, std::vector<std::array<std::string,3>>& filters, uint16_t& port);
};


class Response{
    Signature signer;
public:
    int write(Sock& conn, uint32_t select, std::vector<std::array<std::string,3>>& filters, LocalData& ldata, RemoteData& rdata);
    static void writeDebug(Sock& conn, uint32_t select, std::vector<std::array<std::string,3>>& filters);
    static void writeDenied(Sock& conn, const std::string& reason);
};


#endif // HTTP_H_INCLUDED