#include "config.h"
#include "sock.h" // Sock::ANY_PORT, Sock::stringToIP()
#include "utils.h"
#include <algorithm>
#include <vector>
#include <thread>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
using namespace std;
using namespace std::filesystem;


// watch() looks in the current dir for files of the following types:
// *.badip *.badkey - blacklists of IPs or public keys
// *.service - service description file (one service description per line)
int Config::watch() {
    while(true){
        this_thread::sleep_for( seconds(refreshRate) );
        loadServiceFiles();
        loadBlackListFiles();
    }
}
// TODO: config could keep a list of ports forwarded on a router (with their lease times)
// if a service no longer accepts a connection or "rejects" a 0 size UDP packet,
// close the port and remove the service from the list


struct ServiceString: public string {
    bool operator<(const Service& serv) const { return compare(serv.originalDescription) < 0; }
}; // This class provides operator<(Service&) for string.  Use it with stl algorithms


struct IpPortProt {
    uint32_t ip;
    uint16_t port;
    uint8_t  prot;
    IpPortProt(uint32_t ip1, uint16_t port1, uint8_t prot1): ip(ip1), port(port1), prot(prot1) {}
};


// look for *.service in current directory and load their contents into LocalData::services
int Config::loadServiceFiles(){
    cout << "Loading service files: ";
    static const string extension = ".service";
    vector<string> lines;
    int fCount = parseFilesIntoLines(extension, lines);
    cout << "(" << fCount << " found)" << endl;
    std::sort( begin(lines), end(lines) ); // set_difference needs sorted data

    vector<string> newServices; // newServices(not in ldata->services)
    vector<IpPortProt> portsToOpen;
    unique_lock ulock(ldata->mutx);
    sort( begin(ldata->services), end(ldata->services) ); // set_difference needs sorted data
    vector<ServiceString>& s = * reinterpret_cast<vector<ServiceString>*>( &lines ); // provides string::operator<(Service&)
    set_difference( begin(s), end(s), begin(ldata->services), end(ldata->services), back_inserter(newServices));

//    vector<Service> oldServices; // oldServices(not in "lines")
//    set_difference( begin(ldata->services), end(ldata->services), begin(s), end(s), back_inserter(oldServices));
//    for(Service& serv: oldServices){ // delete old services
//        ldata->services.erase( remove( begin(ldata->services), end(ldata->services), serv ) );
//    }

    // insert new services and prepare to open ports for them
    for(const string& serv: newServices){
        auto s = ldata->addService(serv);
        if(!s) { continue; } // did it parse correctly?
        portsToOpen.emplace_back(s->ip, s->port, s->tcpudp[0]); // tcp/udp comes from service description
    }
    ulock.unlock(); // after unlocking we can open and close ports which can take a while

    for(auto& ipPort: portsToOpen){  // open router ports using UPnP protocol for new services
        string ip = Sock::ipToString(ipPort.ip);
        const char* protocol = ('t'==ipPort.prot) ? "TCP" : "UDP"; // u == UDP
        upnp.add_port_mapping("OutNet", ip.c_str(), ipPort.port, ipPort.port, protocol);
    }

//    for(auto& old: oldServices){ // close ports for old services
//        upnp.closePort(old.ip, old.port);
//    }
    return 0;
}


int Config::loadBlackListFiles(){ // load *.badkey and *.badip files
    static const string ext1 = ".badip";
    vector<string> lines;
    cout << "Loading IP blacklist files: ";
    int fCount = parseFilesIntoLines(ext1, lines);
    cout << "(" << fCount << " found)" << endl;

    static const string ext2 = ".badkey";
    vector<string> lines2;
    cout << "Loading KEY blacklist files: ";
    fCount = parseFilesIntoLines(ext2, lines2);
    cout << "(" << fCount << " found)" << endl;

    unique_lock ulock(blist->mutx);

    for(string& l: lines){ // load lines into blist->badHosts
        uint32_t ip = Sock::stringToIP( l.c_str() );
        if(ip > 0){
            blist->badHosts.push_back(ip);
        }
    }

    for(string& l: lines2){ // load lines2 into blist->badKeys
        if(l.length() == 2*PUBKEY_SIZE){ // each byte is 2 hex digits long
            PubKey& pk = blist->badKeys.emplace_back();
            for(int i=0; i < PUBKEY_SIZE; ++i){ // convert hex string to bytes
                string sub = l.substr(i*2, 2);  // each byte is 2 hex digits long
                pk.key[i] = strtol(sub.c_str(), nullptr, 16);
            }
        }
    }

    sort( blist->badHosts.begin(), blist->badHosts.end() ); // sort to make search faster
    sort( blist->badKeys.begin(),  blist->badKeys.end()  );
    return 0;
}


int Config::findIPs(){
    cout << "Looking for your NAT router..." << endl;
    if ( upnp.discovery() ){
        cout << "Retrieving this host's IP and WAN IP from the router..." << endl;
        string ipStr;
        upnp.getExternalIP(ipStr, ldata->localIP);
        cout << "Local IP: " << Sock::ipToString(ldata->localIP) << endl;
        if( Sock::isRoutable(ldata->localIP) ) {
            ldata->myIP = ldata->localIP;
            cout << "Local IP is not private (routable)." << endl;
            return 0;
        }
        if( ipStr.length() > 6){ // at least x.x.x.x
            ldata->myIP = Sock::stringToIP( ipStr.c_str() );
            if( ldata->myIP > 0 ){
                cout << "WAN IP: " << ipStr << endl;
                if( !Sock::isRoutable(ldata->myIP) ){
                    cout << "# WARNING: router's WAN IP is not routable.  You are behind multiple NAT devices." << endl;
                    cout << "# Manually configure your other NAT devices to forward ports to " << ipStr << endl;
                    cout << "# Otherwise no one can connect TO YOU from the internet." << endl;
                }
                return 0;
            }
        } // this is an indication there is no NAT taking place.
        cerr << "Error retrieving IPs from the router." << endl;
    } else {
        cerr << "Failed to find a NAT router.  ERROR: " << upnp.get_last_error() << endl;
    }
    //1 TODO: fill ldata->myIP and ldata->localIP without using router's help
    // connecto to something and get local IP that way.
    return 0;
}


// a member of type "const std::string" cannot have an in-class initializerC/C++(1591)
static const string configName = "outnet.cfg";

// load port and refresh rate from config file
void Config::init(LocalData& lData, BlackList& bList){
    ldata = &lData;
    blist = &bList;
    loadServiceFiles();
    loadBlackListFiles();
    findIPs();

    cout << "Loading configuration data." << endl;
    ldata->myPort = Sock::ANY_PORT; // default port for OutNet to listen on

    ifstream config (configName);
    if( !config ){
        cout << configName << " not found. Setting configuration data to defaults." << endl;
        return;
    }

    vector<string> lines;
    parseLines(config, lines);
    config.close();

    for(string& line: lines){
        string key, value;
        if( keyValue(line, key, value)){
            toLower(key);
            if( key == "sleepconfig" ){
                refreshRate = strtol(value.c_str(), nullptr, 10); // base 10
            } else if (key == "sleepserver"){
                ldata->sleepServer = strtol(value.c_str(), nullptr, 10); // base 10
            } else if (key == "sleepcrawler"){
                ldata->sleepCrawler = strtol(value.c_str(), nullptr, 10); // base 10
            } else if (key == "timeoutserver"){
                ldata->timeoutServer = strtol(value.c_str(), nullptr, 10); // base 10
            } else if (key == "timeoutcrawler"){
                ldata->timeoutCrawler = strtol(value.c_str(), nullptr, 10); // base 10
            } else if (key == "timeoutupnp"){
                ldata->timeoutUPNP = strtol(value.c_str(), nullptr, 10); // base 10
            } else if (key == "serverport"){
                ldata->myPort = strtol(value.c_str(), nullptr, 10); // base 10
            }
        }
    }

    if( ldata->myPort != Sock::ANY_PORT ) {
        cout << "Configuration data loaded successfuly." << endl;
    }else {
        cout << "Config file is corrupted.  It will be regenerated." << endl;
    }
}


int Config::saveToDisk(){
    cout << "Saving configuration data to disk." << endl;
    ofstream config(configName);
    string msg1 = "# Configuration file for OutNet service https://github.com/rand3289/OutNet";
    string msg2 = "# If this file is deleted or corrupted it will be regenerated with default values.";
    string msg3 = "# SleepZZZ and TimeoutZZZ are in seconds.";
    config << msg1 << endl << msg2 << endl << msg3 << endl << endl;

    shared_lock slock(ldata->mutx);
    config << "SleepConfig="   << refreshRate << "\t# how often do I look for new or updated files" <<  endl;
    config << "SleepServer="   << ldata->sleepServer << "\t# how long do I wait for client to close connection" <<  endl;
    config << "SleepCrawler="  << ldata->sleepCrawler << "\t# wait time between scanning other OutNet services" <<  endl;
    config << endl;
    config << "TimeoutServer="  << ldata->timeoutServer << "\t# server network read / write timeout" << endl;
    config << "TimeoutCrawler=" << ldata->timeoutCrawler << "\t# crawler network read / write timeout" << endl;
    config << "TimeoutUPNP="    << ldata->timeoutUPNP << "  \t# UPNP client network read / write timeout" << endl;
    config << endl;
    config << "ServerPort="    << ldata->myPort << "\t# server accepts connections on this port" << endl;
    return 0;
}


bool Config::forwardLocalPort(uint16_t port){
    string localAddr = Sock::ipToString(ldata->localIP);
    cout << "Forwarding router's WAN port " << port << " to local " << localAddr << ":" << port << endl;
    if( 0 == ldata->localIP || port == 0){
        cerr << "ERROR: local IP or PORT are blank! Can not forward port." << endl;
        return false;
    }
    if( !upnp.add_port_mapping("OutNet main", localAddr.c_str(), port, port, "TCP")){
        cerr << upnp.get_last_error() << endl;
        return false;
    }
    return true;
}
