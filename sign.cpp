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

static const string pubKeyFile = "publicOutNet.key";
static const string privateKeyFile = "secretOutNet.key";
bool       Signature::keysLoaded = false; // static
PrivateKey Signature::privateKey;         // static
PubKey     Signature::publicKey;          // static


// Load both keys at once.  If one file is missing, regenerate BOTH and save.
bool Signature::loadKeys(PubKey& pubKey){ // static
    if(keysLoaded){ // TODO: return const reference instead???
        memcpy(pubKey.key, publicKey.key, sizeof(publicKey));
        return true;
    }

    ifstream pubkey(pubKeyFile, std::ios::binary);
    ifstream seckey(privateKeyFile, std::ios::binary);

    if( pubkey.good() && seckey.good() ){ // both files are there
        pubkey.read( (char*) publicKey.key, sizeof(publicKey) );
        if( pubkey.good() ){ // read public key successfuly
            memcpy(pubKey.key, publicKey.key, sizeof(publicKey)); // return it to caller
            seckey.read( (char*) privateKey.key, sizeof(privateKey) );
            if(seckey.good() ){
                keysLoaded = true;
                return true;
            }
        }
        rename(privateKeyFile.c_str(), (privateKeyFile+".old").c_str() );
        rename(pubKeyFile.c_str(), (pubKeyFile+".old").c_str() );
        cout << "WARNING: one of the key files could not be read." << endl;
        cout << "Both files had to be renamed to *.old and will be regenerated." << endl;
    } else if( (!pubkey ^ !seckey) ){ // one of them exists instead of both.  Rename it.
        const string& exists = !pubkey ? privateKeyFile : pubKeyFile;
        rename(privateKeyFile.c_str(), (privateKeyFile+".old").c_str() );
        cout << "WARNING: missing ONE private or public key file!" << endl; 
        cout << "Renamed existing file "<< exists << " to " << exists << ".old" << endl;
    } else {
        cout << "Public and private key files are missing and will be generated." << endl;
    }
    pubkey.close();
    seckey.close();

    crypto_sign_keypair(publicKey.key, privateKey.key); // Create new public and private keys
    cout << "Generated NEW private and public keys." << endl;
    memcpy(pubKey.key, publicKey.key, sizeof(publicKey));

    ofstream pubk(pubKeyFile, std::ios::binary);
    ofstream seck(privateKeyFile, std::ios::binary);
    pubk.write( (char*) publicKey.key, sizeof(publicKey) );
    seck.write( (char*) privateKey.key, sizeof(privateKey) );
    if(!pubk || !seck){ // check that the writes succeeded
        cerr << "ERROR saving keys.  Do you have write permissions?  Fix it and run OutNet again." << endl;
        return false;
    }

    keysLoaded = true;
    return true;
}


int Signature::init(){ // prepare to sign data written via write()
    buff.reset();
    buff.reserve( SIGNATURE_SIZE ); // reserve space for signature
    return 0;
}


bool Signature::generate(PubSign& pubSign){
    unsigned char* start = (unsigned char*) buff.get() + SIGNATURE_SIZE;
    unsigned long long len = buff.size();// buff had SIGNATURE_SIZE bytes reserved in init()
    vector<unsigned char> m2(len);
    crypto_sign(&m2[0], &len, start, len-SIGNATURE_SIZE, privateKey.key);
    memcpy(pubSign.sign, &m2[0], SIGNATURE_SIZE); // first crypto_sign_BYTES is the signature
//    cout << "Signing " << buff.size() - SIGNATURE_SIZE << " bytes:" << endl;
//    printHex (start, buff.size() - SIGNATURE_SIZE);
//    cout << "Generated " << len << " bytes signed message:" << endl;
//    printHex( &m2[0], len);
//    printAscii( &m2[0], len);
    return true;
}


bool Signature::verify(const PubSign& sign, const PubKey& pubKey) {
    unsigned char* start = (unsigned char*) buff.get(); // pointer to the beginning of the buffer
    memcpy(start, sign.sign, sizeof(sign)); // prepend signature to the beginning of the buffer
    unsigned long long len = buff.size();   // len will be read twice & written to
    vector<unsigned char> m2(len);          // do not worry about extra SIGNATURE_SIZE bytes
//    cout << "Received "<< len << " bytes signed message:" << endl;
//    printHex( (unsigned char*) buff.get(), len);
//    printAscii( (unsigned char*) buff.get(), len);
//    cout << "Remote's public key: ";
//    printHex(pubKey.key, sizeof(pubKey));
    return !crypto_sign_open(&m2[0], &len, start, len, (unsigned char*) pubKey.key); // returns 0 on success
}


int Signature::write(const void* data, size_t size){ // compute "PubSign sign" from data chunks
    buff.write(data, (uint32_t) size);
    return 0;
}


int Signature::writeString(const string& str){
    unsigned char size = (unsigned char) str.length();
    write(&size, sizeof(size));
    write(str.c_str(), size);
    return size+1;
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
        } catch(...){
            device.reset(); // it can throw in operator() not just constructor
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
        std::uniform_int_distribution<int> distribution(0,255);
        for(unsigned long long i=0; i < byteCount; ++i){
            bytes[i] = distribution(generator);
        }
    }
}
