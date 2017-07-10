#ifndef DCM2IMPL_H
#define DCM2IMPL_H
#include <stdlib.h>
#include <unistd.h>
#include <map>
#include <unistd.h>
#include <pthread.h>
#define DCMIFNSIZE 256
#define DCMIREADCONFIG 0x1
#define DCMIINIT 0x2
#define DCMISTARTREADY 0x4
#define DCMISTARTRUN 0x8
#define DCMISTOPRUN 0x10
#define DCMIL1DD 0x20
#define DCMIPARTID 0x40
#define DCMIMASK 0x7F
#define DCMISHIFT 0x4

#define NUMGL1PSCALERS   512
#define NUMCROSSCOUNTERS 128
#define DCMIUFNSIZE 256
#define DCMIUMAXNBPARTNERS 5

extern char DCMIUFileName[DCMIUMAXNBPARTNERS][DCMIUFNSIZE];
extern int DCMIUStartRunStatus[DCMIUMAXNBPARTNERS];
#define WITH_THREADS
using namespace std;

class dcm2Impl
{

 public:

  dcm2Impl(const char *dcmInterName, long PartitionerNb);
  ~dcm2Impl();
  int init();
  int startReady(string partitionname);
  int startRun(string partitionname,int NbOfEvents,const char *DataFilename);
  int stopRun(string partitionname);
  int checkReady(string partitionname);
  int download(string partitionname,string configfilename,int partitionNb); // execute readConfig
  int getEventsProcessed(string configfilename,int fiber);
  int cleanup(string configfilename);
  int disable(string configfilename);
  int count(string configfilename);
  void setEventCount(string partitionname,string eventcount);
  void raiseBusy(string partitionname);
  void releaseBusy(string partitionname);
  void raiseHold(string partitionname);
  void releaseHold(string partitionname);
  string getDeviceName();
  void SetDataFileName(const char * outputDataFileName);
  void SetPartitionname(string partitionname);
  pthread_t  startRunThread(string partitionname,int NbOfEvents,const char * outputDataFileName);
  char * getOutputFile();
  int getNumberOfEvents();
  string getPartitionName();
  int setExitMap();
 
#ifdef WITH_THREADS
  static void *RunThread(void * arg);
  
#endif
  /**
   * runStatus
   * returns run state of dcm 
   * run states are initialized, running, stopped, error
   */
  int runStatus();
 private:
  long  status;
  int verbosity;
  //string dcmInterName;
  int partitionerNumb;
  int PartitionerNb;
  string partitionername;
  char DCMIUFileName[DCMIUMAXNBPARTNERS][DCMIUFNSIZE];
  char dcmInterName[64];  // object name
  char partitionName[64]; // partition name
  long dcmInterStatus;
  unsigned long dcmInterFailureMode;
  unsigned long dcmInterError;
  unsigned long cross_counter[NUMCROSSCOUNTERS];
  unsigned long eventCount;
  int dcm_debug;
  map<int,string> partnameidmap;
  map<int,string>::iterator partmapiter;
  static const int TERMINAL = 2;
  static const int DEBUG = 3;
  static const int LOG = 0;
  pthread_t tid;
  string _partitionname;
  int    _NbOfEvents;
  char  _DataFilename[500];
};
#endif
