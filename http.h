#ifndef HTTP_H_INCLUDED
#define HTTP_H_INCLUDED

#include "data.h"
#include "sock.h"
#include <vector>
#include <string>


class Request{
public:
    static int parseRequest(Sock & conn, std::vector<std::string>& filters); // returns QUERY bit field
};

class Response{
public:
    static int write(Sock& conn, int select, std::vector<std::string>& filters, LocalData& ldata, RemoteData& rdata, BWLists& lists);
};



#endif // HTTP_H_INCLUDED