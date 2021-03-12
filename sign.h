#ifndef SIGN_H_INCLUDED
#define SIGN_H_INCLUDED
#include <cstring> // size_t, memset(), memcpy(), memcmp()
#include <string>

struct PubSign    { char sign[256]; };
struct PrivateKey { char  key[256]; };

struct PubKey     {
    char  key[256];
    char* loadFromDisk();
    bool operator==(const PubKey& rhs){ return 0==memcmp( &key, &rhs.key, sizeof(key) ); }
};


class Signature {
    PubSign signature;
    PrivateKey privateKey; // TODO: make this static. Load it from disk once.
public:
    Signature(); // load private key from disk
    int init();  // clear "PubSign signature"
    int write(const void* data, size_t size); // compute signature from data chunks
    int writeString(const std::string& str);
    const PubSign& getSignature() const { return signature; }
};


class SignatureVerify {
    PubSign signature;
    PubKey pubKey;
public:
    int init(const PubKey& pubKey);           // prepare to verify the signature
    int write(const void* data, size_t size); // compute "PubSign sign" from data chunks
    int writeString(const std::string& str);
    bool verify(const PubSign& signature) const;    // did computed signature match given signature?
};

#endif //SIGN_H_INCLUDED