#ifndef INCLUDED_SVC_H
#define INCLUDED_SVC_H
// interface functions for registering (creating) a service
// implementation is OS dependent
// linux implementation is in svclin.cpp
// windows implementation is in svcwin.cpp


int registerService( void (*run) () ); // make this program run as a service
bool keepRunning(); // if this returns false, exit all service threads


#endif // INCLUDED_SVC_H