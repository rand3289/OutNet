#include "client.h"
#include "sock.h"
#include <cstring>  // strstr()
#include <iostream>
#include <sstream>
using namespace std;
#include <chrono>
using namespace std::chrono;


static bool ERR(const string& msg){ // makes error handling code a bit shorter
    cerr << "ERROR " << msg << endl;
    return false;
}


int queryOutNet(uint32_t select, HostInfo& outnet, vector<HostInfo>& peers, uint16_t myPort, int rwTimeout, vector<string>* filters ){
    stringstream ss;
    ss << "GET /?QUERY=" << select;
    if(myPort>0){ // ad own server port for remote to connect back to
        ss << "&SPORT=" << myPort;
    }
    if(filters){ // did the caller include any query parameters?
        for(string& f: *filters){ ss << "&" << f; }
    }
    ss << " HTTP/1.1\r\n\r\n";

    Sock sock;
    sock.setRWtimeout(rwTimeout); // seconds read/write timeout
    sock.setNoDelay(true); // request is one write()

    if( sock.connect(outnet.host, outnet.port) ){
        cerr  << "ERROR connecting to " << Sock::ipToString(outnet.host) << ":" << outnet.port << endl;
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
    const bool sign = select & SELECT::SIGN; // is signature requested?
    if(sign){ signer.init(); }

    uint32_t selectRet; // SELECT is always received
    rdsize = sock.read(&selectRet, sizeof(selectRet) );
    if(rdsize != sizeof(selectRet)){
        return ERR("reading 'select'");
    }
    if(sign){ signer.write(&selectRet, sizeof(selectRet)); } // signer gets data in network byte order
    selectRet = ntohl(selectRet);

    if( sign && !(selectRet & SELECT::SIGN) ){ // we requested a signature but it was not returned
        return ERR("remote refused to sign response"); // this is a security problem
    }

    if(selectRet & SELECT::LKEY){
        outnet.key = make_shared<PubKey>();
        rdsize = sock.read(&*outnet.key, sizeof(PubKey));
        if( rdsize != sizeof(PubKey) ){
            return ERR("reading remote public key");
        }
        if(sign){ signer.write(&*outnet.key, sizeof(PubKey)); }
    }

    if(selectRet & SELECT::TIME){
        int32_t timeRemote;
        rdsize = sock.read(&timeRemote, sizeof(timeRemote) );
        if(rdsize != sizeof(timeRemote) ){
            return ERR("reading remote's time.");
        }
        if(sign){ signer.write(&timeRemote, sizeof(timeRemote)); }
        timeRemote = ntohl(timeRemote);

        // check that timestamp is not too long in the past, otherwise it can be a replay attack
        static auto epoch = system_clock::from_time_t(0);
        int32_t timeMinutes = duration_cast<minutes>(system_clock::now() - epoch).count();
        int32_t minOld = timeMinutes - timeRemote; // does not depend on a timezone
        if( abs(minOld) > 5 ){
            return ERR("Remote time difference is " + to_string(minOld) + " minutes.  Discarding data.");
        }
    }

    if( selectRet & SELECT::LSVC ){
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
            outnet.services.push_back(buff);
            if(sign){ signer.writeString(buff); }
        }
    }

/******************** remote data *************************/
    uint32_t count;
    rdsize = sock.read(&count, sizeof(count) ); // count is always there even if data is not
    if(rdsize != sizeof(count) ){
        return ERR("reading HostInfo count.");
    }
    if(sign){ signer.write(&count, sizeof(count)); }
    count = ntohl(count);

    for(uint32_t i=0; i< count; ++i){
        HostInfo& hil = peers.emplace_back();

        if( selectRet & SELECT::IP ){ // IP does not need ntohl()
            rdsize = sock.read( &hil.host, sizeof(hil.host));
            if(rdsize != sizeof(hil.host)){
                return ERR("reading IP.");
            }
            if(sign){ signer.write(&hil.host, sizeof(hil.host) ); }
        }

        if( selectRet & SELECT::PORT ){
            rdsize = sock.read(&hil.port, sizeof(hil.port) );
            if(rdsize != sizeof(hil.port) ){
                return ERR("reading port.");
            }
            if(sign){ signer.write(&hil.port, sizeof(hil.port)); }
            hil.port = ntohs(hil.port);
        }

        if( selectRet & SELECT::AGE ){
            uint16_t age;
            rdsize = sock.read(&age, sizeof(age) );
            if(rdsize != sizeof(age) ){
                return ERR("reading age.");
            }
            if(sign){ signer.write(&age, sizeof(age)); }
            hil.age = ntohs(age);
        }

        if( selectRet & SELECT::RKEY ){
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

        if(selectRet & SELECT::ISCHK){
            unsigned char chk;
            rdsize = sock.read(&chk, sizeof(chk) );
            if( rdsize != sizeof(chk) || chk>1 ){ // chk should be 0 or 1
                return ERR("reading signatureVerified.");
            }
            hil.signatureVerified = chk;
        }

        if( selectRet & SELECT::RSVC ){
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

    if(sign){ // If signature can not be verified, discard the data
        PubSign signature;
        if( sizeof(signature) != sock.read( &signature, sizeof(signature) ) ) {
            return ERR("reading signature from remote host");
        }

        if( !signer.verify(signature, *outnet.key) ){
            return ERR("verifying signature");
        }
    }

    return true;
}
