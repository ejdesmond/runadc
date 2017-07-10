/**
 *
 * runadc 
 * configuration options.
 * reads a config file with the names of dcm2, part #, dcm2 configfile, dcm2 jseb , adc jseb, part jseb
 * reads adc config file with runtime parameters
 * uses ADC object in place of Dcm2_ADC. All functions start and stop WinDriver
 * NOTE: modified for va096 with no JSEB2 for xmit readout
 * uses JSEB2.10.0 port 1 to Dcm2Conroller
 * uses JSEB2.10.0 port 2 to ADC controller
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <netinet/in.h>
#include "dcm2Impl.h"
#include <string>
#include <vector>
#include <strings.h>
#include <string.h>
#include "adcj.h"
#include "Dcm2_JSEB2.h"
#include <Dcm2_WinDriver.h>
#include <Dcm2_FileBuffer.h>
#include <Dcm2_Controller.h>
#include <Dcm2_Board.h>
#include <Dcm2_Partitioner.h>
#include <Dcm2_Partition.h>
#include <pthread.h>
#include <jseb2Controller.h>

using namespace std;



string outputFileDir = "/home/phnxrc/desmond/daq/prdf_output/";
string obeventDataFileName = outputFileDir + "obeventdata.prdf";
string outputDataFileName = "adcoutputdata.prdf";

dcm2Impl *pdcm2Object;
string dcm2ObjectName;
string dcm2Jseb2Name; // dcm2 controller jseb2
string adcJseb2Name;  // adc controller jseb2
string partreadoutJseb2Name; // partition readout jseb2

string jseb2OutputDataFile = "adc2part.prdf";
 

string cmdpartitionname = "DCMGROUP.TEST.1";
 string dcm2configfilename = "/home/phnxrc/desmond/daq/build/dcm2DigitizerTest.dat";
jseb2Controller * pjseb2c;
char hostname[64];
char * adcserverhost;
bool jseb2Controller::is_busy = false; // set by ADC when data is ready and waiting to be read from jseb2
bool jseb2Controller::is_data_ready = false; // set by ADC when data is available to be read by jseb2
bool jseb2Controller::forcexit = false;

bool debug_commands = true;
bool enable_part_readout = false;
bool enable_ext_readoutdelay = false;

extern bool exitRequested;
extern  int *exitRunRequested;
/**
 * getDeviceNames
 * determine which configuration file to read based on the host name
 * The configuration file contains the names of the objects to create.
 * Create the device objects for each name in the configuration file.
 *     config objects file format:
 *	    dcm name	   part# port#  dcm2 jseb2  adc jseb2   part jseb2
 *	    DCMGROUP.TEST.1 0     5001  JSEB2.10.0 JSEB2.12.0 JSEB2.14.0
 */
void getDevice2Names()
{
  // set config file name based on host
  char dcm2ConfigFileName[128];

 
 
  char adcConfigFilename[64];


 if (!strcmp(adcserverhost,"va097") || !strcmp(adcserverhost,"va096") )
	{
	  sprintf(adcConfigFilename,"%s","/home/phnxrc/desmond/daq/build/sphenixtest_adc_config");
	  //if (enable_ext_readoutdelay == false) {
	  if (enable_part_readout == false) {
	    dcm2configfilename = "/home/phnxrc/desmond/daq/build/dcm2ADCTest96.dat";
	  } else {
	    dcm2configfilename = "/home/phnxrc/desmond/daq/build/dcm2ADCPar.dat";
	  }
	  // original jseb2 assignments
	  //dcm2Jseb2Name = "JSEB2.12.0"; // dcm2 controller jseb2 on va095
	  //adcJseb2Name  = "JSEB2.10.0";  // adc controller jseb2  // adc control on port 2
	  //partreadoutJseb2Name = "JSEB2.8.0";
	  // jseb2 assignments after moving partitioner to JSEB2.12
	  dcm2Jseb2Name = "JSEB2.10.0"; // dcm2 controller jseb2 on va095
	  adcJseb2Name  = "JSEB2.10.0";  // adc controller jseb2  // adc control on port 2
	  partreadoutJseb2Name = "JSEB2.12.0";

	 }
 if (!strcmp(adcserverhost,"va095"))
   {
	  sprintf(adcConfigFilename,"%s","/home/phnxrc/desmond/daq/build/sphenixtest_adc_config");
	  if (enable_part_readout == false) {
	    dcm2configfilename = "/home/phnxrc/desmond/daq/build/dcm2ADCTest95.dat";
	  } else {
	    dcm2configfilename = "/home/phnxrc/desmond/daq/build/dcm2ADCPar.dat";
	  }
	  dcm2Jseb2Name = "JSEB2.5.0"; // dcm2 controller jseb2 on va095
	  adcJseb2Name  = "JSEB2.5.0";  // adc controller jseb2  // adc control on port 2
	  partreadoutJseb2Name = "JSEB2.8.0";
	  
   }

  // open config file
  // here read the adc_config file to get the dcm and jseb names to use
  // read dcm name, dcm2jseb2, adcjseb2, partreadoutjseb2
  // JSEB2 assignments are valid for va095 -has 3 JSEB2s installed
  dcm2ObjectName = "DCMGROUP.TEST.1";


  // NOTE: on va095 with 3 jseb2s installed the ids of the jseb2 are
  // JSEB2.5.0 , JSEB2.6.0 , JSEB2.8.0

  

} // getJseb2Names


/* Subtract the ‘struct timeval’ values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0. */

int
timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}



void _Exit(int command)
{
  pjseb2c->Exit(command);
  
} // _Exit

int main( int argc, char *argv[] ) {

  // copied from adctest
 int verbosity = Dcm2_BaseObject::DEBUG;
  unsigned long nevents = 100;
  UINT32 numberofEvents = 100;
  // default - use xmit dcm2 adc 100 events through dcm2
  bool enable_adc = true;
  bool enable_dcm2 = true;

 
  bool enable_xmit = true;
 
  int istart = 0;
  int l1delay = 48;
  int samplesize = 12;
  int numadcboards = 1;
  int firstadcslotnumber = 16;
  int numxmitgroups = 1;
  pthread_t dcm2_tid = 0;
  struct timeval tvstarttime,tvendtime,tvdifftime;
  char * cstarttime;
  char * cendtime;


 // get names of devices to use. Then search list of found jseb2 objects and create
  // a jseb2 object for each one

  gethostname((char *)hostname,64);
  adcserverhost = strtok(hostname,"."); // get host name

 // get command arguments

 // scan for arguments
 if (argc > 1)
   for (int i = 1; i < argc; i++ ) {
     if (strcmp(argv[i], "-n") == 0) {
       nevents = atoi(argv[i+1]);
       numberofEvents = atoi(argv[i+1]);
     }
     if (strcmp(argv[i], "-b") == 0) {
       numadcboards = atoi(argv[i+1]);
     }
    if (strcmp(argv[i], "-s") == 0) {
       firstadcslotnumber = atoi(argv[i+1]);
     }    
    if (strcmp(argv[i], "-m") == 0) {
       samplesize = atoi(argv[i+1]);
     }  
    if (strcmp(argv[i], "-p") == 0) {
      // only enable local partition readout on va095
      if (!strcmp(adcserverhost,"va095")) {
       enable_part_readout = true;
      }
      if (!strcmp(adcserverhost,"va096")) {
	// enable manual delay if reading partitioner from a different computer
	//enable_ext_readoutdelay = true; // wait for remote jseb2 startup;
	enable_part_readout = true;
      }
     }
     if (strcmp(argv[i], "-f") == 0) {
       
       if (argv[i+1] != NULL)
	 outputDataFileName =  argv[i+1]; // write data file to current directory
     }
     if (strcmp(argv[i], "-of") == 0) {
       // TODO get file name to write to
       if (argv[i+1] != NULL)
       obeventDataFileName = outputFileDir + argv[i+1];
     }
   if (strcmp(argv[i], "-d") == 0) {
      l1delay = atoi(argv[i+1]);
     }
     if (strcmp(argv[i], "-h") == 0) {
       cout << "Usage: runadc -n nevents (default = 100 ) -d l1delay -s adcslotnumber -b num adc boards -m samplesize -f datafile -p enable partitioner readout -h print help" << endl;
       return 0;
     }
   } // got arguments

 
  getDevice2Names();

  // here to the search for the jseb2 ; now assign the index 
 
  int partitionNb = 0;
 

  
  // create dcm2 control object
  string dcm2name = "DCMGROUP.TEST.1";

  dcm2Impl dcm2control = dcm2Impl(dcm2name.c_str(),partitionNb );

  //
  // adc needs number of loops, number of events, 1st module slot number, number of FEM modules
 
   
 
 
  ADC * padc;


 

  

    padc = new ADC();

    padc->setNumAdcBoards(numadcboards);
    padc->setFirstAdcSlot(firstadcslotnumber);
   // tell adc controller which jseb2 to use
    padc->setJseb2Name(adcJseb2Name);
    padc->setL1Delay(l1delay);
    padc->setSampleSize(samplesize);
    padc->setObDatafileName(obeventDataFileName);


  unsigned long cmdstatus;
  int startadcflag = 0;
  int stopadcflag  = 0;
 


  
 

    if (enable_dcm2)
    {
      //cout << "Download dcm2 file " << dcm2configfilename.c_str() << endl;
      pdcm2Object = new dcm2Impl(dcm2name.c_str(),partitionNb );
      //cout << "Type 1 to download the  DCM2 " << endl;
      //cin >> startadcflag;
      // set up process memory map
      pdcm2Object->setExitMap();
      // configure (download) the dcm2
      pdcm2Object->download(cmdpartitionname,dcm2configfilename,partitionNb);
      cout << "Download done " << endl;
    }

    //enable_part_readout = false;
    //enable_ext_readoutdelay = true;
  bool receivedTrigger = false;
  bool startadc = false;
  if (enable_ext_readoutdelay == true)
    {
      cout << "START JSEB2 READER " << endl;
      cin >> startadcflag;
    }
  
   // readout of dcm2 controller via jseb2
   if (enable_part_readout == true)
     {
       
       cout << "Write " << nevents << " events to file " << outputDataFileName.c_str() << endl;
       
       pjseb2c = new jseb2Controller();
       pjseb2c->Init(partreadoutJseb2Name);
       pjseb2c->setNumberofEvents(nevents);
       pjseb2c->setOutputFilename(outputDataFileName);
       //cout << " -- jseb2Controller: start_read2file ... " << endl;
       
       pjseb2c->start_read2file(); // start read2file  thread
       sleep(3);
     }




  if (enable_adc)
    {
      
      
	
	if (padc->initialize(numberofEvents) == false ) {
	  cout << "Failed to initialize xmit - try restart windriver " << endl;
	  return 0;
	
      }
      // start run in thread
      
      if (enable_dcm2)
	{
	  cout << "startrun on dcm2 " << endl;
	  cout << "dcm2 write data to file " << outputDataFileName.c_str() << endl;
   
	  //printf("\nType 1 to start dcm2 data collection \n");
	  //scanf("%d",&istart);
	  
	  dcm2_tid = pdcm2Object->startRunThread(cmdpartitionname,numberofEvents,outputDataFileName.c_str());
	  if (dcm2_tid == 0 )
	    {
	      cout << "DCM2 Failed to start - Exiting " << endl;
	      return 0;
	    }
	  
	}
      
  
      sleep(2); // wait for dcm2 to start

      // start adc - executes busyreset
	//padc->setup_xmit();
	//padc->start_oevent(); // padc->start_xmit();
	//padc->teststart(); // only executes busy reset
	
      
      // 02/15/2017 try moving start dcm2 after adc start so only one open of WinDriver occurs at a time.
      // start run in thread
      /*
      if (enable_dcm2)
	{
	  cout << "startrun on dcm2 " << endl;
	  cout << "dcm2 write data to file " << outputDataFileName.c_str() << endl;
   
	  //printf("\nType 1 to start dcm2 data collection \n");
	  //scanf("%d",&istart);
	  pdcm2Object->startRunThread(cmdpartitionname,numberofEvents,outputDataFileName.c_str());
	}
	//cout << "menuadctest start " << endl;
       */
    } else {
    // adc not enabled
    if (enable_dcm2)
    {
	  cout << "startrun on dcm2 " << endl;
	  cout << "dcm2 write data to file " << outputDataFileName.c_str() << endl;
   
	  printf("\nType 1 to start dcm2 data collection \n");
	  scanf("%d",&istart);
        dcm2_tid = pdcm2Object->startRunThread(cmdpartitionname,numberofEvents,outputDataFileName.c_str());
	
    }
  }

  // loopback test of jseb2
  //cout << "start LOOPBACK " << endl;
  //padc->jseb2_word_loop(dcm2Jseb2Name);

  // starttime
  gettimeofday(&tvstarttime,NULL);
 
 
   
   void *holder;
  // wait for run completion
   if (enable_dcm2 )
     {
       //cout << "runadc Type ^C to exit " << endl;
       //cout << "runadc thread join wait on tid "<< dcm2_tid  << endl;
       //pthread_join(dcm2_tid,&holder);
 
       // test if event completion status should be used to continue
       if (nevents > 0 ) 
	 {      
       cout << "runadc: Waiting for data completion " << endl;
       while (jseb2Controller::runcompleted == false && (*exitRunRequested == 0) )
	 {
	   sleep(2);
	   //cout << "WAITING FOR DATA " << endl;
	   if (exitRequested) {
	     cout << "RECEIVED exitRequested " << endl;
	     break;
	   }
	 } //while
 	} // nevents test      
     } // enable_dcm2
  
   // endtime
      gettimeofday(&tvendtime,NULL);
      timeval_subtract(&tvdifftime,&tvendtime,&tvstarttime);
      cout << "Elap time sec = " << tvdifftime.tv_sec << " usec = " << tvdifftime.tv_usec << endl;
      if (nevents > 0 ) 
	{
	  float eventrate = 0.0;
	  eventrate = nevents/((tvdifftime.tv_sec * 1000000) + tvdifftime.tv_usec);
	  //cout << "Event Rate events/usec " << eventrate << endl;
	}
   // use manual exit for infinite event count to end run
   if (nevents == 0 )
     {
       cout << "Enter 1 to end run" << endl;
       cin >> stopadcflag;
     }
  
 
  if (enable_adc) {
    //cout << "ADC done... " << endl;
    delete padc;
  }

  verbosity = Dcm2_BaseObject::LOG;
  if (enable_dcm2)
    {
      
      pdcm2Object->stopRun(cmdpartitionname);
    }
  if (enable_part_readout == true)
    {
      jseb2Controller::forcexit = true;
      pjseb2c->setExitRequest(true);
      sleep(2); // wait for data to be written
      pjseb2c->Close();
    }
} // end main
