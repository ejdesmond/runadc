
#include <Dcm2_WinDriver.h>

// Dcm2 includes
#include <Dcm2_Logger.h>

// WinDriver includes
#include <windrvr.h>
#include <wdc_defs.h>
#include "status_strings.h"

// standard includes
#include <iostream>

using namespace std;

// using null license string for temporary license - good for 1 hour
//const string Dcm2_WinDriver::WINDRIVER_LICENSE_STRING = 
//  "6f1ea8961851699ee1cc70f754aba1807746f0cffb1ba6.WD1020_NL_University_of_Colorado";
//const string Dcm2_WinDriver::WINDRIVER_LICENSE_STRING ="";
//const string Dcm2_WinDriver::WINDRIVER_LICENSE_STRING =" 87244a4c69b49a872f4be235ae98bbf10ca726b23432ce0e83a7873fd88e.WD1221_NL_Brookhaven_National_Lab";
// new JUNGO license 03/22/2017
const string Dcm2_WinDriver::WINDRIVER_LICENSE_STRING ="87244a4c6c9542aad614ae4d4ec0a5a080e953749e4dc57023c4267c3497.WD1220_64_NL_Brookhaven_National_Lab";

Dcm2_WinDriver *Dcm2_WinDriver::_instance = 0;
bool Dcm2_WinDriver::_isOpen = false;
int Dcm2_WinDriver::opencount = 0;
bool Dcm2_WinDriver::booltest = false;
/// This method returns a pointer to the current instance.
/// If one does not exists it is created.
///
/// \return Dcm2_WinDriver pointer to the instance
///
Dcm2_WinDriver *Dcm2_WinDriver::instance() {
  if (_instance) {
    //cout << "WinDriver: using found instance of WinDriver " << endl;
    return _instance;
  }
  //cout << "WinDriver: create a new instance " << endl;
  _instance = new Dcm2_WinDriver();
  booltest = true;
  return _instance;
}

bool Dcm2_WinDriver::isOpen() {
  //cout << "WINDRIVER::isOpen called isOpen = " << _isOpen << "  ";
  if (_isOpen == true) {
    //cout << "WinDriver::isOpen driver is OPEN " << endl;
    return _isOpen;
  }
  return false;
} // isOpen


Dcm2_WinDriver::Dcm2_WinDriver() {
  //cout << "WINDRIVER DEFAULT OPEN set OPEN false " << endl;
  _isOpen = false;
  return;
}

Dcm2_WinDriver::~Dcm2_WinDriver() {
  return;
}

/// This method waits to lock Dcm2_WinDriver::_mutex and then opens the 
/// WinDriver kernel module with Jungo calls.
///
/// \todo Currently there is only one open WinDriver Object per process.
/// This can probably be altered to count the number of opens/closes
/// and so maintain a single open driver for all threads
/// until the final thread has called close, the current code isn't a 
/// usage limitation right now, but will be if multi-JSEB2s are placed 
/// in the same host as simultaneous access to the separate cards is blocked
/// by this design (though this amounts to a speed issue only).
///
/// \return true if successful
///
bool Dcm2_WinDriver::Init() {
  // lock the object
  //if (!Lock()) {
  //  return false;
  //}
  if (Dcm2_WinDriver::isOpen()) {
    //cout << "WINDRIVER:isOpen returned true " << endl;
  }
  // open a connection to the driver
  if (!Open()) {
    return false;
  }
  

  return true;
}


/// This method opens the connection to the kernel module
/// if it isn't already open.
///
/// \return true if successful
///
bool Dcm2_WinDriver::Open() {

  if (!_isOpen) {     
    //cout << "WinDriver::Open open WDC with WDC_DriverOpen opencount = " << opencount << endl;

    DWORD dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_DEFAULT, 
				    WINDRIVER_LICENSE_STRING.c_str());
    if (dwStatus != WD_STATUS_SUCCESS) {
      if (_verbosity >= FATAL) {
	Dcm2_Logger::Cout(FATAL,"Dcm2_WinDriver::Init()") 
	  << "Failed to initialize the WinDriver library with return code: " 
	  << Stat2Str(dwStatus) << Dcm2_Logger::Endl;
      } 
	  
      _isOpen = false;
      return false;
    }
    //cout << "WinDriver:Init set isOpen true " << endl;
    _isOpen = true;
  }
  opencount = opencount + 1;
  return true;
}

/// This method closes the kernel module driver but does not release
/// the mutex lock. This allows the fork() in DCM2_startRun() to 
/// take place in deterministic state.
///
/// \return true if successful
///
bool Dcm2_WinDriver::Close() {

  // keep the driver open if still accessed by multipe jseb2s
  //cout << "WinDriver::Close opencount = " << opencount << endl;
  if (opencount > 1 )
    {
      opencount = opencount -1;
      //cout << "Dcm2_WinDriver::Close opencount remains " << opencount << endl;
      return true;
    }
  //cout << "WinDriver::Close  _CLOSE_ driver  isOpen = " << _isOpen << " opencount = " << opencount << endl;
  if (_isOpen) {
    DWORD dwStatus = WDC_DriverClose();
    if (dwStatus != WD_STATUS_SUCCESS) {
      if (_verbosity >= ERROR) {
	Dcm2_Logger::Cout(ERROR,"Dcm2_WinDriver::Close()") 
	  << "Failed to close the WinDriver library with return code: " 
	  << Stat2Str(dwStatus) 
	  << Dcm2_Logger::Endl;
      }

      return false;
    }
   if (opencount == 1 ) {
      opencount = opencount -1;
      //cout << "WinDriver::Close set opencount to " << opencount << endl;
    }
   //cout << "WinDriver::Close set _isOpen = false count = " << opencount << "\n" << endl;
    _isOpen = false;
    //_instance = 0;
  }

  return true;
}

/// This method locks the mutex.
///
/// \return true if successful
///
bool Dcm2_WinDriver::Lock() {
  _mutex.lock();
  return true;
}

/// This method only releases the lock.
///
/// \return true if successful
///
bool Dcm2_WinDriver::Release() {
  _mutex.unlock();
  return true;
}

/// This method closes the kernel module
/// and then releases the lock.
///
/// \return true if successful
///
bool Dcm2_WinDriver::Exit() {

  bool status = true;
  //cout << "WinDriver::Exit called " << endl;

  // close driver
  if (!Close()) {
    status = false;
  }

  // unlock the object
  if (!Release()) {
    status = false;
  }

  return status;
}
