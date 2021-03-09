#include "config.h"
#include "sock.h" // Sock::ANY_PORT
#include "utils.h"
#include <thread>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
using namespace std;
using namespace std::filesystem; // namespace fs = std::filesystem;


int Config::loadServiceFiles(){
    static const string extension = ".service";
    vector<string> lines;

    // ~path() destructor crashes on some systems // path cwd = current_path();
    for(auto& p: directory_iterator(".") ){
        if(p.path().extension() == extension){
            cout << "found file " << p.path() << endl;
            ifstream listf (p.path());
            if( !listf ){ continue; }
            parseLines(listf, lines);
        }
    }

    vector<Service*> portsToOpen;
    unique_lock ulock(ldata->mutx); // lock LocalData before modifying it
// TODO: completely replace all services with new ones loaded from files
    for(auto& s: lines){
        bool newService = true;
        for(Service& ls: ldata->services){
            if( s == ls.originalDescription ) {
                newService = false;
                continue;
            }
        }
        if(newService){
            Service* serv = ldata->addService(s); // new service
            if(serv){ // if it is not null, it was successfuly parsed by Service class
                portsToOpen.push_back(serv);
            }
        }        
    }
    ulock.unlock();

    for(Service* s: portsToOpen){ // open ports for those services after unlocking ldata
        openPort(s->ip, s->port);
    }

    return 0;
}

int Config::loadBlackListFiles(){ return 0; }


// a member of type "const std::string" cannot have an in-class initializerC/C++(1591)
static const string configName = "settings.cfg";

// load port and refresh rate from config file
int Config::loadFromDisk(LocalData& lData, BWLists& bwLists){
    ldata = &lData;
    bwlists = &bwLists;

    loadServiceFiles();
    loadBlackListFiles();

    ldata->myPort = Sock::ANY_PORT; // default
    vector<string> lines;
    ifstream config (configName);
    if( !config ){ return saveToDisk(); }

    parseLines(config, lines);
    config.close();
    bool foundPort = false;

    for(string& line: lines){
        string key, value;
        if( keyValue(line, key, value)){
            toLower(key);
            if( key == "configrefresh" ){
                refreshRate = atoi( value.c_str() );
            } else if (key == "serverport"){
                ldata->myPort = atoi( value.c_str() );
                foundPort = true;
            }
        }
    }
    if( !foundPort) { saveToDisk(); } // config is corrupted
    return 0;
}


int Config::saveToDisk(){
    string msg = "# If this config file is deleted or corrupted it will be regenerated.";
    ofstream config(configName);
    config << msg << endl;
    config << "ConfigRefresh=" << refreshRate << " # how often do I look for new or updated files (seconds)" <<  endl;
    shared_lock slock(ldata->mutx);
    config << "ServerPort=" << ldata->myPort << " # server will accept connections on this port" << endl;
    return 0;
}


// ServiceWatch looks for files in the data directory (specified by config)  of the following types:
// *.blacklist - is a black list of IP:port pairs, public keys or protocols
// *.whitelist - overrides blacklist entries and stores keys of interest
// *.service - is a single or multiple service description (one per line)
// *.service file can contain remote service urls which can be used to bootstrap the network.

int Config::watch() {
    while(true){
        this_thread::sleep_for( seconds(refreshRate) );
        loadServiceFiles();
        loadBlackListFiles();
    }
}
