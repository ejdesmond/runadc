#ifndef JSEB2CONTROLLER_H
#define JSEB2CONTROLLER_H

#include <Dcm2_WinDriver.h>
#include <Dcm2_FileBuffer.h>
#include <Dcm2_Controller.h>
#include <Dcm2_JSEB2.h>
#include <Dcm2_Board.h>
#include <Dcm2_Partitioner.h>
#include <Dcm2_Partition.h>
#include <Dcm2_BaseObject.h>
// WinDriver includes
#include "wdc_defs.h"
#include "wdc_lib.h"
#include "plx_lib.h"
#include "utils.h"
#include "status_strings.h"
#include "samples/shared/diag_lib.h"
#include "samples/shared/wdc_diag_lib.h"
// new for windriver 122
#include "samples/shared/pci_diag_lib.h"
// #include "samples/shared/pci_regs.h"

// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <string>
#include <strings.h>
#include <string.h>
#define MAXJSEBS 4

using namespace std;

class jseb2Controller : public Dcm2_BaseObject{

 public:
  jseb2Controller();
  ~jseb2Controller();
  bool Init(string jseb2name);
  void setJseb2Name(string jseb2name);
  void setNumberofEvents(unsigned long   numberofevents);
  void setOutputFilename(string outputfilename);
  void jseb2_seb_read2file(); 
  void enableWrite2File();
  void disableWrite2File();
  void setExitRequest(bool exitrequest){ exitRequested = exitrequest; }
  void jseb2_seb_reader(Dcm2_JSEB2 *jseb2, bool printWords, bool printFrames, bool writeFrames);
  void jseb2_seb_reader2(Dcm2_JSEB2 *jseb2, bool printWords, bool printFrames, bool writeFrames);
  void Close(); // close windriver
  void Exit(int command);
  //void jseb2_seb_read2file(Dcm2_JSEB2 *jseb2); // maybe no arg if internal jseb2
  void start_read2file(); // creates thread with read2FileThread which calls jseb2_seb_reader
  static void *read2FileThread(void * arg);
  static bool runcompleted;
static bool is_busy; // set by ADC when data is ready and waiting to be read from jseb2
static bool is_data_ready; // set by ADC when data is available to be read by jseb2
static bool forcexit;
 private:

  Dcm2_WinDriver *windriver;
 bool writeFrames;
 bool exitRequested;
 Dcm2_Controller *controller;
 Dcm2_JSEB2 *jseb2;
 Dcm2_JSEB2 *jseb2list[MAXJSEBS];
 char * jseb2namelist[MAXJSEBS];
 string _outputfilename;
 char outputfilename[500];
 string _jseb2name;
 int verbosity;
 unsigned long nevents;
};

#endif
