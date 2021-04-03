#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED
#include <string>
#include <vector>
// collection of utility functions
// It is sad that there are no standard versions of these functions in std::
// Probably why python is more popular :(

void turnBitsOff(uint32_t& mask, uint32_t bits); // turn bits off in a mask

// break string into tokens
bool tokenize( char*& buffer, const char* bufferEnd, char*& token, const std::string& separators );

std::string& toLower(std::string& s); // convert string to lower case in place

std::string& ltrim(std::string& s); // trim white spaces on the left in place

std::string& rtrim(std::string& s); // trim white spaces on the right in place

// read lines from a stream and put them in a vector
void parseLines(std::istream& stream, std::vector<std::string>& lines);

// Determine if str is a key=value pair and parse it into key and value parameters.
// Return true if str is a key=value pair.
bool keyValue(const std::string& str, std::string& key, std::string& value);


template <typename SIZET> // uint32_t or uint64_t
class Buffer {
    SIZET len;
    SIZET maxLen;
    char* buff;
public:
    Buffer(): len(0), maxLen(0), buff(nullptr) {}
    virtual ~Buffer(){ if(buff){ free(buff); } }

    void reset() { len = 0; }
    char* get()  { return buff; }
    SIZET size() { return len; }

    void write(const void* bytes, SIZET count){
        char* where = reserve(count);
        memcpy(where, bytes, count);
    }
    char* reserve(SIZET bytes){
        SIZET oldLen = len;
        len += bytes;
        if(len > maxLen){
            maxLen = len < (64*1024) ? (64*1024) : 2*len;
            buff = (char*) realloc(buff, maxLen);
        }
        return buff+oldLen;
    }
};
typedef Buffer<uint32_t> Buffer32;
typedef Buffer<uint64_t> Buffer64;


#endif // UTILS_H_INCLUDED