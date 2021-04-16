#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED
#include <string>
#include <vector>
// collection of utility functions
// It is sad that there are no standard versions of these functions in std::
// Probably why python is more popular :(

void turnBitsOff(uint32_t& mask, uint32_t bits); // turn bits off in a mask

int32_t timeMinutes(); // get time in minutes since Jan 1, 1970

// break string into tokens
bool tokenize( char*& buffer, const char* bufferEnd, char*& token, const std::string& separators );

std::string& toLower(std::string& s); // convert string to lower case in place

std::string& toUpper(std::string& s); // convert string to upper case in place

std::string& ltrim(std::string& s); // trim white spaces on the left in place

std::string& rtrim(std::string& s); // trim white spaces on the right in place

// read lines from a stream and put them in a vector
void parseLines(std::istream& stream, std::vector<std::string>& lines);

// run parseLines() on all files with specific extension
int parseFilesIntoLines(const std::string& extension, std::vector<std::string>& lines);

// Determine if str is a key=value pair and parse it into key and value parameters.
// Return true if str is a key=value pair.
bool keyValue(const std::string& str, std::string& key, std::string& value);

void printHex(const unsigned char* buff, int len); // print buffer using hex digits (4 + space)
void printAscii(const unsigned char* buff, int len); // ascii printable chars or "."

int writeString(std::ofstream& file, const std::string& str); // write string to file. format: size+string
std::string readString(std::ifstream& file);             // read string from file written by writeString()


template <typename SIZET> // uint32_t or uint64_t
class Buffer {
    SIZET len;    // size of data in buffer
    SIZET maxLen; // allocated memory size
    SIZET offset; // read() returns data from buff[offset]
    char* buff;   // allocated memory pointer
public:
    Buffer(): len(0), maxLen(0), offset(0), buff(nullptr) {}
    virtual ~Buffer(){ if(buff){ free(buff); } }

    void reset() { len = 0; offset = 0; }
    char* get()  { return buff; }
    SIZET size() { return len; }

    SIZET read(void* bytes, SIZET count){
        SIZET size = min(count, len-offset);
        memcpy(bytes, buff+offset, size);
        offset += size;
        return size;
    }

    void write(const void* bytes, SIZET count){
        char* where = reserve(count);
        memcpy(where, bytes, count);
    }

    char* reserve(SIZET bytes){
        char* where = buff+len;
        len += bytes;
        if(len > maxLen){
            maxLen = len < (64*1024) ? (64*1024) : 2*len;
            buff = (char*) realloc(buff, maxLen);
        }
        return where;
    }
};
typedef Buffer<uint32_t> Buffer32;
typedef Buffer<uint64_t> Buffer64;


#endif // UTILS_H_INCLUDED