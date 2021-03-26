#include "config.h"
#include "sock.h" // Sock::ANY_PORT
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


struct ServiceString: public string {
    bool operator<(const Service& serv) const { return compare(serv.originalDescription) < 0; }
}; // This class provides operator<(Service&) for string.  Use it with stl algorithms


// look for *.service in current directory and load their contents into LocalData::services
int Config::loadServiceFiles(){
    static const string extension = ".service";
    vector<string> lines;
    cout << "Loading service files: ";

    // ~path() destructor crashes on some systems // path cwd = current_path();
    int fCount = 0;
    for(auto& p: directory_iterator(".") ){
        if(p.path().extension() != extension){ continue; }
        cout << p.path() << " ";
        ifstream listf (p.path());
        if( !listf ){ continue; }
        parseLines(listf, lines);
        ++fCount;
    }
    cout << "(" << fCount << " found)" << endl;
    std::sort( begin(lines), end(lines) ); // set_difference needs sorted data

    vector<string> newServices;
    vector<Service> oldServices;
    vector<pair<uint32_t,uint16_t>> portsToOpen;

    unique_lock ulock(ldata->mutx);
    sort( begin(ldata->services), end(ldata->services) ); // set_difference needs sorted data

    // find newServices(not in ldata->services) and oldServices(not in "lines")
    vector<ServiceString>& s = * reinterpret_cast<vector<ServiceString>*>( &lines ); // provides operator<(Service&)
    set_difference( begin(s), end(s), begin(ldata->services), end(ldata->services), back_inserter(newServices));
    set_difference( begin(ldata->services), end(ldata->services), begin(s), end(s), back_inserter(oldServices));

    for(Service& serv: oldServices){ // delete old services
        ldata->services.erase( remove( begin(ldata->services), end(ldata->services), serv ) );
    } // TODO: if services are allowed to register with OutNet service directly, this has to change!

    // TODO: config should keep a list of ports forwarded on a router with their lease times
    // extend the lease times every hour if a service accepts a TCP connection 
    // or swallows a 0 size UDP packet

    // insert new services and prepare to open ports for them
    for(const string& serv: newServices){
        auto s = ldata->addService(serv);
        if(!s) { continue; } // did it parse correctly?
        portsToOpen.emplace_back(s->ip, s->port);
    }
    ulock.unlock(); // after unlocking we can open and close ports which can take a while

    for(auto& ipPort: portsToOpen){  // open router ports using UPnP protocol for new services
        upnp.openPort(ipPort.first, ipPort.second);
    }

    for(auto& old: oldServices){ // close ports for old services
        upnp.closePort(old.ip, old.port);
    }
    return 0;
}


int Config::loadBlackListFiles(){ return 0; }


// a member of type "const std::string" cannot have an in-class initializerC/C++(1591)
static const string configName = "settings.cfg";

// load port and refresh rate from config file
int Config::loadFromDisk(LocalData& lData, BlackList& bList){
    cout << "Loading configuration data." << endl;
    ldata = &lData;
    blist = &bList;

    loadServiceFiles();
    loadBlackListFiles();

    ldata->myPort = Sock::ANY_PORT; // default
    vector<string> lines;
    ifstream config (configName);
    if( !config ){
        cout << configName << " not found. Setting configuration data to defaults." << endl;
        return saveToDisk();
    }

    parseLines(config, lines);
    config.close();
    bool foundPort = false;

    for(string& line: lines){
        string key, value;
        if( keyValue(line, key, value)){
            toLower(key);
            if( key == "configrefresh" ){
                refreshRate = strtol(value.c_str(), nullptr, 10); // base 10
            } else if (key == "serverport"){
                ldata->myPort = strtol(value.c_str(), nullptr, 10); // base 10
                foundPort = true;
            }
        }
    }
    if( !foundPort) { saveToDisk(); } // config is corrupted
    cout << "Configuration data loaded successfuly." << endl;
    return 0;
}


int Config::saveToDisk(){
    cout << "Saving configuration data to disk." << endl;
    string msg = "# If this config file is deleted or corrupted it will be regenerated.";
    ofstream config(configName);
    config << msg << endl;
    config << "ConfigRefresh=" << refreshRate << " # how often do I look for new or updated files (seconds)" <<  endl;
    shared_lock slock(ldata->mutx);
    config << "ServerPort=" << ldata->myPort << " # server will accept connections on this port" << endl;
    return 0;
}
