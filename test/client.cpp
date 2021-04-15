#include "client.h"
#include "sock.h"
#include "sign.h"
#include <iostream>
#include <sstream>
#include <memory>
#include <vector>
#include <string>
#include <cstring> // strstr()
using namespace std;


static bool ERR(const string& msg){ // makes error handling code a bit shorter
    cerr << "ERROR " << msg << endl;
    return false;
}

static void turnBitsOff(uint32_t& mask, uint32_t bits){
    mask = mask & (0xFFFFFFFF^bits); // TODO: return a new mask instead of taking a reference???
}


// local services add themselves by connecting to outnet with SPORT= set to their server port.
// when outnet connects to it, reply without signing the response.
// when OutNet detects local (non-routable) address, it adds services as its own local services.
bool queryService(uint32_t host, uint16_t port, uint32_t select, vector<string> services, vector<HostInfo>& peers,
        shared_ptr<PubKey>& pubKey, uint16_t serverPort=0, int rwTimeout=10, vector<string>* filters = nullptr){
// select contains what you want OutNet to return in your query
// server has to have host, port and optionally key filled in before the call
// upon return server contains local services, local key and signatureVerified flag.
// upon return peers contains a list of peers for your service to connect to
// myPort is used for registering your local service with OutNet service
// rwTimeout is a Read/Write time out in seconds for send() and recv() network operations
// filters is a list of filters you want OutNet to apply before returning the results
//int queryService(uint32_t select, HostInfo& server, vector<HostInfo>& peers, uint16_t myPort=0, int rwTimeout=10, vector<string>* filters = nullptr){

    if( pubKey ){ // if we have remote's public key, do not request it
        turnBitsOff(select, SELECTION::LKEY);
    }

    stringstream ss;
    ss << "GET /?QUERY=" << select;
    if(serverPort>0){ // ad own server port for remote to connect back to
        ss << "&SPORT=" << serverPort;
    }
    if(filters){ // did the caller include any query parameters?
        for(string& f: *filters){ ss << "&" << f; }
    }
    ss << " HTTP/1.1\r\n\r\n";

    cout << "Connecting to " << Sock::ipToString(host) << ":" << port << endl;
    Sock sock;
    sock.setRWtimeout(rwTimeout); // seconds read/write timeout
    sock.setNoDelay(true); // request is one write()

    if( sock.connect(host, port) ){
        cerr  << "ERROR connecting to " << Sock::ipToString(host) << ":" << port << endl;
        return 0;
    }

    int len = (int) ss.str().length();
    if(len != sock.write(ss.str().c_str(), len ) ){
        return ERR("sending HTTP request");
    }

    // parse the response into unverifiedData
    // read HTTP/1.1 200 OK\r\n\r\n
    char buff[256];
    int rdsize = sock.readLine(buff, sizeof(buff) );
    if( rdsize < 8 || nullptr == strstr(buff,"200") ) {
        cerr << "ERROR in queryRemoteService() while parsing " << rdsize << " bytes: " << buff << endl;
        return 0;
    }
    while( sock.readLine(buff, sizeof(buff) ) > 0 ) {} // skip till empty line is read (HTTP protocol)

/************ Everything read below this line is signed *************/
    Signature signer;
    const bool sign = select & SELECTION::SIGN; // is signature requested?
    if(sign){ signer.init(); }

    uint32_t selectRet; // SELECT is always received
    rdsize = sock.read(&selectRet, sizeof(selectRet) );
    if(rdsize != sizeof(selectRet)){
        return ERR("reading 'select'");
    }
    if(sign){ signer.write(&selectRet, sizeof(selectRet)); } // signer gets data in network byte order
    selectRet = ntohl(selectRet);

    if( sign && !(selectRet & SELECTION::SIGN) ){ // we requested a signature but it was not returned
        return ERR("remote refused to sign response"); // this is a security problem
    }

    if(selectRet & SELECTION::LKEY){
        pubKey = make_shared<PubKey>();
        rdsize = sock.read(&*pubKey, sizeof(PubKey));
        if( rdsize != sizeof(PubKey) ){
            return ERR("reading remote public key");
        }
        if(sign){ signer.write(&*pubKey, sizeof(PubKey)); }
    }

    if(selectRet & SELECTION::TIME){
        uint32_t timeRemote;
        rdsize = sock.read(&timeRemote, sizeof(timeRemote) );
        if(rdsize != sizeof(timeRemote) ){
            return ERR("reading remote's time.");
        }
        if(sign){ signer.write(&timeRemote, sizeof(timeRemote)); }
        timeRemote = ntohl(timeRemote);
        // check that timestamp is not too long in the past, otherwise it can be a replay attack
        uint32_t now = (uint32_t) time(nullptr); // unix time does not depend on a timezone
        if( now - timeRemote > 10*60 ){
            return ERR("remote time stamp is older than 10 minutes!  Discarding data.");
        }
    }

    if( selectRet & SELECTION::LSVC ){
        uint16_t count;
        rdsize = sock.read(&count, sizeof(count));
        if(rdsize != sizeof(count) ){
            return ERR("reading remote service count.");
        }
        if(sign){ signer.write(&count, sizeof(count)); }
        count = ntohs(count);

        for(int i=0; i < count; ++i){
            rdsize = sock.readString(buff, sizeof(buff));
            if(rdsize <=0){
                return ERR("reading remote serices.");
            }
            services.push_back(buff);
            if(sign){ signer.writeString(buff); }
        }
    }

    // remote data
    uint32_t count;
    rdsize = sock.read(&count, sizeof(count) ); // count is always there even if data is not
    if(rdsize != sizeof(count) ){
        return ERR("reading HostInfo count.");
    }
    if(sign){ signer.write(&count, sizeof(count)); }
    count = ntohl(count);

    for(uint32_t i=0; i< count; ++i){
        HostInfo& hil = peers.emplace_back();

        if( selectRet & SELECTION::IP ){ // IP does not need ntohl()
            rdsize = sock.read( &hil.host, sizeof(hil.host));
            if(rdsize != sizeof(hil.host)){
                return ERR("reading IP.");
            }
            if(sign){ signer.write(&hil.host, sizeof(hil.host) ); }
        }

        if( selectRet & SELECTION::PORT ){
            rdsize = sock.read(&hil.port, sizeof(hil.port) );
            if(rdsize != sizeof(hil.port) ){
                return ERR("reading port.");
            }
            if(sign){ signer.write(&hil.port, sizeof(hil.port)); }
            hil.port = ntohs(hil.port);
        }

        if( selectRet & SELECTION::AGE ){
            uint16_t age;
            rdsize = sock.read(&age, sizeof(age) );
            if(rdsize != sizeof(age) ){
                return ERR("reading age.");
            }
            if(sign){ signer.write(&age, sizeof(age)); }
            hil.age = ntohs(age);
        }

        if( selectRet & SELECTION::RKEY ){
            unsigned char keyCount = 0;
            rdsize = sock.read( &keyCount, sizeof(keyCount) );
            if( rdsize != sizeof(keyCount) ){
                return ERR("reading key count.");
            }
            if(sign){ signer.write(&keyCount, sizeof(keyCount)); }
            if(keyCount){ // for now only one key is allowed
                hil.key = make_shared<PubKey> ();
                rdsize = sock.read( &*hil.key, sizeof(PubKey) );
                if(rdsize != sizeof(PubKey) ){
                    return ERR("reading public key.");
                }
                if(sign){ signer.write(&*hil.key, sizeof(PubKey)); }
            }
        }

        if(selectRet & SELECTION::ISCHK){
            unsigned char chk;
            sock.read(&chk, sizeof(chk) );
            if(rdsize != sizeof(PubKey) ){
                return ERR("reading signatureVerified.");
            }
            hil.signatureVerified = chk;
        }

        if( selectRet & SELECTION::RSVC ){
            uint16_t cnt;
            rdsize = sock.read(&cnt, sizeof(cnt) );
            if(rdsize != sizeof(cnt) ){
                return ERR("reading remote service count.");
            }
            if(sign){ signer.write(&cnt, sizeof(cnt)); }
            cnt = ntohs(cnt);

            for(int i=0; i< cnt; ++i){
                rdsize = sock.readString(buff, sizeof(buff));
                if(rdsize <=0){
                    return ERR("reading remote services.");
                }
                if(sign){ signer.writeString(buff); }
                hil.services.push_back(buff);
            }
        }
    } // for (adding HostInfo)

    if(sign){
        PubSign signature;
        if( sizeof(signature) != sock.read( &signature, sizeof(signature) ) ) {
            return ERR("reading signature from remote host");
        }

        // If signature can not be verified, discard the data
        if( !signer.verify(signature, *pubKey) ){
            return ERR("verifying signature");
        }
    }

    return true;
}
