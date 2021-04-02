#include "sign.h"
#include "tweetnacl.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <memory>
#include <mutex> // call_once()
#include <random>
#include <chrono>
using namespace std;

static const string pubKeyFile = "public.key";
static const string privateKeyFile = "private.key";

char* PubKey::loadFromDisk(){
    ifstream pubkey(pubKeyFile, std::ios::binary);
    if(!pubkey){ cerr << "ERROR loading public key from disk." << endl; return nullptr; }
//     pubkey >> key; // BUG !!! reads more than size of key and overwrites other variables! TODO:
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
    unsigned char size = (unsigned char) str.length();
    write(&size, sizeof(size));
    write(str.c_str(), size);
    return size;
}


int SignatureVerify::writeString(const string& str){
    unsigned char size = (unsigned char) str.length();
    write(&size, sizeof(size));
    write(str.c_str(), size);
    return size;
}


int SignatureVerify::init(const PubKey& publicKey){ // prepare to verify the signature
    pubKey = publicKey; // memcpy(&pubKey, &publicKey, sizeof(pubKey) );
    memset(&signature, 0, sizeof(signature) );
    return 0;
}


bool SignatureVerify::verify(const PubSign& sign) const {
    return 0 == memcmp(&signature, &sign, sizeof(PubSign) );
}


int SignatureVerify::write(const void* data, size_t size){ // compute "PubSign sign" from data chunks
    return 0; // TODO:
}


// this function is called once by tweetnacl when creating a key pair (byteCount is 32)
void randombytes(unsigned char* bytes, unsigned long long byteCount){
//    cout << "(" << byteCount << ") ";
    static shared_ptr<random_device> device;
    static std::once_flag onceFlag;
    call_once(onceFlag, [](){ // try intilializing a true random device during first call
        try {
            device = make_shared<random_device>(); // This could throw.
            device->operator()(); // generate one random number.  This could throw.
            if(0 == device->entropy() ){ // pseudo random
                device.reset();          // we don't want it
                cout << "INFO: random device is not available on the system" << endl;
            }
        } catch(std::exception& err){ // or (...)
            device.reset(); // it can throw in operator()
            cout << "INFO: random device is not available on the system." << endl;
        }
    });

    if(device){
        for(unsigned long long i=0; i < byteCount; ++i){
            bytes[i] = device->operator()() % 256; // TODO: waisting 3/4 of entrophy here !!!
        }
    } else {
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::default_random_engine generator(seed);
        std::uniform_int_distribution<unsigned char> distribution(0,255);
        for(unsigned long long i=0; i < byteCount; ++i){
            bytes[i] = distribution(generator);
        }
    }
}