#ifdef _WIN32
// This is a windows service implementation
#include <windows.h>
#include <winsvc.h>
#include <iostream>
using namespace std;


extern bool globalStopAll; // defined in main.cpp
extern void run();         // defined in main.cpp


static SERVICE_STATUS serviceStatus;
static SERVICE_STATUS_HANDLE serviceStatusHandle = 0;
static char serviceName[] = "OutNet";


void status(DWORD status){
    serviceStatus.dwCurrentState = status;
    SetServiceStatus(serviceStatusHandle, & serviceStatus);
}


DWORD ServiceHandler( DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext){
    switch(dwControl){
        case SERVICE_CONTROL_STOP:  // fall through
        case SERVICE_CONTROL_SHUTDOWN:
            globalStopAll = true;
            status(SERVICE_STOP_PENDING);
            Sleep(3);
            status(SERVICE_STOPPED);
            break;

        default:
            SetServiceStatus(serviceStatusHandle, &serviceStatus);
            break;
    }
    return NO_ERROR;
}


void WINAPI ServiceMain(DWORD argc, LPTSTR* argv){
    serviceStatusHandle = RegisterServiceCtrlHandlerEx(serviceName, &ServiceHandler, NULL);
    status(SERVICE_RUNNING);
    run();
    status(SERVICE_STOPPED);
}


int registerAsWindowsService(){
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
        cerr << "This program is a service and should not be started from command line." << endl;
        cerr << "Use any of the following commands to register, run, stop or delete the serivce:" << endl;
        cerr << "sc create 'OutNet' binPath= c:\\path\\outnet.exe" << endl;
        cerr << "sc [start/stop/delete]  'OutNet'" << endl;
        return -2; // program running as console app
    }
    if(ERROR_INVALID_DATA == err){
        cerr << "BUG in SERIVCE_TABLE_ENTRY" << endl;
        return -3;
    }
    return -4; // what happened ???
}


#endif // #ifdef _WIN32