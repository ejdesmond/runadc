#ifndef __DCM2_WINDRIVER_H__
#define __DCM2_WINDRIVER_H__

//==============================================
/// \file Dcm2_WinDriver.h
/// \brief WinDriver kernel module object
/// \author Michael P. McCumber
//==============================================

// Dcm2 includes
#include <Dcm2_BaseObject.h>

// standard includes
#include <string>

// boost includes
#include <boost/thread/mutex.hpp>

/// \class Dcm2_WinDriver
///
/// \brief WinDriver kernel module object
///
/// This object imports license strings and activates
/// the kernel module. Only one of these should be open in
/// process and is thus a singleton object. A mutex
/// limits the opening of the kernel module to a
/// single process thread. Other threads wait for
/// access. This was useful to get things working
/// in Run11, but this object will probably need
/// an update to have all threads use the same
/// opening of the kernel module. Right now, a
/// controller with several JSEB2s controlling
/// multiple crates will be slower than neccessary
/// on downloads. Update: I tested a version of this
/// object that shares an open kernel module connection
/// and that works, there is no need to keep the connection
/// locked for the entire use, only open/close operations.
/// However this greatly complicates the environment during the
/// fork to readback data over the backplane... so I am not
/// going to implement that design (downloads should be fast enough
/// with a small stack).
///
/// \todo Update this object to restrict access to only open/close
/// for a multi-JSEB2 stack then retweak timeouts
///
class Dcm2_WinDriver : public Dcm2_BaseObject {

 public:

  /// get the object instance
  static Dcm2_WinDriver *instance();
/// keeps track of whether the driver is already open
  //static bool _isOpen;

  static bool isOpen();

  virtual ~Dcm2_WinDriver();

  /// both locks the mutex and opens the kernel module
  bool Init();
  /// both closes the kernel module and unlocks the mutex
  bool Exit();

  /// open the kernel module
  bool Open();
  /// closes the kernel module (allows fork while locked and closed)
  bool Close();

  /// locks the mutex
  bool Lock();
  /// unlocks the mutex
  bool Release();

 private:

  /// private constructor for singleton implementation
  Dcm2_WinDriver();
  /// storage for the object instance
  static Dcm2_WinDriver *_instance; 

  // allows multiple access to open driver so multiple jseb2 can be used
  static int opencount;
  static bool booltest;
  /// mutex lock to stop simultaneous kernel module access
  boost::mutex _mutex; 

/// keeps track of whether the driver is already open
  static bool _isOpen;

  /// keeps track of whether the driver is already open
  //bool _isOpen; 

  /// jungo license string (can not be published publicly)
  static const std::string WINDRIVER_LICENSE_STRING;

};

#endif // __DCM2_WINDRIVER_H__
