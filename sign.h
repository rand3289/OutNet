#ifndef SIGN_H_INCLUDED
#define SIGN_H_INCLUDED
#include <cstring> // size_t, memset(), memcpy(), memcmp()

struct PubSign    { char sign[256]; };
struct PrivateKey { char  key[256]; };
struct PubKey     {
    char  key[256];
    char* loadFromDisk();
};


class Signature {
    PubSign signature;
    PrivateKey privateKey; // TODO: make this static. Load it from disk once.
public:
    Signature(); // load private key from disk
    int init();  // clear "PubSign signature"
    int write(const void* data, size_t size); // compute signature from data chunks
    const PubSign& getSignature(){ return signature; }
};


class SignatureVerify {
    PubSign signature;
    PubKey pubKey;
public:
    int init(PubKey& pubKey);           // prepare to verify the signature
    int write(void* data, size_t size); // compute "PubSign sign" from data chunks
    bool verify(PubSign& signature);    // did computed signature match given signature?
};

#endif //SIGN_H_INCLUDED