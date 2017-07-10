#include <stdio.h>
#include <iostream>
#include "dcm2Impl.h"
#include <Dcm2_Logger.h>
#include <dcm2_runcontrol.h> // for DCM2_xxx functions

using namespace std;

// the name is the device name e.g. DCMGROUP.MPCEX.S.1
dcm2Impl::dcm2Impl(const char *thedcmInterName, long PartitionerNb)
{
  status = 0;
  eventCount = 0;
  sprintf(dcmInterName,"%s",thedcmInterName);
  //verbosity = DEBUG;
  verbosity = LOG; // default TERMINAL
  this->PartitionerNb = PartitionerNb;
  strcpy(partitionName,dcmInterName);
  partnameidmap[0] = partitionName; // thedcmInterName;
  //cout << "dcm2Impl.ctor set partitionmap id " << PartitionerNb << " to " << partitionName << endl;
}


  dcm2Impl::~dcm2Impl()
{
 
}

#ifdef WITH_THREADS
//dcm2Impl:startThread(string partitionname, int NbOfEvents,const char * DataFileName)
pthread_t dcm2Impl::startRunThread(string partitionname,int NbOfEvents, const char * outputDataFileName)
{
 _partitionname = partitionname;
 _NbOfEvents = NbOfEvents;
 strcpy(_DataFilename,outputDataFileName);
 //pthread_t tid;
  int       result;


result = pthread_create(&tid, 0, dcm2Impl::RunThread, this);
// return 0 on success to create thread
 if (result == 0) {
   //pthread_detach(tid);
     return tid;
 }
 return 0;
} // startRunThread



#endif

string dcm2Impl::getPartitionName()
{
  return _partitionname;
} // getPartitionName
int dcm2Impl::getNumberOfEvents()
{
  return _NbOfEvents;
} // getNumberOfEvents

char * dcm2Impl::getOutputFile()
{
  return _DataFilename;
} // getOutputFile
#ifdef WITH_THREADS
// called from pthread_create to start thread
void *dcm2Impl::RunThread(void * arg)
{

  string _partitionname = ((dcm2Impl*)arg)->getPartitionName();
  int _numberOfEvents  = ((dcm2Impl*)arg)->getNumberOfEvents();
  char * outputDataFile   = ((dcm2Impl*)arg)->getOutputFile();
  ((dcm2Impl*)arg)->startRun(_partitionname,_numberOfEvents,outputDataFile);
  
string function = "DCM2impl_startRun()";
 Dcm2_Logger::Cout(Dcm2_BaseObject::INFO,function) 
	 << " StartRun: Return from startRun "   
	<< Dcm2_Logger::Endl;

 

  
}
#endif
void dcm2Impl::SetDataFileName(const char * outputDataFileName)
{
  strcpy(_DataFilename,outputDataFileName);
} // SetDataFileName

void dcm2Impl::SetPartitionname(string partitionname)
{
  _partitionname = partitionname;
} // SetPartitionname(

void dcm2Impl::setEventCount(string partitionname,string eventcount)
{
  // TODO setEventCoun
  sscanf(eventcount.c_str(),"%d",&eventCount);
  //cout << "dcm2Impl::setEventCount to " << eventCount << endl;


} // setEventCount

string dcm2Impl::getDeviceName()
{
  cout << "dcm2::getDeviceName returns " << dcmInterName << endl;
  return dcmInterName;
}

/**
 * setExitMap
 * set up global address for run completion flag to parent process from forked process
 */
int dcm2Impl::setExitMap()
{
  return DCM2_setExitMap();

}

  int dcm2Impl::init()
{
  partmapiter = partnameidmap.find(PartitionerNb);
  if (partmapiter == partnameidmap.end() )
    {
      cerr << "Error: partitioner number "<< PartitionerNb << " not mapped to a partitioner name"  << endl;
      return -1;
    } else {
 
    partitionername = partmapiter->second;
    DCM2_init(partitionername,&dcmInterStatus);
  }
  return status;
}
int  dcm2Impl::startReady(string partitionname)
{
  
  partmapiter = partnameidmap.find(PartitionerNb);
  if (partmapiter == partnameidmap.end() )
    {
      cout << "startReady Error: partitioner number "<< PartitionerNb << " not mapped to a partitioner name"  << endl;
      return -1;
    } else {
  
  
  partitionername = partmapiter->second;
  cout << "dcm2Impl:startReady partitionername = " << partitionername.c_str() << endl;
    cout << "dcm2Impl:startReady execute DCM2_startReady " << endl;

    DCM2_startReady(partitionname,&dcmInterFailureMode,&dcmInterStatus);
  
 
    cout << partitionername.c_str() << "  Returning from startReady: dcmInterStatus = " << dcmInterStatus << endl;


    if (dcmInterStatus)
      dcmInterError |= DCMISTARTREADY; // DCMISTARTREADY 0x4 ; interstatus bit 3 == busy

    }

  return(dcmInterStatus);

 
}

int dcm2Impl::cleanup(string partitionname)
{
  return DCM2_cleanup(partitionname,&status,verbosity);

} 

void dcm2Impl::raiseHold(string partitionname)
{
  unsigned int type = Dcm2_Partitioner::PART3_HOLD_IN;
  DCM2_raiseHold(partitionname,&status,type,verbosity);
}
void dcm2Impl::releaseHold(string partitionname)
{
  unsigned int type = Dcm2_Partitioner::PART3_HOLD_IN;
  DCM2_releaseHold(partitionname,&status,type,verbosity);
}

  void dcm2Impl::releaseBusy(string partitionname)
  {
    DCM2_releaseBusy(partitionname,&status,verbosity);
  } // releaseBusy

 void dcm2Impl::raiseBusy(string partitionname)
 {
   DCM2_raiseBusy(partitionname,&status,verbosity);
 }

/**
 * startRun
 * The Dcm2 (run16) DCMObject startrun operation executes in order:
 *  pDCMInter->startReady();
 *  pDCMInter->startRun(taskName,0,outputFilename);
 */
int dcm2Impl::startRun(string cmdpartitionname,int NbOfEvents,const char *DataFilename)
{
  bool generateFakeData = false;
  //verbosity = DEBUG; // set for full dcm2 DMA debug printout
  verbosity = LOG; // for debug output use TERMINAL;
  unsigned long failure_mode = 0;
  unsigned long number_events = 0;

#ifdef DEBUG_CONFIG

  cout << "dcm2Inter_i::startRun()" << endl;
  cout << "  PartitionerNb =     " << PartitionerNb << endl;
  cout << "  taskName =          " << taskName << endl;
  cout << "  Number of events =  " << NbOfEvents << endl;
  cout << "  Data Filename =     " << DataFilename << endl;

#endif
  number_events = NbOfEvents;

  partmapiter = partnameidmap.find(PartitionerNb);
  if (partmapiter == partnameidmap.end() )
    {
   
      cout << "Error: partitioner number "<< PartitionerNb << " not mapped to a partitioner name"  << endl;
 
      return -1;
    } else {
 
    partitionername = partmapiter->second;

    sprintf(DCMIUFileName[PartitionerNb],"%s",DataFilename);
    long status = 0;
 
    //int verbosity = Dcm2_BaseObject::TERMINAL;
    DCM2_startReady(partitionername,&failure_mode,&status,verbosity);
    cout << "dcm2Impl: execute DCM2_startRun for events "  << number_events  << endl;

    DCM2_startRun(partitionername,&number_events,DataFilename,&status,verbosity);
    //DCM2_startRun(partitionername,&eventCount,DataFilename,&status,verbosity);

       // get the startrun status
    dcmInterStatus = status;

    //cout << "dcm2Inter_i:startRun  Returning from DCM2_startRun: dcmInterStatus = " << dcmInterStatus << endl;
    if (dcmInterStatus) {
      dcmInterError = dcmInterStatus;
      dcmInterError |= DCMISTARTRUN;

    } // failed status
  } // found partition name

  return(dcmInterStatus);

  
} // startRun



/**
 * stopRun
 * The Dcm2 (run16) DCMObject stoprun operation executes in order:
 *  pDCMInter->stopRun();
 *  
 */
  int dcm2Impl::stopRun(string partitionname)
{
  verbosity = LOG;
  partmapiter = partnameidmap.find(PartitionerNb);
  if (partmapiter == partnameidmap.end() )
    {
      cout << "Error: partitioner number "<< PartitionerNb << " not mapped to a partitioner name"  << endl;
      return -1;
    } else {
 
    partitionername = partmapiter->second;

    DCM2_stopRun(partitionername,&status,verbosity);
   
    //cout << "  Returning from stopRun: status = " << status << endl;

    if (status)
      dcmInterError |= DCMISTOPRUN;

  }

  return(status);

    return status;
} // stopRun

/**
 * checkReady
 * The Dcm2 (run16) DCMObject status operation executes in order:
 *  pDCMInter->checkReady();
 *  
 */ 
int dcm2Impl::checkReady(string cmdpartitionname)
{
  long checkReadyValue = 0;
  long dcm2status = 0;
  bool verbosity = false;
 

  //cout << "dcm2Inter_i::checkReady()" << endl;
  //cout << "  PartitionerNb =     " << PartitionerNb << endl;
  dcm_debug = 1;

  partmapiter = partnameidmap.find(PartitionerNb);
  if (partmapiter == partnameidmap.end() )
    {
      cout << "checkReady Error: partitioner number "<< PartitionerNb << " not mapped to a partitioner name"  << endl;
      return -1;
    } else {
 
    partitionername = partmapiter->second;
    //cerr << "dcm2Inter_i:: call checkReady on partitioner " << partitionername.c_str() << endl;
    // the dcm2 checkready has an exit code ( the return value, a status field and a retvalue
    // return code:
    // 1 return code 1 = failure and  status also = 1 then download cannot be done - total failure
    //         no download CANNONT executed
    // 2 return code 1 = failure and  status = 0; no previous pcf download 
    //         download CAN  proceed
    // 3 return code 1 and status = 1 ; no download can occur
    //         download CANNOT be executed
    // 4 return code 0
    // dcm2 is status checkReadyValue = retvalue
    // the retvalue is a 64 bit long value 
    // DCMISHIFT is 4 bits


    DCM2_checkReady(partitionername,&checkReadyValue,&dcm2status,verbosity);

    dcm_debug = 0;
    if (dcmInterError)
      checkReadyValue |= (dcmInterError<<DCMISHIFT);

  }

  return(checkReadyValue);
} // checkReady

int dcm2Impl::runStatus()
{


  return status;
} // runStatus

/**
 * getEventsProcessed
 * gets the number of events processed for a single dcm2board fiber
 * If fiber == 0 then return the sum of all events for this dcm2 board
 */
int dcm2Impl::getEventsProcessed(string partitionername,int fiber)
{
  // get pointer to a Dcm2_Board and execute GetNumberOfProcessedEvents(
  // return pdcm2board->GetNumberOfProcessedEvents(fiber);
  unsigned int processedEvents = 0;
  
  //processedEvents = pdcm2board->GetNumberOfProcessedEvents(fiber);
  processedEvents = DCM2_getEventCount(partitionername,verbosity);

  // OR execute DCM2_READER_getEventCount(std::string jseb2name, int verbosity) 
  // partitionername is jseb2name ex JSEB2.8.0
  //processedEvents = DCM2_READER_getEventCount(partitionername,verbosity);
return processedEvents;
} // getEventsProcessed

/**
 * download
 * this is the dcm2 download function 
 * arguments: .dat file name, partitioner number
 */
int dcm2Impl::download(string cmdpartitionname,string configFilename,int partitionerNb)
{
  long status = 0;

  //std::string partitionernamet(dcmInterName); 
  std::string partitionernamet(cmdpartitionname);
  std::string configfilet(configFilename);  // dat file
  partitionerNumb = partitionerNb;

  bzero(DCMIUFileName[partitionerNumb],DCMIUFNSIZE);
  sprintf(DCMIUFileName[partitionerNumb],"NONE");
  dcm_debug = 1;

  // parse the object name to get the partition name
  // DCMGROUP.VTXP.1 -> VTXP.1
  char * pdot = strchr(dcmInterName,'.');
  //strcpy(partitionName,pdot + 1);
  // the partition name is DCMGROUP.VTXP.1
  //strcpy(partitionName,dcmInterName);
  strcpy(partitionName,cmdpartitionname.c_str());
  //cout << "dcm2Impl:download using config file " << configFilename.c_str() << endl;

  //cout << "dcm2Impl.download set partitionmap id " << partitionerNb << " to " << partitionName << endl;
  //cout << "dcm2Impl: call DCM2_readConfig *** " << endl;
 DCM2_readConfig(partitionernamet,&configFilename,&status,verbosity);
 DCM2_init(partitionernamet,&status,verbosity);
 /*
  if (DCM2_readConfig(partitionernamet,&configFilename,&status,verbosity) 
	== DCM2_GOOD_EXIT) {
    if (DCM2_modify_L1DD_readstate(partitionernamet,0,&status,verbosity) 
	  == DCM2_GOOD_EXIT) {
      cout << "dcm2Impl: call DCM2_init *** " << endl;
	if (DCM2_init(partitionernamet,&status,verbosity) == DCM2_GOOD_EXIT) {
	  if (verbosity >= TERMINAL) {
	    cout << "Dcm2() - " << partitionernamet << " downloaded" << endl;
	  }
	} else {
	  status = 3; // failed dcm2_init
	}
    }
  }
 */
 return status;
} // download

