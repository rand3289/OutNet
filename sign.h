#ifndef SIGN_H_INCLUDED
#define SIGN_H_INCLUDED

struct PubSign    { char sign[256]; };
struct PubKey     { char  key[256]; };
struct PrivateKey { char  key[256]; };


class Signature {
    PubSign signature;
    PubKey pubKey;
    PrivateKey privateKey;
public:
    Signature(){} // TODO: load public & private keys from disk
    int init(){ return 0; }                      // clear "PubSign sign"
    int write(const char* data, int size){ return 0; } // compute "PubSign sign" from data chunks
    const PubSign& getSignature(){ return signature; }
    const PubKey&  getPublicKey(){ return pubKey; }
};


class SignatureVerify {
public:
    int init(PubKey& pubKey){ return 0; }        // prepare to verify the signature
    int write(char* data, int size){ return 0; } // compute "PubSign sign" from data chunks
    bool verify(PubSign& signature){ return true;}
    // { return 0==memcmp( &signature, &blah, sizeof(PubSign) ); } // #include <cstring> // memcmp()
};

#endif //SIGN_H_INCLUDED