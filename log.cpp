#include "log.h"
#include <ctime>
#include <fstream>
#include <sstream>
#include <iomanip> // std::hex, std::dec
#include <chrono>
using namespace std::chrono;
using namespace std;
// TODO: make logging code thread safe

static ofstream devNull;
static const int LINES_PER_FILE = 1000000;
static LOG_LEVEL logLevel = LOG_LEVEL::INFO;
void setLogLevel(LOG_LEVEL level){ logLevel = level; }


void logTime(ostream& log){ // THE HORROR!  When are we getting operator<<(time_point&) ???
    auto t = system_clock::now();
    auto ms = duration_cast<milliseconds>(t.time_since_epoch()) % 1000;
    time_t tt = system_clock::to_time_t(t);
    tm *tmp = localtime(&tt);

    log << tmp->tm_hour << setfill('0');
    log << setw(2) << tmp->tm_min;
    log << setw(2) << tmp->tm_sec << '.';
    log << setw(3) << ms.count() << setw(0);
}


bool createLogFile(ofstream& log){
    time_t now = time(0);
    tm *tmp = localtime(&now);

    stringstream ss;
    ss << setfill('0') << 1900+tmp->tm_year << setw(2) << tmp->tm_mon << setw(2) << tmp->tm_mday;
    string date = ss.str();
    ss << "_" << setw(2) << tmp->tm_hour << setw(2) << tmp->tm_min << setw(2) << tmp->tm_sec;

    string fname = "outnet"+ss.str()+".log";
    log.close();
    log.open(fname, ios_base::out | ios_base::app); // append
    log << "LOG START date: " << date << endl;

    return log.good();
}


ostream& logToFile(const string& logType){
    static ofstream log;
    static int lineCount = 0;
    if( --lineCount <=0 ){
        lineCount = LINES_PER_FILE;
        createLogFile(log);
    }

    logTime(log);
    log << " " << logType;
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
