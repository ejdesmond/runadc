#include <jseb2Controller.h>
bool jseb2Controller::runcompleted = false;

jseb2Controller::jseb2Controller()
{
  _verbosity = LOG;
   verbosity = Dcm2_BaseObject::LOG;
  nevents = 0; // default to unlimited events
  // default output file name
  strcpy(outputfilename,"dcm2jseb2adcoutput.prdf");
  exitRequested = false; //
  writeFrames = true; // default write to file enabled

}
  jseb2Controller::~jseb2Controller()
  {
    delete controller;
    windriver->Close();
  }

  void jseb2Controller::Close()
  {

  } // Close

  void jseb2Controller::enableWrite2File()
  {
    writeFrames = true; //  write to file enabled
  }
  void jseb2Controller::disableWrite2File()
  {
    writeFrames = false; // disable write to file 
  }


  void jseb2Controller::start_read2file()
  {
     int       result;
     pthread_t tid;

     result = pthread_create(&tid, 0, jseb2Controller::read2FileThread, this);
  } // start_read2file

void * jseb2Controller::read2FileThread(void * arg)
{
  ((jseb2Controller*)arg)->jseb2_seb_read2file();

  cout << "jseb2Controller: end read2FileThread" << endl;
} // read2FileThread

 void jseb2Controller::Exit(int command)
 {
   cout << "jseb2Controller Exit called " << endl;
   exitRequested = true;
   return;
 } // Exit

  bool jseb2Controller::Init(string jseb2name)
  {
    // set name of jseb2 to use
       _jseb2name = jseb2name; 
       //Dcm2_WinDriver *windriver = Dcm2_WinDriver::instance();
  windriver = Dcm2_WinDriver::instance();
  windriver->SetVerbosity(verbosity);
  if(!windriver->Init())
    {
      cerr << "   JSEB2() - Failed to load WinDriver Library" << endl;
      windriver->Close();
      return 0;
    }     

  //Dcm2_Controller *controller = new Dcm2_Controller();
  controller = new Dcm2_Controller();
  controller->SetVerbosity(verbosity);
  if(!controller->Init())
    {
      cerr << "   JSEB2() - Failed to initialize the controller machine" << endl;
      delete controller;
      windriver->Close();
      return 0;
    }

  unsigned int njseb2s = controller->GetNJSEB2s();
  cout << "   A scan of the PCIe bus shows " << njseb2s << " JSEB2(s) installed" << endl;
  cout << endl;

   jseb2 = controller->GetJSEB2(jseb2name); // uses internal jseb2
    jseb2->Init();
    //delete controller;
    //windriver->Close();
  } // Init

/**
 * setJseb2Name
 * sets the name of the jseb2 2 use
 * must be called before init
 */
  void jseb2Controller::setJseb2Name(string jseb2name)
  {
    _jseb2name = jseb2name;
 
  } // setJseb2Name
  void jseb2Controller::setNumberofEvents(unsigned long   numberofevents)
  {
    nevents = numberofevents;
  } // setNumberofEvents

  void jseb2Controller::setOutputFilename(string outputdatafilename)
  {
    strcpy(outputfilename,outputdatafilename.c_str());
    cout << "jseb2Controller: using output data file : " << outputfilename << endl;

  } // setOutputFilename



  void jseb2Controller::jseb2_seb_read2file()
  {
  bool printWords = false;
  bool printFrames = false;




  jseb2_seb_reader(jseb2, printWords, printFrames, writeFrames);
  cout << "jeb2_seb_read2file: returned from jseb2_seb_reader " << endl;
  
  jseb2Controller::runcompleted = true;
  } // jseb2_seb_read2file
  
/**
 * jseb2_seb_reader
 * copied from jseb2 on 4/11/2017
 */
void jseb2Controller::jseb2_seb_reader(Dcm2_JSEB2 *jseb2, bool printWords, bool printFrames, bool writeFrames)
{
  // kill signal capture
  //signal(SIGABRT, &jseb2_exit);
  //signal(SIGTERM, &jseb2_exit);
  //signal(SIGINT, &jseb2_exit);

  bool failDetected = false;
  bool debugDma     = false;
  bool debugEventReadout = false;

  cout << endl;
  if(printWords)
    {
      cout << "   Beginning SEB Receiver Read Words to Screen..." << endl;
    }
  if(printFrames)
    {
      cout << "   Beginning SEB Receiver Read Frames to Screen..." << endl;
    }
  if(writeFrames)
    {
      cout << "   Beginning SEB Receiver Read Frames to File..." << endl;
    }

  jseb2->SetReceiverDefaults();

  unsigned int rcv = 3;
  int MAX_PRINT_ERRORS = 20;
  int frame_errors_printed = 0;
 

  if(writeFrames)
    {
      //cout << "   Please enter the name of an output file:" << endl;
      //scanf("%s",outputfilename);
      cout << "jseb2Controller: Using output data file: " << outputfilename << endl;
      //cout << "   Please select the desired number of events (0 for unlimited):" << endl;
      //scanf("%lu",&nevents);
      cout << "jseb2Controller: Collecting  " << nevents << " events " << endl;
    }

  // file output
  Dcm2_FileBuffer *filebuffer = new Dcm2_FileBuffer();
  string filename(outputfilename);
  unsigned long ievent = 0;
  UINT32 event_offset = 0;
  UINT32 event_number = 0;
  if(writeFrames)
    {
      filebuffer->SetVerbosity(Dcm2_FileBuffer::LOG);
      filebuffer->SetRunNumber(0xdcb25eb);
      if(!filebuffer->OpenFile(filename))
	{ 
	  cerr << "   Failed to open output file: " << filename << endl;
	  delete filebuffer;
	  return;
	}
    }

  // Frame storage
  UINT32 *pframe = NULL; // beginning of frame in dma buffer
  unsigned int iframe = 0;
  unsigned int ioffset = 0;
  unsigned int nFrameWords = 0;
  unsigned int nFramesPerEvent = 0;
  const unsigned int maxFrameWords = 8*Dcm2_FileBuffer::MAX_WORDS_PER_PACKET;
  UINT32 frameWords[maxFrameWords];
  UINT32 *pframeWords = (UINT32*)&frameWords;

  // DMA buffer settings
  const unsigned int nbuffers = 2;
  UINT32 nbytes = 4*1024*1024; // 4 MB buffers
  DWORD dwDMABufSize = nbytes;

  PVOID pbuff[nbuffers];
  WD_DMA *pDma[nbuffers];   
  DWORD dwOptions = DMA_FROM_DEVICE | DMA_ALLOW_64BIT_ADDRESS; // required to prevent system meltdowns on over-allocation

  cout << "   Allocating DMA buffers size = " << dwDMABufSize << endl;

  // Get DMA buffers
  UINT32 *pwords[nbuffers];
  for(unsigned int idma = 0; idma < nbuffers; idma++)
    {
      WDC_DMAContigBufLock(jseb2->GetHandle(), &pbuff[idma], dwOptions, dwDMABufSize, &pDma[idma]);
      pwords[idma] = static_cast<UINT32*>(pbuff[idma]);
    }

  // Prepare for operation
  jseb2->EnableHolds(0x2000);       // FIFO fills to 16kB then TRX exerts hold
  jseb2->AbortDMA();                // Ends previous DMA
  jseb2->ClearReceiverFIFO(rcv);    // Flushes FIFOs
  jseb2->SetUseLock(true);          // Lock the JSEB2 use during DMAs
  
  if(!jseb2->WaitForLock(5))
    {
      cout << "  Unable to lock JSEB2, attempting cleanup..." << endl;
      jseb2->DestroyLock(); // called in jseb2_cleanup
      jseb2->CleanUp(Dcm2_JSEB2::RCV_BOTH);

      if(!jseb2->WaitForLock(5))
	{
	  cout << "  Still unable to lock JSEB2, exiting..." << endl;
      
	  return;
	}
    }
  cout << "   Locked JSEB2 for use" << endl;

  bool DMAcomplete = false;

  UINT32 bytesleftold = nbytes;
  UINT32 bytesleftnew = nbytes;
  UINT32 receivedbytes = bytesleftold-bytesleftnew;
  UINT32 receivedwords = receivedbytes/sizeof(UINT32);

  UINT32 status = 0x0;
  UINT32 bytesleftrcvr = 0;

  unsigned int idma = nbuffers-1;
  bool firstpass = true;

  UINT64 bytesThroughOptics = 0;
  UINT64 bytesThroughDMA = 0; 
  UINT64 bytesLeftOnJSEB2 = 0;

  while(true)
    {
      // calculate bytes through optics
      if(firstpass)
	{
	  bytesThroughOptics = 0;
	  bytesThroughDMA = 0;
	}
      else // not first pass
	{ // not first pass
	  if (debugEventReadout)
	     {
	       cout << "LINE " << __LINE__ <<" start NEXT  pass: " << endl;
	      }
	  if(!DMAcomplete)
	    {
	      // record progress on previous DMA
	      bytesleftnew = jseb2->GetBytesLeftToDMA();
	      if (debugDma)
		cout << "LINE " << __LINE__ << "  not first pass: DMA NOT complete bytesleftnew = " << bytesleftnew << endl;

	      // abort the previous DMA
	      jseb2->AbortDMA();

	      // sync the processor cache to system memory
	      WDC_DMASyncIo(pDma[idma]);
	    }
	  else
	    {
	      // record progress on previous DMA
	      bytesleftnew = 0;

	      // sync the processor cache to system memory
	      WDC_DMASyncIo(pDma[idma]);
	    }
	  
	  // update counter for bytes out on DMA
	  bytesThroughDMA += (nbytes - bytesleftnew);
	  if (debugDma) {
	    cout << "jseb2: DMA done bytesThroughDMA = " << bytesThroughDMA << endl;
	    cout << "jseb2: bytesleftnew = " << bytesleftnew << " bytesleftold = " << bytesleftold << endl;
	  }
	  // print any additional DMAed words since last print...
	  if(bytesleftnew != bytesleftold)
	    {
	      receivedbytes = bytesleftold-bytesleftnew;
	      receivedwords = receivedbytes/sizeof(UINT32);
	  
	      //---BEGIN PARSING DATA STREAM------------------------

	      // print out new words and/or advance dma progress pointer
	      if(printWords)
		{
		  cout << "   (#" << idma << ") ";
		  for(unsigned int iword = 1; iword < receivedwords+1; iword++)
		    {
		      UINT32 u32Data = *pwords[idma];
		      pwords[idma]++;
		      cout.width(8); cout.fill('0'); cout << hex << u32Data << dec << " ";
		      if(iword%8 == 0)
			{
			  cout << endl; 
			  if(iword < receivedwords) cout << "   (#" << idma << ") ";
			}
		    }
		}
	      else // just advance pointer
		{
		  for(unsigned int iword = 1; iword < receivedwords+1; iword++)
		    {
		      pwords[idma]++;
		    }
		}

	      // parse new data for frames
	      if(printFrames || writeFrames)
		{
		  // check for new frame word...
		  while(pframe < pwords[idma])
		    {	
		      // copy out length of new frame
		      if(nFrameWords == 0)
			{
			  
			  nFrameWords = *pframe; 
			  
			}

		      // see if this frame has been fully DMAed
		      UINT32 DMAprogress = pwords[idma] - pframe; // number of words
		      if(nFrameWords - ioffset < DMAprogress)
			{
			  // put the remainder of this frame into array
			  for(unsigned int iword = ioffset; iword < nFrameWords; iword++)
			    {
			      frameWords[iword] = *pframe;
			      pframe++;
			    }

			  // the first frame will be an empty partitioner frame
			  if(iframe == 0)
			    {
			      if(writeFrames)
				{
				  
				  filebuffer->StartEvent();
				}

			      unsigned int oldFramesPerEvent = nFramesPerEvent;
			      nFramesPerEvent = (frameWords[3] >> 16);
			      if(nFramesPerEvent == 0) nFramesPerEvent = oldFramesPerEvent;

			      if(printFrames)
				{
				  cout << endl;
				  cout << "   PART3 Frame: ";
				  for(unsigned int iword = 0; iword < 8; iword++)
				    {
				      cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
				    }
				  cout << endl;
				}
			  
			      if(frameWords[0] != 0x8)
				{
				  cout << "Sync frame has incorrect length " << frameWords[0] << endl;
				  exitRequested = true; printWords = true;
				  failDetected = true;
				  break;
				}

			      if(frameWords[1] != 0xFFFFFF00)
				{
				  cout << "Corrupt or missing frame marker" << endl;
			  
				  failDetected = true;
				  exitRequested = true; printWords = true;
				  break;
				}
			    }
			  // additional frames will be from each dcm2 board
			  else
			    {
			      // coding around Chi's extra sync frame appearances
			      if(frameWords[0] != 0x8)
				{
				  if(printFrames)
				    {
				      cout << "   DCM2 Frame:::  ";
				      for(unsigned int iword = 0; iword < 8; iword++)
					{
					  cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
					}
				      cout << "... ";
				      cout.width(8); cout.fill('0'); cout << hex << frameWords[nFrameWords-1] << dec;
				      cout << endl;
				    }

				  if(frameWords[1] != 0xFFFFFF00)
				    {
				      cout << "Corrupt or missing frame marker" << endl;
				  
				      failDetected = true;
				      exitRequested = true; printWords = true;
				      break;
				    }

				  if(writeFrames)
				    {
				      // get event number
				      UINT32 prev_event_number = event_number;
				      event_number = frameWords[6];
				      if(event_number < prev_event_number)
					{
					  event_offset += prev_event_number;

					  if(event_number == 0) event_offset++;
					}
				      
				      // set event number
				      filebuffer->SetEventNumber(event_number+event_offset);
				      //cout << "jseb2: copyFrameIntoBuffer for event " << event_number+event_offset << endl;
				      // add frame to file buffer
				      filebuffer->CopyFrameIntoBuffer(nFrameWords,pframeWords);
				    }
				}
			      else
				{
				  cout << "---Skipping spurious sync frame---" << endl;
				  iframe--;
				}
			    }

			  ioffset = 0;
			  nFrameWords = 0;
			  iframe++;

			  // end the event when all frames are collected
			  if(iframe > nFramesPerEvent)
			    {
			      if(writeFrames)
				{
				  filebuffer->EndEvent();
				  filebuffer->CheckBufferSpace();

				  ievent++;
				  if (debugDma)
				    {
				      cout << "LINE1 " << __LINE__ << " got the last frame event end " << ievent << endl;
				    }
				  if(nevents != 0)
				    {
				      if(ievent > nevents)
					{
					  exitRequested = true; 
					}
				    }
				  if (debugEventReadout)
				    {
				  // print out procress
				  if(ievent < 5)
				    {
				      cout << "   SEB Readback Events Written = " << ievent << ", Event Number = " << event_number+event_offset << endl;
				    }
				  else
				    {
				      if(ievent%100 == 0) cout << "   SEB Readback Events Written = " << ievent << ", Event Number = " << event_number+event_offset << endl;
				    }
				  if(ievent == 5) cout << "   Progress will now be updated every 100 events" << endl;
				    }

				}
			      iframe = 0;
			    }
			} // nFrameWords - ioffset < DMAprogress
		      else // frame is not fully DMAed
			{
			  // put remaining frame fragment into array
			  UINT32 frag_begin = ioffset;
			  UINT32 frag_size = (pwords[idma] - pframe); // gives number of words, not bytes

			  for(unsigned int iword = 0; iword < frag_size; iword++)
			    {
			      frameWords[iword+frag_begin] = *pframe;
			      pframe++;
			      ioffset++;
			    }
			} // frame not fully DMAed

		      if(exitRequested)
			{
			  //printWords = true;
			  break;
			}
		    } // pframe < pwords[idma]
		} // writeFrames

	      //---FINISHED PARSING DATA STREAM------------------------

	      // go back to waiting for new data
	      bytesleftold = bytesleftnew;
	      receivedbytes = 0;
	      receivedwords = 0;
	    } // bytesleftnew != bytesleftold

	  if(failDetected) break;

	  // stress hold-busy chain during swap
	  //cout << "----pause started (dma swap)----" << endl;
	  //sleep(2);
	  //cout << "----pause complete (dma swap)----" << endl;
	} // not first pass
      // continue with first pass
      // round robin on DMA buffers
      idma++;
      if(idma == nbuffers)
	{
	  idma = 0;
	}

      if(debugDma)
	cout << "jseb2: end firstpass check idma = " << idma  << endl;

      // Reset Pointer (needed?)
      //pwords[idma] = static_cast<UINT32*>(pbuff[idma]);

      if(!firstpass)
	{
	  // update counter for bytes into optics
	  status = jseb2->GetReceiverStatusWord(1);
	}
      if (debugDma) {
	cout << "LINE " << __LINE__ << " start new stream nbytes = " << nbytes << " on jseb2 port " << rcv << endl;
      }
      // start new receive counter & DMA immediately
      jseb2->SetReceiveLength(rcv,nbytes);   // this way we don't kill the FIFO data
      jseb2->Receive(rcv,nbytes,pDma[idma]); // start new data stream
      DMAcomplete = false;
      if (debugDma) {
	cout << "LINE " << __LINE__ << " FIRST RECEIVE DONE firstpass = " << firstpass  << endl;
      }
      // Reset pointers
      pwords[idma] = static_cast<UINT32*>(pbuff[idma]);
      pframe = pwords[idma];

      // now calculate byte counts
      if(!firstpass)
	{
	  bytesleftrcvr = (status & 0x0FFFFFFF);
	  bytesThroughOptics += (nbytes - bytesleftrcvr);
	  if (debugDma)
	    {
	      cout << "jseb2_seb_reader: not FIRST PASS " << endl;
	      cout <<  "jseb2:  bytesThroughOptics = " << bytesThroughOptics << " bytesThroughDMA = " << bytesThroughDMA << endl;
	    }
	  bytesLeftOnJSEB2 += bytesThroughOptics - bytesThroughDMA;
	  bytesThroughOptics = 0;
	  bytesThroughDMA = 0;

	  if (debugDma)
	    {
	      cout << "jseb2_seb_reader: not FIRST PASS " << endl;
	      cout << "jseb2: bytesleftonJSEB2 = " << bytesLeftOnJSEB2 << endl;
	    }      if (debugDma) {
	cout << "LINE " << __LINE__ << " start new stream nbytes = " << nbytes << endl;
      }
	} // not firstpass

      // Remaining DMA length
      bytesleftold = nbytes; // 4 * 1024 * 1024 = 4194304
      bytesleftnew = nbytes;
      receivedbytes = bytesleftold-bytesleftnew;
      receivedwords = receivedbytes/sizeof(UINT32);

      

	  if (debugEventReadout)
	    {
	      cout << "DMA buffer size =  " << nbytes << endl;
	      cout << "LINE " << __LINE__ << "  bytesleftold  = " << bytesleftold << " bytesleftnew = " << bytesleftnew << " received bytes = " << receivedbytes << endl;
	    }
      // New DMA has begun (tell user to start data stream)
	  if (debugDma)
	    {
      cout << endl;
      cout << "   |==========================================================================|" << endl;
      cout << "   |   New DMA is running on Buffer #"<< idma << " and JSEB2 is ready to receive words    |" << endl;
      cout << "   |==========================================================================|" << endl;
	    }
      bool bufferOkay = true;
      unsigned int iprint = 0;
      while(bufferOkay)
	{

	  if (debugDma)
	    {
	      cout << "LINE " << __LINE__ << " bufferOkay - wait for data from DMA " << endl;
	    }
	  // Wait for data
	  bool waitingOnData = true;
	  
	  while(waitingOnData)
	    {
	      WDC_Sleep(100, WDC_SLEEP_BUSY);

	      bytesleftnew = jseb2->GetBytesLeftToDMA();
	      
	      if(bytesleftnew != bytesleftold)
		{
		  waitingOnData = false;
		}

	      if(exitRequested)
		{
		  //printWords = true;
		  break;
		}
	    } // while(waitingOnData)

	  receivedbytes = bytesleftold-bytesleftnew;
	  receivedwords = receivedbytes/sizeof(UINT32);

	  if (debugDma)
	    {
	      cout << "LINE " << __LINE__ << " got data from DMA received bytes = " << receivedbytes << endl;
	    }

	  if (debugDma)
	    {
	      cout << "got data from DMA " << endl;
	      cout << "jseb2_seb_reader:  bytesleftold  = " << bytesleftold << " bytesleftnew = " << bytesleftnew << " received bytes = " << receivedbytes << " received words = " << receivedwords << endl;
	      cout << "jseb2_seb_reader: NOW BEGIN PARSING DATA  "  << endl;
	    }
	  //---BEGIN PARSING DATA STREAM------------------------
	  
	  // print out new words and/or advance dma progress pointer
	  if(printWords)
	    {
	      cout << "   (#" << idma << ") ";
	      for(unsigned int iword = 1; iword < receivedwords+1; iword++)
		{
		  UINT32 u32Data = *pwords[idma];
		  pwords[idma]++;
		  cout.width(8); cout.fill('0'); cout << hex << u32Data << dec << " ";
		  if(iword%8 == 0)
		    {
		      cout << endl; 
		      if(iword < receivedwords) cout << "   (#" << idma << ") ";
		    }
		}
	      iprint++;
	    }
	  else // just advance pointer
	    {
	      for(unsigned int iword = 1; iword < receivedwords+1; iword++)
		{
		  pwords[idma]++;
		}
	      iprint++;
	    }

	  // parse new data for frames
	  if(printFrames || writeFrames)
	    {
	      if (debugDma ) {
	      cout << "jseb2: check for new frame word (pframe < pwords) pframe= "<< pframe << " pwords["<< idma<<"] = " << pwords[idma] << endl;
	      }
	      // check for new frame word...
	      while(pframe < pwords[idma])
		{	
		  // copy out length of new frame
		  if(nFrameWords == 0)
		    {
		      
		      nFrameWords = *pframe; // first word is the frame size
		      
		       
		    }

		  // see if this frame has been fully DMAed
		  
		  UINT32 DMAprogress = pwords[idma] - pframe ; // number of words
	  if (debugDma)
	    {
	      cout << "jseb2_seb_reader: DMAprogress ( #words ) = " << DMAprogress  << endl;
	      cout << "jseb2_seb_reader: iframe ( 0 if first frame ) =  " << iframe  << " nFrameWords = " << nFrameWords << " ioffset = " << ioffset << " nFrameWords - ioffset = " << nFrameWords - ioffset  << endl;
	      cout << " length of new frame is first word " << hex << *pframe  << dec << endl;
	    }

		  if(nFrameWords - ioffset < DMAprogress)
		    {
		      if (debugDma)
			    {
			      cout << "jseb2_seb_reader: nFrameWords is less than # DMA'd words - got frame" << endl;
			    }
		      // put the remainder of this frame into array
		      for(unsigned int iword = ioffset; iword < nFrameWords; iword++)
			{
			  frameWords[iword] = *pframe;
			  pframe++;
			}

		      // the first frame will be an empty partitioner frame
		      if(iframe == 0)
			{
			  if(writeFrames)
			    {
			      filebuffer->StartEvent();
			    }
			  if (debugDma)
			    {
			      cout << "jseb2: write first frame event " << endl;
			    }
			  unsigned int oldFramesPerEvent = nFramesPerEvent;
			  nFramesPerEvent = (frameWords[3] >> 16);
			  if(nFramesPerEvent == 0) nFramesPerEvent = oldFramesPerEvent;

			  if(printFrames)
			    {
			      cout << endl;
			      cout << "seb2:   PART3 Frame: ";
			      for(unsigned int iword = 0; iword < 8; iword++)
				{
				  cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
				}
			      cout << endl;
			    }

			  if (debugDma)
			    {
			      cout << "jseb2_seb_reader: check sync frame ( should be 0x8 ) =  " << frameWords[0] << endl;
			      
			    }

			  if(frameWords[0] != 0x8)
			    {
			      cout << "Sync frame has incorrect length... frameWords[0] = 0x " << hex << frameWords[0] << dec << endl;
			      exitRequested = true; printWords = true;
			      failDetected = true;
			      break;
			    }

			  if(frameWords[1] != 0xFFFFFF00)
			    {
			      cout << "Corrupt or missing frame marker should be 0xFFFFFF00" << endl;
			  
			      failDetected = true;
			      exitRequested = true; printWords = true;
			      break;
			    }
			} // iframie = 0
		      // additional frames will be from each dcm2 board
		      else
			{
			  if (debugDma)
			    cout << "jseb2: got frame " << iframe << " NOT the first frame "  << endl;

			  // coding around Chi's extra sync frame appearances
			  if(frameWords[0] != 0x8)
			    {
			      if(printFrames)
				{
				  cout << "   DCM2 Frame:_  ";
				  for(unsigned int iword = 0; iword < 8; iword++)
				    {
				      cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
				    }
				  cout << "... ";
				  cout.width(8); cout.fill('0'); cout << hex << frameWords[nFrameWords-1] << dec;
				  cout << endl;
				}

			      if(frameWords[1] != 0xFFFFFF00)
				{
				  cout << "Corrupt or missing frame marker" << endl;
				  
				  failDetected = true;
				  exitRequested = true; printWords = true;
				  break;
				}

			      if(writeFrames)
				{
				  // get event number
				  UINT32 prev_event_number = event_number;
				  event_number = frameWords[6];
				  if(event_number < prev_event_number)
				    {
				      event_offset += prev_event_number;

				      if(event_number == 0) event_offset++;
				    }
				      
				  // set event number
				  filebuffer->SetEventNumber(event_number+event_offset);
				  //cout << "jseb2: copy frame to buffer event# " << event_number+event_offset << endl;
				  // add frame to file buffer
				  filebuffer->CopyFrameIntoBuffer(nFrameWords,pframeWords);
				}
			    }
			  else
			    {
			      cout << "---Skipping spurious sync frame---" << endl;
			      iframe--;
			    }
			}

		      ioffset = 0;
		      nFrameWords = 0;
		      iframe++;
		      if (debugDma)
			{
			  cout << "jseb2_seb_reader: check if last frame iframe = " << iframe << " last frame"  << nFramesPerEvent << endl;
			  
			}
		      // end the event when all frames are collected
		      if(iframe > nFramesPerEvent)
			{
			  if (debugDma)
			    {
			      cout << "LINE " << __LINE__ << " got the last frame event end " << ievent << endl;
			    }
			  if(writeFrames)
			    {
			      filebuffer->EndEvent();
			      filebuffer->CheckBufferSpace();

			      ievent++;
			      
			      if(nevents != 0)
				{
				  if(ievent > nevents)
				    {
				      exitRequested = true; 
				    }
				}
			      if (debugEventReadout)
				{
			      // print out procress
			      if(ievent < 5)
				{
				  cout << "   SEB Readback Events Written = " << ievent << ", Event Number = " << event_number+event_offset << endl;
				}
			      else
				{
				  if(ievent%1000 == 0) cout << "   SEB Readback Events Written = " << ievent << ", Event Number = " << event_number+event_offset << endl;
				}
			      if(ievent == 5) cout << "   Progress will now be updated every 100 events" << endl;
				} // debug printout
			    }
			  iframe = 0;
			}
		    }
		  else // frame is not fully DMAed
		    {
		      
		      // put remaining frame fragment into array
		      UINT32 frag_begin = ioffset;
		      //UINT32 frag_size = (pwords[idma] - pframe); // gives number of words, not bytes orig
		      UINT32 frag_size = (pwords[idma] - pframe); // gives number of words, not bytes
		      if (debugDma) {
		      cout << "jseb2: frame not fully DMAed " << endl;
		      cout << "jseb2: dma fragment size ( whats read ) = " << frag_size << endl;
		      cout << "jseb2: inc frame pointer by " << frag_size << " words " << endl;
		      cout << "    - puts frame pointer to end of frame data " << endl;
		      }
		      for(unsigned int iword = 0; iword < frag_size; iword++)
			{
			  frameWords[iword+frag_begin] = *pframe;
			  pframe++;
			  ioffset++;
			}
		    } // not fully dma's frame copied

		  if(exitRequested)
		    {
		      //printWords = true;
		      break;
		    }
		} // pframe < pwords[idma]
	    } // if writeFrames

	  //---FINISHED PARSING DATA STREAM------------------------

	  if(debugDma)
	    cout << "Done PARSING DATA STREAM " << endl;

	  // reset counters
	  bytesleftold = bytesleftnew;
	  receivedbytes = 0;
	  receivedwords = 0;

	  // swap when DMA is complete
	  if(jseb2->HasDMACompleted())
	    {
	       if (debugDma) {
		 cout << "jseb2:  DMA COMPLETED   "   << endl;
	       }
	      if(jseb2->GetBytesLeftToDMA() == 0)
		{
		  //cout << "Complete bit set, NO words left to DMA - set bufferok to false" << endl;
		  bufferOkay = false;
		  DMAcomplete = true;
		}
	      else
		{
		  //cout << "Complete bit set, but words left to DMA" << endl;

		  jseb2->SetFree();

		  return;
		}
	    }
	  if (debugDma) {
	    cout << "jseb2:  first pass done bytesleftold  = " << bytesleftold << " bytesleftnew = "<< bytesleftnew  << endl;
	  }
 	  // cycle more often for testing...
 	  //if(bytesleftnew < 0.25*nbytes) // if less than 25% space remains switch buffers
 	  //  {
 	  //    bufferOkay = false;
 	  //  }

 	  // cycle more often for testing...
 	  //if(iprint > 5) 
 	  //   {
 	  //     iprint = 0;
 	  //     bufferOkay = false;
 	  //   }

	  firstpass = false;
	  if (forcexit)
	    {
	      cout << "Force exitRequested = true" << endl;
	      exitRequested = true;
	    }
	  if(exitRequested)
	    {
	      //printWords = true;
	      break;
	    }
	  if (debugDma) {

	    cout << "LINE " << __LINE__ << " end bufferokay loop " << endl;
	  }

	} // bufferokay

      if(exitRequested) 
	{
	  //printWords = true;
	  break;
	}
    } // while true

  //----------------//
  // Stop final DMA //
  //----------------//

  // record progress on previous DMA
  bytesleftnew = jseb2->GetBytesLeftToDMA();

  if (debugDma)
    {
      cout << "jseb2: stop final DMA bytesleftnew = " << bytesleftnew  << " bytesleftold = " << bytesleftold<< endl;
    }
  // abort the previous DMA
  jseb2->AbortDMA();

  // sync the processor cache to system memory
  WDC_DMASyncIo(pDma[idma]);

  // print any additional DMAed words since last print...
  if(bytesleftnew != bytesleftold)
    {
      receivedbytes = bytesleftold-bytesleftnew;
      receivedwords = receivedbytes/sizeof(UINT32);
      if (debugDma)
	{
	  cout << "jseb2: ADDITIONAL DMAed words bytes left receivedwords  = " << receivedwords << endl;
	}
      // print out new words...
      if(printWords)
	{
	  cout << "   (#" << idma << ") ";
	  for(unsigned int iword = 1; iword < receivedwords+1; iword++)
	    {
	      UINT32 u32Data = *pwords[idma]++;
	      cout.width(8); cout.fill('0'); cout << hex << u32Data << dec << " ";
	      if(iword%8 == 0)
		{
		  cout << endl; 
		  if(iword < receivedwords) cout << "   (#" << idma << ") ";
		}
	    }
	}
      else
	{
	  for(unsigned int iword = 1; iword < receivedwords+1; iword++)
	    {
	      *pwords[idma]++;
	    }
	}

      // check for new frame word...
      if(printFrames || writeFrames)
	{
	  while(pframe < pwords[idma])
	    {
	      if(nFrameWords == 0)
		{
		  nFrameWords = *(pframe);
		}
	      if (debugDma) {
		cout << "jseb2: new frame found " << endl;
	      }
	      // see if frame has been fully DMAed
	      if(pframe + nFrameWords - ioffset < pwords[idma])
		{
		  // put frame into array
		  for(unsigned int iword = ioffset; iword < nFrameWords; iword++)
		    {
		      frameWords[iword] = *(pframe++);
		    }
	      
		  // the first frame will be an empty partitioner frame
		  if(iframe == 0)
		    {
		      if(writeFrames)
			{
			  filebuffer->StartEvent();
			}

		      unsigned int oldFramesPerEvent = nFramesPerEvent;
		      nFramesPerEvent = (frameWords[3] >> 16);
		      if(nFramesPerEvent == 0) nFramesPerEvent = oldFramesPerEvent;
		      if(printFrames) 
			{
			  cout << "   PART3 Frame0: ";
			  for(unsigned int iword = 0; iword < 8; iword++)
			    {
			      cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
			    }
			  cout << endl;
			} // printFrames
		      if(frameWords[0] != 0x8)
			{
			  cout << "Sync frame has incorrect length" << endl;
			  exitRequested = true;
			  failDetected = true;
			  break;
			}

		      if(frameWords[1] != 0xFFFFFF00)
			{
			  cout << "Corrupt or missing frame marker" << endl;
			  exitRequested = true; printWords = true;
			  failDetected = true;
			  break;
			}
		    } // iframe == 0
		  // additional frames will be frome each dcm2 board
		  else
		    {
		      if(printFrames) 
			{
			  cout << "   DCM2 Frame::  ";
			  for(unsigned int iword = 0; iword < 8; iword++)
			    {
			      cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
			    }
			  cout << "... ";
			  cout.width(8); cout.fill('0'); cout << hex << frameWords[nFrameWords-1] << dec << endl;
			} // printFrames
		      if(frameWords[1] != 0xFFFFFF00)
			{
			  cout << "Corrupt or missing frame marker" << endl;
			  failDetected = true;
			  exitRequested = true; printWords = true;
			  break;
			}

		      if(writeFrames)
			{
			  // get event number
			  UINT32 prev_event_number = event_number;
			  event_number = frameWords[6];
			  if(event_number < prev_event_number)
			    {
			      event_offset += prev_event_number;

			      if(event_number == 0) event_offset++;
			    }
				      
			  // set event number
			  filebuffer->SetEventNumber(event_number+event_offset);

			  // add frame to file buffer
			  filebuffer->CopyFrameIntoBuffer(nFrameWords,pframeWords);
			}
		    }

		  ioffset = 0;
		  nFrameWords = 0;
		  iframe++;

		  // end the event when all frames are collected
		  if(iframe > nFramesPerEvent)
		    {
		      if(writeFrames)
			{
			  filebuffer->EndEvent();
			  filebuffer->CheckBufferSpace();
			}
		      iframe = 0;
		    }
		}
	      else // frame is not fully DMAed
		{
		  // put remaining frame fragment into array
		  UINT32 frame_write = ioffset;
		  UINT32 frame_end = pwords[idma] - pframe;
		  for(unsigned int iword = frame_write; iword < frame_end; iword++)
		    {
		      frameWords[iword] = *(pframe++);
		      ioffset++;
		    }
		}

	      if(exitRequested) 
		{
		  //printWords = true;
		  break;
		}
	    }
	} // printFrames || writeFrames

      // go back to waiting for new data
      bytesleftold = bytesleftnew;
      receivedbytes = 0;
      receivedwords = 0;
  if (debugDma)
    {
      cout << "jseb2:  wait for more data " << endl;
    }
    } // bytesleftnew != bytesleftold
	  
  
  if (debugDma)
    {
      cout << "jseb2:  bytesThroughOptics = " << bytesThroughOptics  << " nbytes " << nbytes << " bytesleftrcvr " << bytesleftrcvr << endl;
      cout << "jseb2: bytesleftonJSEB2 = " << bytesLeftOnJSEB2 << endl;
    }
  // update counter for bytes into optics
  status = jseb2->GetReceiverStatusWord(1);
  bytesleftrcvr = (status & 0x0FFFFFFF);
  bytesThroughOptics += (nbytes - bytesleftrcvr);

  if (debugDma)
    {
      cout << "jseb2:   bytesleftrcvr " << bytesleftrcvr  << " status = " << hex << status << dec << endl;
      cout << "jseb2:   bytesThroughOptics = " << bytesThroughOptics  << " bytesleftnew = " << bytesleftnew << endl;
    }

  // update counter for bytes out on DMA
  bytesThroughDMA += (nbytes - bytesleftnew);
  if (debugDma)
    {
  cout << endl;
  cout << "   |==========================================================================|" << endl;
  cout << "   |   Final DMA complete, spilling out any remaining words stored on FIFO    |" << endl;
  cout << "   |==========================================================================|" << endl;
    }
  //cout << " Currently ignoring final event due to FIFO extraction issue" << endl;
  if (debugDma)
    {
      cout << "jseb2: bytesleftonJSEB2 = " << bytesLeftOnJSEB2 << " bytesThroughOptics = " << bytesThroughOptics  << " bytesThroughDMA = " << bytesThroughDMA << endl;
    }
 UINT64 lastbytesThroughDMA = bytesThroughDMA;
  bytesLeftOnJSEB2 += bytesThroughOptics - bytesThroughDMA;
  bytesThroughOptics = 0;
  bytesThroughDMA = 0;
  if (debugDma)
    {
      cout << "jseb2: bytesleftonJSEB2 = " << bytesLeftOnJSEB2 << " bytesThroughOptics = " << bytesThroughOptics << " bytesThroughDMA = " << bytesThroughDMA << " nbytes = " << nbytes << " bytesleftrcvr = " << bytesleftrcvr << " bytesleftnew = " << bytesleftnew << endl;
    }
  // ejd 11/21/16 should this be ( bytesLeftOnJSEB2 > 0 ) ; here bytesleftonJSEB2 is < 0
  //if(bytesLeftOnJSEB2 != 0)
  if(bytesLeftOnJSEB2 > 0 && bytesLeftOnJSEB2 < lastbytesThroughDMA)
    {
      // storage for longs on FIFO
      UINT64 longsTRX1[1025] = {0xbad};
      UINT64 *plongsTRX1 = (UINT64*)&longsTRX1;

      UINT64 longsTRX2[1025] = {0xbad};
      UINT64 *plongsTRX2 = (UINT64*)&longsTRX2;

      // storage for words on FIFO
      unsigned int nwordsOnJSEB2 = bytesLeftOnJSEB2/4;

      UINT32 wordsOnJSEB2[2049] = {0xbad};
      unsigned int nlongsOnJSEB2 = nwordsOnJSEB2/2;
      if(nwordsOnJSEB2%2 != 0)
	{
	  nlongsOnJSEB2++;
	}
      
      // Read back FIFO
      jseb2->SetUseDMA(false);
      jseb2->Read(Dcm2_JSEB2::RCV_ONE,nlongsOnJSEB2,plongsTRX1);
      jseb2->Read(Dcm2_JSEB2::RCV_TWO,nlongsOnJSEB2,plongsTRX2);
      jseb2->SetUseDMA(true);

      for(unsigned int ilong = 0; ilong < nlongsOnJSEB2; ilong++)
	{
	  //cout << "TRX1 & TRX2: ";
	  //cout.width(16); cout.fill('0'); cout << hex << longsTRX1[ilong] << dec << " ";
	  //cout.width(16); cout.fill('0'); cout << hex << longsTRX2[ilong] << dec << endl;

	  UINT64 byteA = ((longsTRX1[ilong] & 0x0000FFFF00000000) >> 32);
	  UINT64 byteB = (longsTRX1[ilong] & 0x000000000000FFFF);
	  UINT64 byteC = ((longsTRX2[ilong] & 0xFFFF000000000000) >> 48);
	  UINT64 byteD = ((longsTRX2[ilong] & 0x0000FFFF00000000) >> 32);
	  
	  UINT32 word_odd  = ((byteD << 16) + byteB);
	  UINT32 word_even = ((byteC << 16) + byteA);
	  
	  wordsOnJSEB2[2*ilong] = word_odd;
	  wordsOnJSEB2[2*ilong+1] = word_even;	  
	}

      // print out new words...
      if(printWords)
	{
	  cout << "   (--) ";
	  for(unsigned int iword = 1; iword < nwordsOnJSEB2+1; iword++)
	    {
	      cout.width(8); cout.fill('0'); cout << hex << wordsOnJSEB2[iword-1] << dec << " ";
	      if(iword%8 == 0)
		{
		  cout << endl; 
		  if(iword < nwordsOnJSEB2) cout << "   (--) ";
		}
	    }
	  cout << endl;
	}
      
      if(printFrames || writeFrames)
	{
	  for(unsigned int iword = 1; iword < nwordsOnJSEB2+1; iword++)
	    {
	      frameWords[iword+ioffset-1] = wordsOnJSEB2[iword-1];
	    }

	  // the first frame will be an empty partitioner frame
	  if(iframe == 0)
	    {
	      if(writeFrames)
		{
		  //filebuffer->StartEvent();
		}

	      unsigned int oldFramesPerEvent = nFramesPerEvent;
	      nFramesPerEvent = (frameWords[3] >> 16);
	      if(nFramesPerEvent == 0) nFramesPerEvent = oldFramesPerEvent;

	      if (debugEventReadout) {
	      cout << "   PART3 Final Frame: ";
	      for(unsigned int iword = 0; iword < 8; iword++)
		{
		  cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
		}
	      cout << endl;
	      } // debugDma

	      /* DONT CHECK SO DATA GETS WRITTEN OUT
	      if(frameWords[0] != 0x8)
		{
		  cout << "Sync frame has incorrect length" << endl;
		  exitRequested = true;
		  failDetected = true;
		}

	      if(frameWords[1] != 0xFFFFFF00)
		{
		  cout << "Corrupt or missing frame marker" << endl;
		  exitRequested = true;
		  failDetected = true;
		}
	      */
	    }
	  // additional frames will be frome each dcm2 board
	  else
	    {
	      if (debugEventReadout && (nFrameWords > 0 ))
		{
		  cout << "   DCM2 Frame:__  ";
		  for(unsigned int iword = 0; iword < 8; iword++)
		    {
		      cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
		    }
		  cout << "... ";
		  cout.width(8); cout.fill('0'); cout << hex << frameWords[nFrameWords-1] << dec << endl;
		} // debug printout additional frames

	      /** DON'T CHECK	  
	      if(frameWords[1] != 0xFFFFFF00)
		{
		  cout << "Corrupt or missing frame marker" << endl;
		  failDetected = true;
		  exitRequested = true;
		}
**/
	      if(writeFrames)
		{
		  // get event number
		  //filebuffer->SetEventNumber(frameWords[6]);

		  // add frame to file buffer
		  //filebuffer->CopyFrameIntoBuffer(nFrameWords,pframeWords);
		}
	    }

	  ioffset = 0;
	  nFrameWords = 0;
	  iframe++;

	  // end the event when all frames are collected
	  if(iframe > nFramesPerEvent)
	    {
	      if(writeFrames)
		{
		  //filebuffer->EndEvent();
		  //filebuffer->CheckBufferSpace();
		}
	      iframe = 0;
	    }
	}
    }

  if(writeFrames)
    {
      cout << "   Closing Output File" << endl;
      if(!filebuffer->CloseFile())
	{
	  cerr << "   Failed to close file: " << filename << endl;
	  delete filebuffer;
	  return;
	}
    }

  cout << endl;
  cout << "   Releasing DMA buffers" << endl;
  
  // Release DMA buffers
  for(unsigned int idma = 0; idma < nbuffers; idma++)
    {
      WDC_DMABufUnlock(pDma[idma]);
    }
  
  cout << "   Readout Copmlete" << endl;

  jseb2->SetFree();
  
  return;
} // jseb2_seb_reader


void jseb2Controller::jseb2_seb_reader2(Dcm2_JSEB2 *jseb2, bool printWords, bool printFrames, bool writeFrames)
{
  bool failDetected = false;
  bool printFrameInfo = true;
  bool debugDma = false;
  cout << endl;
  if(printWords)
    {
      cout << "   jseb2Controller::Beginning SEB Receiver Read Words to Screen..." << endl;
    }
  if(printFrames)
    {
      cout << "   jseb2Controller::Beginning SEB Receiver Read Frames to Screen..." << endl;
    }
  if(writeFrames)
    {
      cout << "   jseb2Controller::Beginning SEB Receiver Read Frames to File..." << endl;
    }

  jseb2->SetReceiverDefaults();

  unsigned int rcv = 3;
  int MAX_PRINT_ERRORS = 20;
  int frame_errors_printed = 0;
 

  if(writeFrames)
    {
      //cout << "   Please enter the name of an output file:" << endl;
      cout << "jseb2Controller: Using output data file: " << outputfilename << endl;
      //scanf("%s",outputfilename);

      //cout << "   Please select the desired number of events (0 for unlimited):" << endl;
      //scanf("%lu",&nevents);
      cout << "jseb2Controller: Collecting  " << nevents << " events " << endl;
    }

  // file output
  Dcm2_FileBuffer *filebuffer = new Dcm2_FileBuffer();
  string filename(outputfilename);
  unsigned long ievent = 0;
  UINT32 event_offset = 0;
  UINT32 event_number = 0;
  if(writeFrames)
    {
      filebuffer->SetVerbosity(Dcm2_FileBuffer::LOG);
      filebuffer->SetRunNumber(0xdcb25eb);
      if(!filebuffer->OpenFile(filename))
	{ 
	  cerr << "   Failed to open output file: " << filename << endl;
	  delete filebuffer;
	  return;
	}
    }

  // Frame storage
  UINT32 *pframe = NULL; // beginning of frame in dma buffer
  unsigned int iframe = 0;
  unsigned int ioffset = 0;
  unsigned int nFrameWords = 0;
  unsigned int nFramesPerEvent = 0;
  const unsigned int maxFrameWords = 8*Dcm2_FileBuffer::MAX_WORDS_PER_PACKET;
  UINT32 frameWords[maxFrameWords];
  UINT32 *pframeWords = (UINT32*)&frameWords;

  // DMA buffer settings
  const unsigned int nbuffers = 2;
  UINT32 nbytes = 4*1024*1024; // 4 MB buffers
  DWORD dwDMABufSize = nbytes;

  PVOID pbuff[nbuffers];
  WD_DMA *pDma[nbuffers];   
  DWORD dwOptions = DMA_FROM_DEVICE | DMA_ALLOW_64BIT_ADDRESS; // required to prevent system meltdowns on over-allocation

  cout << "   Allocating DMA buffers" << endl;

  // Get DMA buffers
  UINT32 *pwords[nbuffers];
  for(unsigned int idma = 0; idma < nbuffers; idma++)
    {
      WDC_DMAContigBufLock(jseb2->GetHandle(), &pbuff[idma], dwOptions, dwDMABufSize, &pDma[idma]);
      pwords[idma] = static_cast<UINT32*>(pbuff[idma]);
    }

  // Prepare for operation
  jseb2->EnableHolds(0x2000);       // FIFO fills to 16kB then TRX exerts hold
  jseb2->AbortDMA();                // Ends previous DMA
  jseb2->ClearReceiverFIFO(rcv);    // Flushes FIFOs
  jseb2->SetUseLock(true);          // Lock the JSEB2 use during DMAs
  
  if(!jseb2->WaitForLock(5))
    {
      cout << "  Unable to lock JSEB2, attempting cleanup..." << endl;

      jseb2->CleanUp(Dcm2_JSEB2::RCV_BOTH);

      if(!jseb2->WaitForLock(5))
	{
	  cout << "  Still unable to lock JSEB2, exiting..." << endl;
      
	  return;
	}
    }
  cout << "   Locked JSEB2 for use" << endl;

  bool DMAcomplete = false;

  UINT32 bytesleftold = nbytes;
  UINT32 bytesleftnew = nbytes;
  UINT32 receivedbytes = bytesleftold-bytesleftnew;
  UINT32 receivedwords = receivedbytes/sizeof(UINT32);

  UINT32 status = 0x0;
  UINT32 bytesleftrcvr = 0;

  unsigned int idma = nbuffers-1;
  bool firstpass = true;

  UINT64 bytesThroughOptics = 0;
  UINT64 bytesThroughDMA = 0; 
  UINT64 bytesLeftOnJSEB2 = 0;

  while(true)
    {
      // calculate bytes through optics
      if(firstpass)
	{
	  bytesThroughOptics = 0;
	  bytesThroughDMA = 0;
	}
      else // not first pass
	{

	  if(!DMAcomplete)
	    {
	      // record progress on previous DMA
	      bytesleftnew = jseb2->GetBytesLeftToDMA();

	      // abort the previous DMA
	      jseb2->AbortDMA();

	      // sync the processor cache to system memory
	      WDC_DMASyncIo(pDma[idma]);
	    }
	  else
	    {
	      // record progress on previous DMA
	      bytesleftnew = 0;

	      // sync the processor cache to system memory
	      WDC_DMASyncIo(pDma[idma]);
	    }
	  
	  // update counter for bytes out on DMA
	  bytesThroughDMA += (nbytes - bytesleftnew);
	  if (printFrameInfo)
	    cout << "bytes out of DMA = " << bytesThroughDMA << endl;
	  // print any additional DMAed words since last print...
	  if(bytesleftnew != bytesleftold)
	    {
	      receivedbytes = bytesleftold-bytesleftnew;
	      receivedwords = receivedbytes/sizeof(UINT32);
	  
	      //---BEGIN PARSING DATA STREAM------------------------
	      if (printFrameInfo)
		cout << "frame: receivedwords = " << receivedwords << endl;
	      // print out new words and/or advance dma progress pointer
	      if(printWords)
		{
		  cout << "   (#" << idma << ") ";
		  for(unsigned int iword = 1; iword < receivedwords+1; iword++)
		    {
		      UINT32 u32Data = *pwords[idma];
		      pwords[idma]++;
		      cout.width(8); cout.fill('0'); cout << hex << u32Data << dec << " ";
		      if(iword%8 == 0)
			{
			  cout << endl; 
			  if(iword < receivedwords) cout << "   (#" << idma << ") ";
			}
		    }
		}
	      else // just advance pointer
		{
		  for(unsigned int iword = 1; iword < receivedwords+1; iword++)
		    {
		      pwords[idma]++;
		    }
		}

	      // parse new data for frames
	      if(printFrames || writeFrames)
		{
		  // check for new frame word...
		  while(pframe < pwords[idma])
		    {	
		      // copy out length of new frame
		      if(nFrameWords == 0)
			{
			  nFrameWords = *pframe;
			}

		      // see if this frame has been fully DMAed
		      UINT32 DMAprogress = pwords[idma] - pframe; // number of words
		      if(nFrameWords - ioffset < DMAprogress)
			{
			  // put the remainder of this frame into array
			  for(unsigned int iword = ioffset; iword < nFrameWords; iword++)
			    {
			      frameWords[iword] = *pframe;
			      pframe++;
			    }

			  // the first frame will be an empty partitioner frame
			  if(iframe == 0)
			    {
			      if (printFrameInfo)
				cout << "got first frame " << endl;
			      if(writeFrames)
				{
				  filebuffer->StartEvent();
				}

			      unsigned int oldFramesPerEvent = nFramesPerEvent;
			      nFramesPerEvent = (frameWords[3] >> 16);
			      if(nFramesPerEvent == 0) nFramesPerEvent = oldFramesPerEvent;

			      if(printFrames)
				{
				  cout << endl;
				  cout << "   PART3 Frame: ";
				  for(unsigned int iword = 0; iword < 8; iword++)
				    {
				      cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
				    }
				  cout << endl;
				}
			  
			      if(frameWords[0] != 0x8)
				{
				  cout << "Sync frame has incorrect length" << endl;
				  exitRequested = true; printWords = true;
				  failDetected = true;
				  break;
				}

			      if(frameWords[1] != 0xFFFFFF00)
				{
				  cout << "Corrupt or missing frame marker" << endl;
			  
				  failDetected = true;
				  exitRequested = true; printWords = true;
				  break;
				}
			    }
			  // additional frames will be from each dcm2 board
			  else
			    {
			      if (printFrameInfo)
				cout << "read additional frames " << iframe << endl;

			      // coding around Chi's extra sync frame appearances
			      if(frameWords[0] != 0x8)
				{
				  if(printFrames)
				    {
				      cout << "   DCM2 Frame:  ";
				      for(unsigned int iword = 0; iword < 8; iword++)
					{
					  cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
					}
				      cout << "... ";
				      cout.width(8); cout.fill('0'); cout << hex << frameWords[nFrameWords-1] << dec;
				      cout << endl;
				    }

				  if(frameWords[1] != 0xFFFFFF00)
				    {
				      cout << "Corrupt or missing frame marker" << endl;
				  
				      failDetected = true;
				      exitRequested = true; printWords = true;
				      break;
				    }

				  if(writeFrames)
				    {
				      // get event number
				      UINT32 prev_event_number = event_number;
				      event_number = frameWords[6];
				      if(event_number < prev_event_number)
					{
					  event_offset += prev_event_number;

					  if(event_number == 0) event_offset++;
					}
				      
				      // set event number
				      filebuffer->SetEventNumber(event_number+event_offset);

				      // add frame to file buffer
				      filebuffer->CopyFrameIntoBuffer(nFrameWords,pframeWords);
				    }
				}
			      else
				{
				  frame_errors_printed++;
				  if (frame_errors_printed < MAX_PRINT_ERRORS)
				    cout << "---Skipping spurious sync frame---" << frame_errors_printed << endl;
				  iframe--;
				}
			    }

			  ioffset = 0;
			  nFrameWords = 0;
			  iframe++;

			  // end the event when all frames are collected
			  if(iframe > nFramesPerEvent)
			    {
			      if(writeFrames)
				{
				  filebuffer->EndEvent();
				  filebuffer->CheckBufferSpace();

				  ievent++;

				  if(nevents != 0)
				    {
				      if(ievent > nevents)
					{
					  exitRequested = true; 
					}
				    }

				  // print out procress
				  if(ievent < 5)
				    {
				      cout << "   SEB Readback Events Written = " << ievent << ", Event Number = " << event_number+event_offset << endl;
				    }
				  else
				    {
				      if(ievent%100 == 0) cout << "   SEB Readback Events Written = " << ievent << ", Event Number = " << event_number+event_offset << endl;
				    }
				  if(ievent == 5) cout << "   Progress will now be updated every 100 events" << endl;

				}
			      iframe = 0;
			    }
			}
		      else // frame is not fully DMAed
			{
			  // put remaining frame fragment into array
			  UINT32 frag_begin = ioffset;
			  UINT32 frag_size = (pwords[idma] - pframe); // gives number of words, not bytes
			  if (printFrameInfo)
			    cout << "frame not fully DMAed " << endl;
			  for(unsigned int iword = 0; iword < frag_size; iword++)
			    {
			      frameWords[iword+frag_begin] = *pframe;
			      pframe++;
			      ioffset++;
			    }
			}

		      if(exitRequested)
			{
			  printWords = true;
			  break;
			}
		    } // pframe < pwords[idma]
		}

	      //---FINISHED PARSING DATA STREAM------------------------

	      // go back to waiting for new data
	      bytesleftold = bytesleftnew;
	      receivedbytes = 0;
	      receivedwords = 0;
	    } // bytesleftnew != bytesleftold

	  if(failDetected) break;

	  // stress hold-busy chain during swap
	  //cout << "----pause started (dma swap)----" << endl;
	  //sleep(2);
	  //cout << "----pause complete (dma swap)----" << endl;
	} // not first pass

      // round robin on DMA buffers
      idma++;
      if(idma == nbuffers)
	{
	  idma = 0;
	}

      // Reset Pointer (needed?)
      //pwords[idma] = static_cast<UINT32*>(pbuff[idma]);

      if(!firstpass)
	{
	  // update counter for bytes into optics
	  status = jseb2->GetReceiverStatusWord(1);
	}
      //if (printFrameInfo)
      //cout << "START new DMA " << endl;

      // start new receive counter & DMA immediately
      jseb2->SetReceiveLength(rcv,nbytes);   // this way we don't kill the FIFO data
      jseb2->Receive(rcv,nbytes,pDma[idma]); // start new data stream
      DMAcomplete = false;

      // Reset pointers
      pwords[idma] = static_cast<UINT32*>(pbuff[idma]);
      pframe = pwords[idma];

      // now calculate byte counts
      if(!firstpass)
	{
	  bytesleftrcvr = (status & 0x0FFFFFFF);
	  bytesThroughOptics += (nbytes - bytesleftrcvr);

	  bytesLeftOnJSEB2 += bytesThroughOptics - bytesThroughDMA;
	  bytesThroughOptics = 0;
	  bytesThroughDMA = 0;
	}

      // Remaining DMA length
      bytesleftold = nbytes;
      bytesleftnew = nbytes;
      receivedbytes = bytesleftold-bytesleftnew;
      receivedwords = receivedbytes/sizeof(UINT32);

      // New DMA has begun (tell user to start data stream)
      if (debugDma)
	{
      cout << endl;
      cout << "   |==========================================================================|" << endl;
      cout << "   |   New DMA is running on Buffer #"<< idma << " and JSEB2 is ready to receive words    |" << endl;
      cout << "   |==========================================================================|" << endl;
	}
      bool bufferOkay = true;
      unsigned int iprint = 0;
      while(bufferOkay)
	{
	  // Wait for data
	  bool waitingOnData = true;
	  while(waitingOnData)
	    {
	      WDC_Sleep(100, WDC_SLEEP_BUSY);
	      
	      bytesleftnew = jseb2->GetBytesLeftToDMA();

	      if(bytesleftnew != bytesleftold)
		{
		  waitingOnData = false;
		}

	      if(exitRequested)
		{
		  //printWords = true;
		  break;
		}
	    }

	  receivedbytes = bytesleftold-bytesleftnew;
	  receivedwords = receivedbytes/sizeof(UINT32);

	  //---BEGIN PARSING DATA STREAM------------------------

	  // print out new words and/or advance dma progress pointer
	  if(printWords)
	    {
	      cout << "   (#" << idma << ") ";
	      for(unsigned int iword = 1; iword < receivedwords+1; iword++)
		{
		  UINT32 u32Data = *pwords[idma];
		  pwords[idma]++;
		  cout.width(8); cout.fill('0'); cout << hex << u32Data << dec << " ";
		  if(iword%8 == 0)
		    {
		      cout << endl; 
		      if(iword < receivedwords) cout << "   (#" << idma << ") ";
		    }
		}
	      iprint++;
	    }
	  else // just advance pointer
	    {
	      for(unsigned int iword = 1; iword < receivedwords+1; iword++)
		{
		  pwords[idma]++;
		}
	      iprint++;
	    }

	  // parse new data for frames
	  if(printFrames || writeFrames)
	    {
	      // check for new frame word...
	      while(pframe < pwords[idma])
		{	
		  // copy out length of new frame
		  if(nFrameWords == 0)
		    {
		      nFrameWords = *pframe;
		    }

		  // see if this frame has been fully DMAed
		  UINT32 DMAprogress = pwords[idma] - pframe; // number of words
		   if (printFrameInfo)
		     cout << "Check if frame fully DMAed nFrameWords= " << nFrameWords << " offset " << ioffset << " DMAprogress  " << DMAprogress << endl;

		  if(nFrameWords - ioffset < DMAprogress)
		    {
		      // put the remainder of this frame into array
		      for(unsigned int iword = ioffset; iword < nFrameWords; iword++)
			{
			  frameWords[iword] = *pframe;
			  pframe++;
			}

		      // the first frame will be an empty partitioner frame
		      if(iframe == 0)
			{
			   if (printFrameInfo)
			     cout << "First Frame " << endl;
			  if(writeFrames)
			    {
			      filebuffer->StartEvent();
			    }

			  unsigned int oldFramesPerEvent = nFramesPerEvent;
			  nFramesPerEvent = (frameWords[3] >> 16);
			  if(nFramesPerEvent == 0) nFramesPerEvent = oldFramesPerEvent;

			  if(printFrames)
			    {
			      cout << endl;
			      cout << "   PART3 Frame: ";
			      for(unsigned int iword = 0; iword < 8; iword++)
				{
				  cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
				}
			      cout << endl;
			    }
			  
			  if(frameWords[0] != 0x8)
			    {
			      cout << "Sync frame has incorrect length" << endl;
			      exitRequested = true; printWords = true;
			      failDetected = true;
			      break;
			    }

			  if(frameWords[1] != 0xFFFFFF00)
			    {
			      cout << "Corrupt or missing frame marker" << endl;
			  
			      failDetected = true;
			      exitRequested = true; printWords = true;
			      break;
			    }
			}
		      // additional frames will be from each dcm2 board
		      else
			{
			  if (printFrameInfo)
			    cout << "Frame Data from each dcm2 " << hex << frameWords[0] << dec << endl;
			  // coding around Chi's extra sync frame appearances
			  if(frameWords[0] != 0x8)
			    {
			      if(printFrames)
				{
				  cout << "   DCM2 Frame:  ";
				  for(unsigned int iword = 0; iword < 8; iword++)
				    {
				      cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
				    }
				  cout << "... ";
				  cout.width(8); cout.fill('0'); cout << hex << frameWords[nFrameWords-1] << dec;
				  cout << endl;
				}

			      if(frameWords[1] != 0xFFFFFF00)
				{
				  cout << "Corrupt or missing frame marker" << endl;
				  
				  failDetected = true;
				  exitRequested = true; printWords = true;
				  break;
				}

			      if(writeFrames)
				{
				  // get event number
				  UINT32 prev_event_number = event_number;
				  event_number = frameWords[6];
				  if(event_number < prev_event_number)
				    {
				      event_offset += prev_event_number;

				      if(event_number == 0) event_offset++;
				    }
				      
				  // set event number
				  filebuffer->SetEventNumber(event_number+event_offset);

				  // add frame to file buffer
				  filebuffer->CopyFrameIntoBuffer(nFrameWords,pframeWords);
				}
			    }
			  else
			    {
			      frame_errors_printed++;
			       if (frame_errors_printed < MAX_PRINT_ERRORS)
				 cout << "---Skipping spurious sync frame--- now = " << frame_errors_printed  << endl;
			      iframe--;
			    }
			}

		      ioffset = 0;
		      nFrameWords = 0;
		      iframe++;

		      // end the event when all frames are collected
		      if(iframe > nFramesPerEvent)
			{
			  if(writeFrames)
			    {
			      filebuffer->EndEvent();
			      filebuffer->CheckBufferSpace();

			      ievent++;
			      
			      if(nevents != 0)
				{
				  if(ievent > nevents)
				    {
				      exitRequested = true; 
				    }
				}

			      // print out procress
			      if(ievent < 5)
				{
				  cout << "LINE " << __LINE__ << "  Readback Events Written = " << ievent << ", Event Number = " << event_number+event_offset << endl;
				}
			      else
				{
				  if(ievent%100 == 0) cout << "LINE " << __LINE__ << "  Readback Events Written = " << ievent << ", Event Number = " << event_number+event_offset << endl;
				}
			      if(ievent == 5) cout << "   Progress will now be updated every 100 events" << endl;

			    }
			  iframe = 0;
			}
		    }
		  else // frame is not fully DMAed
		    {
		      // put remaining frame fragment into array
		      UINT32 frag_begin = ioffset;
		      UINT32 frag_size = (pwords[idma] - pframe); // gives number of words, not bytes

		      for(unsigned int iword = 0; iword < frag_size; iword++)
			{
			  frameWords[iword+frag_begin] = *pframe;
			  pframe++;
			  ioffset++;
			}
		    }

		  if(exitRequested)
		    {
		      cout << "EXIT REQUESTED " << endl;
		      printWords = true;
		      break;
		    }
		}
	    }

	  //---FINISHED PARSING DATA STREAM------------------------
	  if (printFrameInfo)
	    cout << "Done PARSING DATA STREAM " << endl;

	  // reset counters
	  bytesleftold = bytesleftnew;
	  receivedbytes = 0;
	  receivedwords = 0;

	  // swap when DMA is complete
	  if(jseb2->HasDMACompleted())
	    {
	      if(jseb2->GetBytesLeftToDMA() == 0)
		{
		  bufferOkay = false;
		  DMAcomplete = true;
		}
	      else
		{
		  cout << "Complete bit set, but words left to DMA" << endl;

		  jseb2->SetFree();

		  return;
		}
	    }
	  
 	  // cycle more often for testing...
 	  //if(bytesleftnew < 0.25*nbytes) // if less than 25% space remains switch buffers
 	  //  {
 	  //    bufferOkay = false;
 	  //  }

 	  // cycle more often for testing...
 	  //if(iprint > 5) 
 	  //   {
 	  //     iprint = 0;
 	  //     bufferOkay = false;
 	  //   }

	  firstpass = false;

	  if(exitRequested)
	    {
	      if (printFrameInfo)
		cout << "ExitRequested received  " << endl;
	      printWords = true;
	      break;
	    }
	} // bufferOkay

      if(exitRequested) 
	{
	  if (printFrameInfo)
		cout << "ExitRequested exit while  " << endl;
	  printWords = true;
	  break;
	}
    } // while(true)

  //----------------//
  // Stop final DMA //
  //----------------//

  // record progress on previous DMA
  bytesleftnew = jseb2->GetBytesLeftToDMA();

  // abort the previous DMA
  jseb2->AbortDMA();

  // sync the processor cache to system memory
  WDC_DMASyncIo(pDma[idma]);

  if (printFrameInfo)
    cout << "Stopping the DMA " << endl;

  // print any additional DMAed words since last print...
  if(bytesleftnew != bytesleftold)
    {
      receivedbytes = bytesleftold-bytesleftnew;
      receivedwords = receivedbytes/sizeof(UINT32);
	  
      // print out new words...
      if(printWords)
	{
	  cout << "   (#" << idma << ") ";
	  for(unsigned int iword = 1; iword < receivedwords+1; iword++)
	    {
	      UINT32 u32Data = *pwords[idma]++;
	      cout.width(8); cout.fill('0'); cout << hex << u32Data << dec << " ";
	      if(iword%8 == 0)
		{
		  cout << endl; 
		  if(iword < receivedwords) cout << "   (#" << idma << ") ";
		}
	    }
	}
      else
	{
	  for(unsigned int iword = 1; iword < receivedwords+1; iword++)
	    {
	      *pwords[idma]++;
	    }
	}

      // check for new frame word...
      if(printFrames || writeFrames)
	{
	  while(pframe < pwords[idma])
	    {
	      if(nFrameWords == 0)
		{
		  nFrameWords = *(pframe);
		}
	  
	      // see if frame has been fully DMAed
	      if(pframe + nFrameWords - ioffset < pwords[idma])
		{
		  // put frame into array
		  for(unsigned int iword = ioffset; iword < nFrameWords; iword++)
		    {
		      frameWords[iword] = *(pframe++);
		    }
	      
		  // the first frame will be an empty partitioner frame
		  if(iframe == 0)
		    {
		      if(writeFrames)
			{
			  filebuffer->StartEvent();
			}

		      unsigned int oldFramesPerEvent = nFramesPerEvent;
		      nFramesPerEvent = (frameWords[3] >> 16);
		      if(nFramesPerEvent == 0) nFramesPerEvent = oldFramesPerEvent;

		      cout << "   PART3 Frame: ";
		      for(unsigned int iword = 0; iword < 8; iword++)
			{
			  cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
			}
		      cout << endl;
		  
		      if(frameWords[0] != 0x8)
			{
			  cout << "Sync frame has incorrect length" << endl;
			  exitRequested = true;
			  failDetected = true;
			  break;
			}

		      if(frameWords[1] != 0xFFFFFF00)
			{
			  cout << "Corrupt or missing frame marker" << endl;
			  exitRequested = true; printWords = true;
			  failDetected = true;
			  break;
			}
		    }
		  // additional frames will be frome each dcm2 board
		  else
		    {
		      cout << "   DCM2 Frame:  ";
		      for(unsigned int iword = 0; iword < 8; iword++)
			{
			  cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
			}
		      cout << "... ";
		      cout.width(8); cout.fill('0'); cout << hex << frameWords[nFrameWords-1] << dec << endl;
		  
		      if(frameWords[1] != 0xFFFFFF00)
			{
			  cout << "Corrupt or missing frame marker" << endl;
			  failDetected = true;
			  exitRequested = true; printWords = true;
			  break;
			}

		      if(writeFrames)
			{
			  // get event number
			  UINT32 prev_event_number = event_number;
			  event_number = frameWords[6];
			  if(event_number < prev_event_number)
			    {
			      event_offset += prev_event_number;

			      if(event_number == 0) event_offset++;
			    }
				      
			  // set event number
			  filebuffer->SetEventNumber(event_number+event_offset);

			  // add frame to file buffer
			  filebuffer->CopyFrameIntoBuffer(nFrameWords,pframeWords);
			}
		    }

		  ioffset = 0;
		  nFrameWords = 0;
		  iframe++;

		  // end the event when all frames are collected
		  if(iframe > nFramesPerEvent)
		    {
		      if(writeFrames)
			{
			  filebuffer->EndEvent();
			  filebuffer->CheckBufferSpace();
			}
		      iframe = 0;
		    }
		}
	      else // frame is not fully DMAed
		{
		  // put remaining frame fragment into array
		  UINT32 frame_write = ioffset;
		  UINT32 frame_end = pwords[idma] - pframe;
		  for(unsigned int iword = frame_write; iword < frame_end; iword++)
		    {
		      frameWords[iword] = *(pframe++);
		      ioffset++;
		    }
		}

	      if(exitRequested) 
		{
		  printWords = true;
		  break;
		}
	    }
	}

      // go back to waiting for new data
      bytesleftold = bytesleftnew;
      receivedbytes = 0;
      receivedwords = 0;
    } // bytesleftnew != bytesleftold
	  
  

  // update counter for bytes into optics
  status = jseb2->GetReceiverStatusWord(1);
  bytesleftrcvr = (status & 0x0FFFFFFF);
  bytesThroughOptics += (nbytes - bytesleftrcvr);

  // update counter for bytes out on DMA
  bytesThroughDMA += (nbytes - bytesleftnew);

  cout << endl;
  cout << "   |==========================================================================|" << endl;
  cout << "   |   Final DMA complete, spilling out any remaining words stored on FIFO    |" << endl;
  cout << "   |==========================================================================|" << endl;

  //cout << " Currently ignoring final event due to FIFO extraction issue" << endl;

  bytesLeftOnJSEB2 += bytesThroughOptics - bytesThroughDMA;
  bytesThroughOptics = 0;
  bytesThroughDMA = 0;

  if(bytesLeftOnJSEB2 != 0)
    {
      // storage for longs on FIFO
      UINT64 longsTRX1[1025] = {0xbad};
      UINT64 *plongsTRX1 = (UINT64*)&longsTRX1;

      UINT64 longsTRX2[1025] = {0xbad};
      UINT64 *plongsTRX2 = (UINT64*)&longsTRX2;

      // storage for words on FIFO
      unsigned int nwordsOnJSEB2 = bytesLeftOnJSEB2/4;

      UINT32 wordsOnJSEB2[2049] = {0xbad};
      unsigned int nlongsOnJSEB2 = nwordsOnJSEB2/2;
      if(nwordsOnJSEB2%2 != 0)
	{
	  nlongsOnJSEB2++;
	}
      
      // Read back FIFO
      jseb2->SetUseDMA(false);
      jseb2->Read(Dcm2_JSEB2::RCV_ONE,nlongsOnJSEB2,plongsTRX1);
      jseb2->Read(Dcm2_JSEB2::RCV_TWO,nlongsOnJSEB2,plongsTRX2);
      jseb2->SetUseDMA(true);

      for(unsigned int ilong = 0; ilong < nlongsOnJSEB2; ilong++)
	{
	  //cout << "TRX1 & TRX2: ";
	  //cout.width(16); cout.fill('0'); cout << hex << longsTRX1[ilong] << dec << " ";
	  //cout.width(16); cout.fill('0'); cout << hex << longsTRX2[ilong] << dec << endl;

	  UINT64 byteA = ((longsTRX1[ilong] & 0x0000FFFF00000000) >> 32);
	  UINT64 byteB = (longsTRX1[ilong] & 0x000000000000FFFF);
	  UINT64 byteC = ((longsTRX2[ilong] & 0xFFFF000000000000) >> 48);
	  UINT64 byteD = ((longsTRX2[ilong] & 0x0000FFFF00000000) >> 32);
	  
	  UINT32 word_odd  = ((byteD << 16) + byteB);
	  UINT32 word_even = ((byteC << 16) + byteA);
	  
	  wordsOnJSEB2[2*ilong] = word_odd;
	  wordsOnJSEB2[2*ilong+1] = word_even;	  
	}

      
      // print out new words...
      if(printWords)
	{
	  cout << "   (--) ";
	  for(unsigned int iword = 1; iword < nwordsOnJSEB2+1; iword++)
	    {
	      cout.width(8); cout.fill('0'); cout << hex << wordsOnJSEB2[iword-1] << dec << " ";
	      if(iword%8 == 0)
		{
		  cout << endl; 
		  if(iword < nwordsOnJSEB2) cout << "   (--) ";
		}
	    }
	  cout << endl;
	}
      
      if(printFrames || writeFrames)
	{
	  for(unsigned int iword = 1; iword < nwordsOnJSEB2+1; iword++)
	    {
	      frameWords[iword+ioffset-1] = wordsOnJSEB2[iword-1];
	    }

	  // the first frame will be an empty partitioner frame
	  if(iframe == 0)
	    {
	      if(writeFrames)
		{
		  //filebuffer->StartEvent();
		}

	      unsigned int oldFramesPerEvent = nFramesPerEvent;
	      nFramesPerEvent = (frameWords[3] >> 16);
	      if(nFramesPerEvent == 0) nFramesPerEvent = oldFramesPerEvent;
		  
	      cout << "   PART3 Frame: ";
	      for(unsigned int iword = 0; iword < 8; iword++)
		{
		  cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
		}
	      cout << endl;
		  
	      if(frameWords[0] != 0x8)
		{
		  cout << "Sync frame has incorrect length" << endl;
		  exitRequested = true;
		  failDetected = true;
		}

	      if(frameWords[1] != 0xFFFFFF00)
		{
		  cout << "Corrupt or missing frame marker" << endl;
		  exitRequested = true;
		  failDetected = true;
		}
	    }
	  // additional frames will be frome each dcm2 board
	  else
	    {
	      cout << "   DCM2 Frame:  ";
	      for(unsigned int iword = 0; iword < 8; iword++)
		{
		  cout.width(8); cout.fill('0'); cout << hex << frameWords[iword] << dec << " ";
		}
	      // ejd 10/19/2016 Seg fault nFrameWords == 0
	      //cout << "... ";
	      //cout.width(8); cout.fill('0'); cout << hex << frameWords[nFrameWords-1] << dec << endl;
	  
	      if(frameWords[1] != 0xFFFFFF00)
		{
		  cout << "Corrupt or missing frame marker" << endl;
		  failDetected = true;
		  exitRequested = true;
		}

	      if(writeFrames)
		{
		  // get event number
		  //filebuffer->SetEventNumber(frameWords[6]);

		  // add frame to file buffer
		  //filebuffer->CopyFrameIntoBuffer(nFrameWords,pframeWords);
		}
	    }

	  ioffset = 0;
	  nFrameWords = 0;
	  iframe++;

	  // end the event when all frames are collected
	  if(iframe > nFramesPerEvent)
	    {
	      if(writeFrames)
		{
		  //filebuffer->EndEvent();
		  //filebuffer->CheckBufferSpace();
		}
	      iframe = 0;
	    }
	}
    }

  if(writeFrames)
    {
      cout << "   Closing Output File" << endl;
      if(!filebuffer->CloseFile())
	{
	  cerr << "   Failed to close file: " << filename << endl;
	  delete filebuffer;
	  return;
	}
    }

  cout << endl;
  cout << "   Releasing DMA buffers" << endl;
  
  // Release DMA buffers
  for(unsigned int idma = 0; idma < nbuffers; idma++)
    {
      WDC_DMABufUnlock(pDma[idma]);
    }
  
  cout << "   SEB Readout Complete" << endl;

  jseb2->SetFree();
  
  return;
} // jseb2_seb_reader
