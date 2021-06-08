#ifdef _WIN32
// This is a windows service implementation (see svclin.cpp for linux version)
#include "svc.h"
#include "log.h"
#include <windows.h>
#include <winsvc.h>
#include <iostream>
using namespace std;


static SERVICE_STATUS serviceStatus;
static SERVICE_STATUS_HANDLE serviceStatusHandle = 0;
static char serviceName[] = "OutNet";
static bool keepServicesRunning = true;
void (*serviceRun) () = nullptr;


bool keepRunning(){ return keepServicesRunning; }


void status(DWORD status){
    serviceStatus.dwCurrentState = status;
    SetServiceStatus(serviceStatusHandle, & serviceStatus);
}


DWORD ServiceHandler( DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext){
    switch(dwControl){
        case SERVICE_CONTROL_STOP:  // fall through
        case SERVICE_CONTROL_SHUTDOWN:
            keepServicesRunning = false;
            status(SERVICE_STOP_PENDING);
            Sleep(3); // this_thread::sleep_for(seconds(3));
            status(SERVICE_STOPPED);
            break;

        default:
            SetServiceStatus(serviceStatusHandle, &serviceStatus);
            break;
    }
    return NO_ERROR;
}


void WINAPI ServiceMain(DWORD argc, LPTSTR* argv){
    char path[MAX_PATH+1]; // +1 for null termination
    DWORD ret = GetModuleFileName(NULL, path, sizeof(path) );
    if(ret > 0){
        char* slash = strrchr(path, '\\');
        if(slash){
            *slash = 0; // need the path to the exe
            SetCurrentDirectory(path);
            log() << "CWD=" << path << endl;
        }
    } else {
        logErr() << "GetModuleFileName() failed.  Unable to change CWD." << endl;
    }

    serviceStatusHandle = RegisterServiceCtrlHandlerEx(serviceName, &ServiceHandler, NULL);
    status(SERVICE_RUNNING);
    serviceRun();
    status(SERVICE_STOPPED);
}


int registerService( void (*run)() ){
    serviceRun = run;

    serviceStatus = {
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_STOPPED,
        SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN,
        NO_ERROR,
        0,
        0,
        0,
    };

    SERVICE_TABLE_ENTRY table[] = { {serviceName, &ServiceMain }, { NULL, NULL } };
    if ( StartServiceCtrlDispatcher(table) ){
        return 0;
    }

    DWORD err = GetLastError();
    if (ERROR_SERVICE_ALREADY_RUNNING == err) {
        return -1; // caller needs to exit
    }
    if(ERROR_FAILED_SERVICE_CONTROLLER_CONNECT == err){
        logErr() << "This program is a service and should not be started from command line." << endl;
        logErr() << "Use any of the following commands to register, run, stop or delete the serivce:" << endl;
        logErr() << "sc create 'OutNet' binPath= c:\\path\\outnet.exe" << endl;
        logErr() << "sc [start/stop/delete]  'OutNet'" << endl;
        return -2; // program running as console app
    }
    if(ERROR_INVALID_DATA == err){
        logErr() << "BUG in SERIVCE_TABLE_ENTRY" << endl;
        return -3;
    }
    return -4; // what happened ???
}


#endif // #ifdef _WIN32