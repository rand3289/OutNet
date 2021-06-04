#ifndef INCLUDED_LOG_H
#define INCLUDED_LOG_H
#include <ostream>


enum LOG_LEVEL { NONE=0, LERR=1, LOG=2, INFO=3 };
void setLogLevel(LOG_LEVEL level);

std::ostream& log();
std::ostream& logErr();
std::ostream& logInf();


#endif // INCLUDED_LOG_H