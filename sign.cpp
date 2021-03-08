#include "sign.h"
#include <iostream>
#include <fstream>
using namespace std;

static const string pubKeyFile = "public.key";
static const string privateKeyFile = "private.key";

char* PubKey::loadFromDisk(){
    ifstream pubkey(pubKeyFile, std::ios::binary);
    if(!pubkey){ cerr << "ERROR loading public key from disk." << endl; return nullptr; }
    pubkey >> key;
    if(!pubkey){ cerr << "ERROR loading public key from disk." << endl; return nullptr; }
    return key;
}


Signature::Signature(){ // load private key from disk
    ifstream privkey(privateKeyFile, std::ios::binary);
    if(!privkey){ cerr << "ERROR loading private key from disk." << endl; }
    privkey >> privateKey.key;
    if(!privkey){ cerr << "ERROR loading private key from disk." << endl; }
}


int Signature::init(){ // prepare to sign data written via write()
    memset(&signature, 0, sizeof(signature) );
    return 0;
}


int Signature::write(const void* data, size_t size){ // compute "PubSign sign" from data chunks
    return 0; // TODO:
}


int SignatureVerify::init(PubKey& publicKey){ // prepare to verify the signature
    memcpy(&pubKey, &publicKey, sizeof(pubKey) );
    memset(&signature, 0, sizeof(signature) );
    return 0;
}

bool SignatureVerify::verify(PubSign& sign){
    return 0 == memcmp(&signature, &sign, sizeof(PubSign) );
}

int SignatureVerify::write(void* data, size_t size){ // compute "PubSign sign" from data chunks
    return 0; // TODO:
}
