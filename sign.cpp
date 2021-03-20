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


Signature::Signature(): signature(), privateKey() { // load private key from disk
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


int Signature::writeString(const string& str){
    unsigned char size = str.length();
    write(&size, sizeof(size));
    write(str.c_str(), size);
    return size;
}


int SignatureVerify::writeString(const string& str){
    unsigned char size = str.length();
    write(&size, sizeof(size));
    write(str.c_str(), size);
    return size;
}


int SignatureVerify::init(const PubKey& publicKey){ // prepare to verify the signature
    memcpy(&pubKey, &publicKey, sizeof(pubKey) );
    memset(&signature, 0, sizeof(signature) );
    return 0;
}


bool SignatureVerify::verify(const PubSign& sign) const {
    return 0 == memcmp(&signature, &sign, sizeof(PubSign) );
}


int SignatureVerify::write(const void* data, size_t size){ // compute "PubSign sign" from data chunks
    return 0; // TODO:
}
