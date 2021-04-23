#ifndef SIGN_H_INCLUDED
#define SIGN_H_INCLUDED
#include <cstring> // size_t, memset(), memcpy(), memcmp()
#include <string>
#include "tweetnacl.h"
#include "buffer.h"

#define SIGNATURE_SIZE (crypto_sign_BYTES)
#define PUBKEY_SIZE    (crypto_sign_PUBLICKEYBYTES)
#define SECRETKEY_SIZE (crypto_sign_SECRETKEYBYTES)


struct PubSign    { unsigned char sign[SIGNATURE_SIZE]; };
struct PrivateKey { unsigned char  key[SECRETKEY_SIZE]; };

struct PubKey {
    unsigned char  key[PUBKEY_SIZE];
    bool operator==(const PubKey& rhs) const { return 0==memcmp( &key, &rhs.key, sizeof(key) ); }
    bool operator< (const PubKey& rhs) const { return 0 >memcmp( &key, &rhs.key, sizeof(key) ); }
//    PubKey& operator=(const PubKey& rhs){ memcpy(key, rhs.key, sizeof(key)); return *this; }
//    PubKey(const PubKey& rhs){ memcpy(key, rhs.key, sizeof(key)); }
};


class Signature {
    Buffer32 buff;
    static bool keysLoaded;
    static PrivateKey privateKey; // static field loaded by loadKeys()
    static PubKey     publicKey;  // cached publicKey - access it with loadKeys()
public:
    static bool loadKeys(PubKey& publicKey); // load both keys, but return public key only.
    int init();
    int write(const void* data, size_t size);
    int writeString(const std::string& str);
    bool generate(PubSign& pubSign);
    bool verify(const PubSign& signature, const PubKey& pubKey);
};


#endif //SIGN_H_INCLUDED