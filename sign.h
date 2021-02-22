#ifndef SIGN_H_INCLUDED
#define SIGN_H_INCLUDED

struct PubKey  { char  key[256]; };
struct PubSign { char sign[256]; };

class Signature {
    PubSign sign;
public:
    Signature(); // load public key from disk
    int copyPublicKey(PubKey* key);

    int init();                      // clear "PubSign sign"
    int write(char* data, int size); // compute "PubSign sign" from data chunks
    PubSign& getSignature(){ return sign; }
};

#endif //SIGN_H_INCLUDED