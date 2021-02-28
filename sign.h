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
    int copyPublicKey(PubKey* key);

    int init(){ return 0; }                      // clear "PubSign sign"
    int write(char* data, int size){ return 0; } // compute "PubSign sign" from data chunks
    PubSign& getSignature(){ return signature; }
};

#endif //SIGN_H_INCLUDED