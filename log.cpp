#include "log.h"
#include <ctime>
#include <fstream>
using namespace std;


static ofstream devNull;
static const int LINES_PER_FILE = 1000000;
static LOG_LEVEL logLevel;
void setLogLevel(LOG_LEVEL level){ logLevel = level; }


string logTime(){
    return ""; // TODO:
}


bool createLogFile(ofstream& log){
    time_t now = time(0);
    tm *tmp = localtime(&now);
    string date = to_string( 1900 + tmp->tm_year) + to_string(tmp->tm_mon) + to_string(tmp->tm_mday);

    string fname = "outnet"+date+".log";
    log.close();
    log.open(fname);
    log << date << " " << logTime() << endl;

    return log.good();
}


ostream& logToFile(const string& logType){
    static ofstream log;
    static int lineCount = 0;
    if( --lineCount <=0 ){
        lineCount = LINES_PER_FILE;
        createLogFile(log);
    }
    log << logTime() << " " << logType;
    return log;
}


ostream& log() {
    if( logLevel < LOG_LEVEL::LOG ){ return devNull; }
    return logToFile("ERR");
}


ostream& logErr(){
    if( logLevel < LOG_LEVEL::ERR ){ return devNull; }
    return logToFile("LOG");
}


ostream& logInf() {
    if( logLevel < LOG_LEVEL::INFO ){ return devNull; }
    return logToFile("INF");
}
