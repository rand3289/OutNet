#include "log.h"
#include <ctime>
#include <fstream>
#include <sstream>
#include <iomanip> // std::hex, std::dec
using namespace std;
// TODO: make this code thread safe

static ofstream devNull;
static const int LINES_PER_FILE = 1000000;
static LOG_LEVEL logLevel = LOG_LEVEL::INFO;
void setLogLevel(LOG_LEVEL level){ logLevel = level; }


string logTime(){
    return ""; // TODO:
}


bool createLogFile(ofstream& log){
    time_t now = time(0);
    tm *tmp = localtime(&now);

    stringstream ss;
    ss << 1900+tmp->tm_year << std::setw(2) << std::setfill('0') << tmp->tm_mon << std::setw(2) << tmp->tm_mday;
    string date = ss.str();

    string fname = "outnet"+date+".log";
    log.close();
    log.open(fname, ios_base::out | ios_base::app); // append
    log << "LOG date: " << date << " " << logTime() << endl;

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
    return logToFile("");
}


ostream& logErr(){
    if( logLevel < LOG_LEVEL::LERR ){ return devNull; }
    return logToFile("ERR ");
}


ostream& logInf() {
    if( logLevel < LOG_LEVEL::INFO ){ return devNull; }
    return logToFile("INF ");
}
