#ifndef _WIN32
// This is a linux service implementation (see svcwin.cpp for windows version)
#include "svc.h"
#include "log.h"
#include <unistd.h>
#include <errno.h>
#include <signal.h>


static bool keepServicesRunning = true;


bool keepRunning(){ return keepServicesRunning; }


void handleSigterm(int sigNum){
    if(SIGTERM == sigNum){
        keepServicesRunning = false; 
    }
}


int registerService( void (*run) () ){
    signal(SIGTERM, &handleSigterm);

    if( daemon(true, false) ){ // Do not change CWD.  Close stdin/stdout/stderr
        int err = errno;
        logErr() << "daemon(true, false) failed with error code " << err << std::endl;
        return -1;
    }
    run();
    return 0;
}


#endif // #ifndef _WIN32
