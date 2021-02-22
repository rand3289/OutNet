#ifndef HTTP_H_INCLUDED
#define HTTP_H_INCLUDED

#include "data.h"
#include "sock.h"
#include <vector>
#include <string>
using namespace std;

class Request{
public:
    Request(Sock& conn);
    int parseRequest(vector<string>& filters);
};

class Response{
public:
    Response(Sock& conn);
    int write(int select, vector<string>& filters, LocalData& ldata, RemoteData& rdata);
};

#endif // HTTP_H_INCLUDED