#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED
#include <cstring> // memcpy

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


#endif // BUFFER_H_INCLUDED