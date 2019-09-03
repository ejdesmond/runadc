/**
 *
 * runmxadc
 * configures and runs daq with multiple xmit groups
 * reads a configuration file with location and configuration of the adc boards
 * reads a config file with the names of dcm2, part #, dcm2 configfile, dcm2 jseb , adc jseb, part jseb
 * reads adc config file with runtime parameters
 * uses ADC object in place of Dcm2_ADC. All functions start and stop WinDriver
 * NOTE: modified for va095 with no JSEB2 for xmit readout
 * uses JSEB2.5.0 port 1 to Dcm2Conroller
 * uses JSEB2.5.0 port 2 to ADC controller
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include "dcm2Impl.h"
#include <string>
#include <vector>
#include <strings.h>
#include <string.h>
#include "adcjmultix.h"
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

 struct xmit_group_modules {
    int slot_nr;
    int nr_modules;
    int l1_delay;
    int n_samples;
  };
struct xmit_group_modules xmit_groups[3];

  int _slot_nr;
  int _nr_modules;
  int _l1_delay;
  int _nevents;
  int _n_samples;
  int _num_xmitgroups;
  string _dcm2jseb2name;
  string _adcjseb2name;
  string _partreadoutJseb2Name;
  
  string configfiledir = "/export/software/online_configuration/";
  string _dcm2name;
  string adcconfigfilename; // configuration file for adc boards
  string dcm2name = "DCMGROUP.TEST.1";
  string cmdpartitionname = "DCMGROUP.TEST.1";
 string dcm2configfilename = "/home/phnxrc/desmond/daq/build/dcm2DigitizerTest.dat";
jseb2Controller * pjseb2c;
char hostname[64];
char * adcserverhost;
bool jseb2Controller::is_busy = false; // set by ADC when data is ready and waiting to be read from jseb2
bool jseb2Controller::is_data_ready = false; // set by ADC when data is available to be read by jseb2
bool jseb2Controller::forcexit = false;
bool jseb2Controller::dmaallocationcompleted = false; // DMA allocation attempt done flag
bool jseb2Controller::dmaallocationsuccess   = true; // DMA allocation status flag

bool debug_commands = true;
bool enable_part_readout = true;
bool enable_ext_readoutdelay = false;
bool debug_adcparser = false;

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
  
 
 
  char adcConfigFilename[64];


 if (!strcmp(adcserverhost,"va097") || !strcmp(adcserverhost,"va096") )
	{

	  //if (enable_ext_readoutdelay == false) {
	  if (enable_part_readout == false) {
	    dcm2configfilename = "/home/phnxrc/desmond/daq/build/dcm2ADCTest96.dat";
	  } else {
	    dcm2configfilename = "/home/phnxrc/desmond/daq/build/dcm2ADCParM.dat";
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

	  if (enable_part_readout == false) {
	    dcm2configfilename = configfiledir + "dcm2ADCTest95.dat";
	  } else {
	    dcm2configfilename = configfiledir + "dcm2ADCPar.dat";
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

  

} // getDevice2Names

// get a text line from the input file
bool adc_getline(ifstream *fin, string *line) 
{
  // needed to parse config files with and without
  // end of line escape characters
  string tempstring;
  char * newline = NULL;
  (*line) = "";
  
  // read in all lines until the last char is not a '\'
  while (getline(*fin, tempstring)) { 
    // if no escape character, end of line
    int length = tempstring.size();
    if (tempstring[length-1] != '\\') {
      (*line) += tempstring;
      
      return true;
    } else { // continue to next line
      tempstring.resize(length-2);
      (*line) += tempstring;
    }
  }

  // end of file
  return false;

}


/*
 *
 * file format 
 *  each line contains a single line for each xmit group. Each line contains:
 *  nevents, n_xmitgroups, n_slot, n_modules, l1_delay, n_samples
 *  0,1,16,3,48,31
 */
int parseAdcConfigFile( string adcconfigfilename)
{
  string cfg_line;
 // open configuration file
  ifstream fin(adcconfigfilename.c_str());
  if (!fin) {
    cout << __FILE__ << " adc config file not found " << adcconfigfilename.c_str() << endl;
    return 1;
  }
  size_t pos = 0;
  
  
  int tokencount = 0;
  int nxmitgroups = 0;
  int tmpvalue = 0;
  char * ptmps;
  // parse each line
  while (adc_getline(&fin, &cfg_line)) 
    {
      tokencount = 0;
      //cout << __FILE__ << " " << __LINE__ << " parse search line " << cfg_line.c_str() << endl;
      
      // if first character is a / or a ' ' then skip the line
      char *tmpstr = new char [cfg_line.length() + 1];
      strcpy(tmpstr,cfg_line.c_str());
      if (isdigit(tmpstr[0]) != 0)
	{
           if (debug_adcparser)
             {
               cout << __FILE__ << " " << __LINE__ << " found parameter line " << tmpstr<< endl;
             }
	  // parse the config file string to int
	  ptmps = strtok(tmpstr,",");
	  // parse string for adc configuration values
          if (debug_adcparser)
             {
               cout << " GET THE VALUES for xmit group " << nxmitgroups << endl;
             }
	  while (ptmps != NULL)
	    {

	      switch (tokencount)
		{
		case 0:
		  sscanf(ptmps,"%d",&tmpvalue);
		  _nevents = tmpvalue;
                  if (debug_adcparser)
                    {
                      cout << "from file nevents = " << _nevents << endl;
                    }
                  break;
		case 1:
		  sscanf(ptmps,"%d",&tmpvalue);
		  xmit_groups[nxmitgroups].slot_nr = tmpvalue;
                  if (debug_adcparser)
                    {
                      cout << " got slot_nr " << tmpvalue << endl;
                    }
		  break;
		case 2:
		  sscanf(ptmps,"%d",&tmpvalue);
		  xmit_groups[nxmitgroups].nr_modules = tmpvalue;
                  if (debug_adcparser)
                    {
                      cout << " got nmodules " << tmpvalue << endl;
                    }
		  break;
		case 3:
		  sscanf(ptmps,"%d",&tmpvalue);
		  xmit_groups[nxmitgroups].l1_delay = tmpvalue;
                  if (debug_adcparser)
                    {
                      cout << " got l1_delay " << tmpvalue << endl;
                    }
		  break;
		case 4:
		  sscanf(ptmps,"%d",&tmpvalue);
		  xmit_groups[nxmitgroups].n_samples = tmpvalue;
                  if (debug_adcparser)
                    {
                      cout << " got n_samples " << tmpvalue << endl;
                    }
		  break;
		default :
		  break;
		}

	      ptmps = strtok(NULL,",");

	      tokencount++;

	    } // found a token

	  nxmitgroups++;
	} // line starts with digit so is a parameter line


      delete tmpstr;
    } // search string
  // set total number of xmit groups found
  if (debug_adcparser)
    {
      cout << __FILE__ << " LINE " << __LINE__ << " ADC nxmitgroups = " << nxmitgroups << endl;
    }
  _num_xmitgroups = nxmitgroups;
    return 0;

} // parseAdcConfigFile

bool dcm2_getline(ifstream *fin, string *line) {

  // needed to parse dat files with and without
  // end of line escape characters

  string tempstring;
  (*line) = "";
  
  // read in all lines until the last char is not a '\'
  while (getline(*fin, tempstring)) { 
    // if no escape character, end of line
    int length = tempstring.size();
    if (tempstring[length-1] != '\\') {
      (*line) += tempstring;
      
      return true;
    } else { // continue to next line
      tempstring.resize(length-2);
      (*line) += tempstring;
    }
  }

  // end of file
  return false;
}




string parseDcm2Configfile(string dcm2configfilename)
{
  
  string cfg_line;
 // open configuration file
  ifstream fin(dcm2configfilename.c_str());
  if (!fin) {
    
    return "none";
  }
  size_t pos = 0;
  string tokenholder;
  int linecount = 0;
  // in dat file name is on line 2 after name:
  while (dcm2_getline(&fin, &cfg_line)) 
    {
      //cout << "parse search line " << cfg_line.c_str() << endl;
      //while(( cfg_line.substr(cfg_line.find("name:"))) != string::npos) 
      // search for label:ONCS_PARTITIONER
      if((pos = cfg_line.find("label:")) != string::npos)
	{
	  tokenholder = cfg_line.substr(pos+6,16);
	  //cout << __LINE__ <<  " found label " << tokenholder.c_str() << endl;
	  
	}
      if (strncmp(tokenholder.c_str(),"ONCS_PARTITION",14) == 0 )
	{
	  //cout << __LINE__ << " found  ONCS_PARTITONER label " << endl;
      if((pos = cfg_line.find("name:")) != string::npos)
	{
	  // now should be at the position of the name: start after : character
	  dcm2name = cfg_line.substr(pos+5,15);
	  //cout << __LINE__ << "  found dcm2name " << dcm2name.c_str() << endl;
	  break;
	} // search for name word
	} else {
	//cout << __LINE__ << " NOT on ONCS_PARTITONER line - get next line" << endl;
      }
      linecount++;
      if (linecount > 4)
	break;

    } // search string

    return dcm2name;

} // parseDcm2Configfile(dcm2configfilename)

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
  ADC * pADC[3]; // array of pointers to adc objects

  int istart = 0;

  pthread_t dcm2_tid = 0;
  struct timeval tvstarttime,tvendtime,tvdifftime;
  char * cstarttime;
  char * cendtime;
  string _adcJseb2Name = "JSEB2.5.0";
 
  string adcconfigfiledir = "/home/phnxrc/";
  string dcm2datfilename =  "dcm2ADCPar.dat";
 
  dcm2configfilename = configfiledir +  dcm2datfilename;
  adcconfigfilename = adcconfigfiledir + "adcconfigdata.txt"; // default digitizer config file
  
 // get names of devices to use. Then search list of found jseb2 objects and create
  // a jseb2 object for each one

  gethostname((char *)hostname,64);
  adcserverhost = strtok(hostname,"."); // get host name

  // set default device names for jsebs and dcm2 configuration
 getDevice2Names();

 // get command arguments

 // scan for arguments
 if (argc > 1)
   for (int i = 1; i < argc; i++ ) {
     if (strcmp(argv[i], "-n") == 0) {
       nevents = atoi(argv[i+1]);
       numberofEvents = atoi(argv[i+1]);
     }
     // get adc configuration file name ; directory added to name
   if (strcmp(argv[i], "-c") == 0) {
     adcconfigfilename = adcconfigfiledir + argv[i+1];
     }
   if (strcmp(argv[i], "-d") == 0) {
     dcm2Jseb2Name = argv[i+1]; // dcm2 jseb2 name
   }
   if (strcmp(argv[i], "-a") == 0) {
     adcJseb2Name = argv[i+1]; // digitizer jseb2 name
   }
   if (strcmp(argv[i], "-p") == 0 ) {
     partreadoutJseb2Name = argv[i+1]; // partitioner jseb2 name
   }
   if (strcmp(argv[i], "-dat") == 0 ) {
     dcm2datfilename = argv[i+1]; // dcm2 dat file name
     dcm2configfilename = configfiledir +  dcm2datfilename;
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

     if (strcmp(argv[i], "-h") == 0) {
       cout << "Usage: runmxadc -n nevents (default = 100 ) -c digitizerconfigfile -dat dcm2 dat filename -d dcm2jseb2 -a digitizerjseb2 -p partitionerjseb2  -f PRDFdatafile  -h print help" << endl;
       cout << "\t Defaults: nevents = 100, digitizerconfigfile = adcconfigdata.txt, dcm2jseb2 = JSEB2.5.0,  partitionerjseb2 = JSEB2.8.0" << endl;
       return 0;
     }
   } // got arguments



  // here to the search for the jseb2 ; now assign the index 
 
  int partitionNb = 0;

  if (parseAdcConfigFile( adcconfigfilename) != 0 )
    {

	xmit_groups[0].slot_nr = 16;
	xmit_groups[0].nr_modules = 3;
	xmit_groups[0].l1_delay = 58;
	xmit_groups[0].n_samples = 31;
	_num_xmitgroups = 1;
         std::cout  << __FILE__ << " " << __LINE__ << " Error: unable to open configuration file  "<< adcconfigfilename.c_str() << " using defaults 16,3,58,31 " << endl;
         cout << " use defaults configuration: " << endl;
         cout << " num xmit groups " << _num_xmitgroups << endl;
         cout << " slot number     " << xmit_groups[0].slot_nr << endl;
         cout << " num digitizers  " << xmit_groups[0].nr_modules << endl;
         cout << " l1 delay        " << xmit_groups[0].l1_delay   << endl;
         cout << " num samples     " << xmit_groups[0].n_samples  << endl;

    }
  _nevents = nevents; // from command line argument
  // set dcm2 par file based on the number of xmit groups and on sample size

  if (xmit_groups[0].n_samples == 16 )
    {
      if (_num_xmitgroups > 1 )
        {
          dcm2configfilename = configfiledir + "dcm2ADCPar_16M.dat";
        } else
        dcm2configfilename = configfiledir + "dcm2ADCPar_16.dat"; // hitformat 95

    } else 
    {
      // 31 sample size

     if (_num_xmitgroups > 1 )
        {
          dcm2configfilename = configfiledir + "dcm2ADCPar_32M.dat";
        } else
       dcm2configfilename = configfiledir + "dcm2ADCPar_32.dat"; // hitformat 93
    }
 
 
  // create dcm2 control object
  string dcm2name = "DCMGROUP.TEST.1";

  //cout  << __FILE__ << " LINE " << __LINE__ << " parsing dcm2 file  " << dcm2configfilename.c_str() << endl;
  dcm2name = parseDcm2Configfile(dcm2configfilename);
  //cout << __FILE__ << " LINE " << __LINE__ << " using dcm2 name " << dcm2name.c_str() << endl;
  if (strcmp(dcm2name.c_str(),"none") == 0 )
    {
      cout << "Unable to open dcm2 configuration file " << dcm2configfilename.c_str() << endl;
      return 1;
    }
  //
  // adc needs number of loops, number of events, 1st module slot number, number of FEM modules
  pADC[0] = new ADC();
  pADC[0]->setNumXmitGroups(_num_xmitgroups);
   for ( int ig = 0; ig < _num_xmitgroups ; ig++ )
    {
      //cout << __FILE__ << " Create ADC for xmit group " << ig << endl;

	  pADC[0]->setNumAdcBoards(ig,xmit_groups[ig].nr_modules);
	  pADC[0 ]->setFirstAdcSlot(ig,xmit_groups[ig].slot_nr);
	  pADC[0 ]->setJseb2Name(adcJseb2Name);
	  pADC[0 ]->setL1Delay(xmit_groups[ig].l1_delay);
	  pADC[0 ]->setSampleSize(ig,xmit_groups[ig].n_samples);
          pADC[0]->setObDatafileName(obeventDataFileName); // TODO is this needed
          //cout << __LINE__ << "  " << __FILE__ << " NUM modules: " << xmit_groups[ig ].nr_modules 
          //     << " slot num: " <<  xmit_groups[ig ].slot_nr << " adcjseb " << adcJseb2Name.c_str() << endl;


    }

   //dcm2Impl dcm2control = dcm2Impl(dcm2name.c_str(),partitionNb );


  unsigned long cmdstatus;
  int startadcflag = 0;
  int stopadcflag  = 0;


    if (enable_dcm2)
    {
      //cout << "Download dcm2 file " << dcm2configfilename.c_str() << endl;
      pdcm2Object = new dcm2Impl(dcm2name.c_str(),partitionNb );
      //cout << "Type 1 to download the  DCM2 " << endl;
      //cin >> startadcflag;
      // set up process memory map for exit notificatin
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
       if (nevents == 0 )
	 cout << "Write unlimited events to file " << outputDataFileName.c_str() << endl;
       else
         cout << "Write " << nevents << " events to file " << outputDataFileName.c_str() << endl;

       pjseb2c = new jseb2Controller();
       jseb2Controller::forcexit = false; // signal jseb run end
       pjseb2c->Init(partreadoutJseb2Name);
       pjseb2c->setNumberofEvents(nevents);
       pjseb2c->setOutputFilename(outputDataFileName);
       //cout << " -- jseb2Controller: start_read2file ... " << endl;
       // add to initialize from digitizerTriggerHandler
       
       pjseb2c->setExitRequest(false);


       pjseb2c->start_read2file(); // start read2file  thread
       sleep(3);
       if (jseb2Controller::dmaallocationsuccess == false)
	 {
	   cerr << "Failed to allocate DMA buffer space - aborting start run " << endl;
	   if (enable_dcm2) {
	     delete pdcm2Object;
	       }
	   return 0;
	 }
       // check if failed to lock jseb2; exit if failed
       //if (jseb2Controller::runcompleted == true)
       //{
       //return 0;
       //}
     } // enable_part_readout




  if (enable_adc)
    {
      

      if (pADC[0]->initialize(_nevents) == false ) 
        {
	  cout << "Failed to initialize xmit - try restart windriver " << endl;
	  return 1;
        }
        
      // start run in thread

      if (enable_dcm2)
	{
	  
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
  } // not enable adc 

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
       cout << "runmxadc: Waiting for data completion " << endl;
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

    //cout << __FILE__ << " " << __LINE__ << " delete pADC "       << endl;
      delete pADC[0 ];
  }

  verbosity = Dcm2_BaseObject::LOG;
  if (enable_dcm2)
    {
      pdcm2Object->stopRun(cmdpartitionname);
    }
  if (enable_part_readout == true)
    {
      jseb2Controller::forcexit = true; // signal jseb run end
      pjseb2c->setExitRequest(true);
      sleep(2); // wait for data to be written
      pjseb2c->Close();
      delete pjseb2c;
    }
} // end main
