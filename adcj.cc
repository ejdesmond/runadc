#include "Dcm2_JSEB2.h"
#include "adcj.h"

#include <Dcm2_WinDriver.h>
#include <Dcm2_FileBuffer.h>
#include <iostream>
#include <fstream>

//#include <ogzBuffer.h>
//#include <olzoBuffer.h>
#include <oBuffer.h>
#include <phenixTypes.h>
#include <packetConstants.h>
#include <EventTypes.h>
#include <oEvent.h>

using namespace std;
extern bool exitRequested;

#define FAILURE 1
/**
 * this code is adopted from sphenix_adc_test_xmit_dcm.cc in
 * scampbel/source/adc_code. This will configure and get the dititizer
 * to write a data file
 * This uses the jseb2_lib in WinDriver/my_projects which talks to the
 * jseb2 card via generated code. This will use MM Dcm2_JSEB2
 * code for all writes. It is to test using the JSEB2 in a threaded environment
 */


ADC::ADC()
{
  dwOptions_send = DMA_TO_DEVICE;
  dwOptions_send = DMA_TO_DEVICE | DMA_ALLOW_CACHE;
  dwOptions_rec = DMA_FROM_DEVICE | DMA_ALLOW_64BIT_ADDRESS;
  adcJseb2Name = string("JSEB2.10.0"); // default adc JSEB2
  obfilename = "/home/phnxrc/desmond/daq/prdf_output/adceventdata.prdf";
  nread = 4096*2+6; /*16384 32768, 65536+4;  number of byte to be readout */
  ifr=0;
  iwrite =0;
  iprint =0;
  icheck =0;
  istop=0;
  tim.tv_sec = 0;
  //    tim.tv_nsec =128000;
  tim.tv_nsec =172000;
  InitJseb2Dev();
  l1_delay = 48;
  trport2 = 2; // port 2 for adc jseb2 access
  nsample = 31; // default sample size
}


ADC::~ADC()
{
  //delete controller;
  //windriver->Exit();
  //stopWinDriver();
}

void ADC::InitJseb2Dev()
{
  // use JSEB2 only for now
  //jseb2d = new Jseb2Device();
  //jseb2d->Init(); // allocate DMA buffers
 // is this necessary to use the same mutex; try separate first
 //jseb2d->SetID("JSEB2:10.0"); // set mutex id to dcm2 to both jsebs use the same mutex 
}

void ADC::stopWinDriver()
{

  //cout << "ADC::stopWinDriver called " << endl;
  wdc_started = 0;
  delete controller;
  windriver->Exit();
} // stopWinDriver
/**
 * startWinDriver
 * open WinDriver, create Dcm2_Controller ; get jseb2 devices get Dcm2_JSEB2 object
 * of jseb2 named in arguement
 *
 */
bool ADC::startWinDriver()
{
 
  wdc_started = 1;
  //cout << "ADC:startWinDriver - Open WinDriver " << endl;
  static int ifr = 0;
  ifr = 0;
 // create windriver and controller object
 // use different driver open 2 windrivers
 windriver = Dcm2_WinDriver::instance();
 windriver->SetVerbosity(verbosity);
  if(!windriver->Init())
    {
      cerr << "   JSEB2() - Failed to load WinDriver Library" << endl;
      windriver->Exit();
      return false;
    }    
 

  controller = new Dcm2_Controller();
  controller->SetVerbosity(verbosity);
  if(!controller->Init())
    {
      cerr << "   JSEB2() - Failed to initialize the controller machine" << endl;
      delete controller;
      windriver->Exit();
      return false;
    }
 unsigned int njseb2s = controller->GetNJSEB2s();
 //cout << "   A scan of the PCIe bus shows " << njseb2s << " JSEB2(s) installed" << endl;
 //cout << "get  jseb2 for JSEB2 device " << adcJseb2Name.c_str()<< endl;
  jseb2 = controller->GetJSEB2(adcJseb2Name);
  // get the handle to the jseb
  jseb2->Init();
  //cout << "ADC:   " << jseb2->GetID() << " has been selected and initialized" << endl;
 hDev = jseb2->GetHandle();
 //delete controller;

 
 return true;

} // startWinDriver


void ADC::setObDatafileName(string datafilename)
{
  obfilename = datafilename;
  //cout << "set obdata filename = " << obfilename.c_str() << endl;
} // setObDatafileName

/**
 * freeDMAbuffer
 * free the DMCContigBuf allocated by WDC_DMAContigBufLock
 */
void ADC::freeDMAbuffer()
{
  /*
DWORD DLLCALLCONV WDC_DMASGBufLock(
    WDC_DEVICE_HANDLE hDev,
    PVOID pBuf,
    DWORD dwOptions,
    DWORD dwDMABufSize,
    WD_DMA **ppDma);
  */
  
  //WDC_DMASGBufLock(hDev,pbuf_send,dwOptions_send, dwDMABufSize, &pDma_send);
  WDC_DMABufUnlock(pDma_send);
} // freeDMAbuffer

 void ADC::setNumAdcBoards(int numboards)
 {
   nmod = numboards;

 } // setNumAdcBoards

// set slot number of first adc module
void ADC::setFirstAdcSlot(int slotnumber)
{
  imod_start = slotnumber;
} // setFirstAdcSlot

void ADC::setSampleSize(int samplesize)
{
  nsample = samplesize;
} // setSampleSize

// initializes 1 adc board and 1 xmit board located in the slot after the adc board
bool ADC::initialize(UINT32 eventstotake)
{
  cout << "ADC::initialize " << endl;
  int   adc_data[64][32];
    nread = 4096*2+6; /*16384 32768, 65536+4;  number of byte to be readout */
    ifr=0;
    iwrite =0;
    iprint =0;
    icheck =0;
    istop=0;
    isel_dcm = 0; 
    isel_xmit = 1; // always use xmit board
    iext_trig = 1;
  bool windriverstatus = false;
   windriverstatus =  startWinDriver();
    if (windriverstatus == false)
      {
	return false;
      }

    nevent = eventstotake;
    // adc module start slot and number of modules is set from main application
    //imod_start = 16; // adc board in slot 16
    //nmod = 1; // uses 1 adc module board
    //nsample = 31;
    //nsample = 12;
    //cout << "initialize: imod_start = " << imod_start << endl;

     jseb2->SetUseDMA(true); // write one word

    px = buf_send;
     py = read_array;
//     imod =6;
     imod_xmit = imod_start+nmod;
     ichip=6;

     // here f0000008 is sent to tx_mode_reg 
     // skipping initialize tx register

   ifr=0;
     buf_send[0]=0x0;
     buf_send[1]=0x0;
     i=1;
     k=1;
     //i = pcie_send_1(hDev, i, k, px); //pcie_send_1 is for adc operations port 2
     // i = 1 == DMA; i = 0 == single word
     jseb2->Write(trport2,k,px);

    ichip = sp_xmit_sub; //1
     //analog reset of xmit
     buf_send[0]=(imod_xmit <<11)+ (ichip << 8) + sp_xmit_rxanalogreset + (0<<16) ;
     i= 1; // dma
     k= 1;
     //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
     usleep(10);

    //digital reset of xmit
    buf_send[0]=(imod_xmit <<11)+ (ichip << 8) + sp_xmit_rxdigitalreset + (0<<16) ;
     i= 1;
     k= 1;
     jseb2->Write(trport2,k,px); // write 1 word dma mode

     //set byte ordinger
    buf_send[0]=(imod_xmit <<11)+ (ichip << 8) + sp_xmit_rxbytord + (0<<16) ;
    i= 1;
    k= 1;
    jseb2->Write(trport2,k,px); // write 1 word dma mode

    //init xmit
     buf_send[0]=(imod_xmit <<11)+ (ichip << 8) + sp_xmit_init + (0<<16) ;
     //mod_xmit+dcm_last+1, ichip=1, sp_xmit_init=4
     i= 1;
     k= 1;
     // i = pcie_send_1(hDev, i, k, px);
    jseb2->Write(trport2,k,px); // write 1 word dma mode
    usleep(10);

     //   send init to the controller -- take adc controller out of 'run' state
     buf_send[0]= (0x2<<8)+sp_cntrl_timing+ (sp_cntrl_init<<16);
     i= 1;
     k= 1;
     //i = pcie_send_1(hDev, i, k, px);
    jseb2->Write(trport2,k,px); // write 1 word dma mode
    usleep(10);

     buf_send[0]= (0x2<<8)+sp_cntrl_timing+ (sp_cntrl_init<<16);
     i= 1;
     k= 1;
     //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
     usleep(10);

     //   send reset to the controller -- put adc controller back in 'run' state
     buf_send[0]= (0x2<<8)+ sp_cntrl_timing+ (sp_cntrl_reset<<16);
     i= 1;
     k= 1;
     //i = pcie_send_1(hDev, i, k, px);
    jseb2->Write(trport2,k,px); // write 1 word dma mode
     usleep(10);

     for (ik =0; ik<nmod; ik++) {

       // send sample size
      imod = ik+imod_start;
      ichip = sp_adc_slowcntl_sub;
      buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_evt_sample + ((nsample-1)<<16) ;
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
      jseb2->Write(trport2,k,px); // write 1 word dma mode
      usleep(10);

      //   set L1 delay  -- to 255
      
      buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_l1_delay + (l1_delay<<16) ;
      //printf(" buf_send = %x\n", buf_send[0]);
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
      jseb2->Write(trport2,k,px); // write 1 word dma mode
      usleep(10);

//   set dat2link on for xmit 
      buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_rd_link + (1<<16) ; //1 on, 0 off
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
      usleep(10);

     //   set dat2control off for xmit
     buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_rd_cntrl + (0<<16) ;
      i= 1;
      k= 1;
     jseb2->Write(trport2,k,px); // write 1 word dma mode
     usleep(10);

      //   set pulse trigger off
      buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_pulse + (0x0<<16) ;
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
      jseb2->Write(trport2,k,px); // write 1 word dma mode
      usleep(10);

     // Here the calibration trigger is set to off when using the xmit.
	  buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_caltrig + (0x0<<16) ;
	  i= 1;
	  k= 1;
	  //i = pcie_send_1(hDev, i, k, px);
	  jseb2->Write(trport2,k,px); // write 1 word dma mode
	  usleep(10);

     //   set test trigger off
      buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_test_trig + (0x0<<16) ;
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
      jseb2->Write(trport2,k,px); // write 1 word dma mode
      usleep(10);

      //   set select L1 trigger on
      buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_l1 + (0x1<<16) ;
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
      jseb2->Write(trport2,k,px); // write 1 word dma mode
      usleep(10);

     // here is a test if imod != imod_start then tunr off link_rxoff
      // This should not be executed at any rate since there is only 1 module
      if(imod != imod_start) 
	   {
	     //printf(" type 1 to link rx off if not first module");
	     //scanf("%d",&is);
	     ichip = sp_adc_slowcntl_sub;
	     buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_link_rxoff + (0<<16) ;
	     i= 1;
	     k= 1;
	     //i = pcie_send_1(hDev, i, k, px);
	     jseb2->Write(trport2,k,px); // write 1 word dma mode

	     usleep(10);
	     //printf(" rx_off called , module %d\n", imod);
	   } //if imod!= imod_start


      i = adc_setup(hDev,imod, 0);
      usleep(500);
     } // next module loop over nmod

  // set last module location fro xmit
 
	 imod = imod_xmit;
	 ichip = sp_xmit_sub;
	 buf_send[0]=(imod <<11)+ (ichip << 8) + sp_xmit_lastmod + (imod_start<<16) ;
	 i= 1;
	 k= 1;
	 //i = pcie_send_1(hDev, i, k, px);
	 jseb2->Write(trport2,k,px); // write 1 word dma mode
	 usleep(10);
	 // printf(" set last module location for xmit %d\n", imod_start);



     // LOWER THE BUSY
      buf_send[0]= (0x2<<8)+sp_cntrl_busyrst+ (0<<16);
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, buf_send);
      jseb2->Write(trport2,k,px); // write 1 word dma mode
      usleep(10);

      ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
      buf_send[0]=(imod<<11)+(ichip<<8)+(8) + (0x0<<16);  // 8 = "nothing" (grabbing the backplane bus)
      i=1;
      k=1;
      //i = pcie_send_1(hDev, i, k, buf_send);
      jseb2->Write(trport2,k,px); // write 1 word dma mode
      usleep(10);
      



    // close windriver
    stopWinDriver();

  return true;
} // initialize


void ADC::setJseb2Name(string jseb2name)
{
  adcJseb2Name = jseb2name;
} // setJseb2Name
void ADC::adcResets(int index_nmod)
{

  printf("Execute pulse generator code \n");

} // adcResets


void ADC::setBusy()
{
     buf_send[0]= (0x2<<8)+sp_cntrl_busyrst+ (0<<16);
      i= 1;
      k= 1;
      i = pcie_send_1(hDev, i, k, buf_send);
      usleep(10);

      ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
      buf_send[0]=(imod<<11)+(ichip<<8)+(8) + (0x0<<16);  // 8 = "nothing" (grabbing the backplane bus)
      i=1;
      k=1;
      i = pcie_send_1(hDev, i, k, buf_send);
      usleep(10);


} // setBusy

void ADC::teststart()
{
 
  PHDWORD  *buffer; // some work memory to hold the buffer
  oBuffer *ob;    
  int i,k,ia,ik;
  int status;
  int count;
  
  UINT32 nwords = 1;
  UINT32 nbytes = nwords*4;
  bool skipstatusread = true;

  printf("ADC::TESTSTART events to take = %d  \n",nevent);
startWinDriver();
	 for (ia=0; ia<nevent; ia++) 
	   {
	     //startWinDriver();
	     if (skipstatusread != true)
	       {
	     for (ik =0; ik<nmod; ik++) 
	       {
		 imod = ik+imod_start;

		 ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
		 buf_send[0]=(imod<<11)+(ichip<<8)+(8) + (0x0<<16);  // read out status
		 i=0; // single word
		 k=1;
		 i = pcie_send_1(hDev, i, k, px);
		 
		 
		 // init receiver
		 nword =1;
		 py = read_array;
		 i = pcie_rec_2(hDev,0,1,nword,iprint,py); // initialize port 2 for single read

		 ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
		 buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_readback_status) + (0x0<<16);  // read out status
		 i=1;
		 k=1;
		 i = pcie_send_1(hDev, i, k, px);
		 usleep(10);

		     py = read_array;
		     printf("READ %d words \n",nword );
		     i = pcie_rec_2(hDev,0,2,nword,iprint,py);     // read nread words - single word read
		  printf(" read array %x %x %x %x \n", read_array[0], read_array[1], read_array[2], read_array[3]);


	       }// next module
	       } // skip status read

	     // reset busy
      if (ia%50 == 0) {
	 printf("  busy reset  %d\n", ia);
      }
      //cout << " reset busy wait" << endl;
      buf_send[0]= (0x2<<8)+sp_cntrl_busyrst+ (0<<16);
      i= 1; // was 1 dma try single word transfer
      k= 1;
      //i = pcie_send_1(hDev, i, k, buf_send);
      //jseb2d->pcie_send_1(hDev, i, k, buf_send);

      jseb2->SetUseDMA(true); // write one word
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);

      ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
      buf_send[0]=(imod<<11)+(ichip<<8)+(8) + (0x0<<16);  // 8 = "nothing" (grabbing the backplane bus)
      i=0;
      k=1;
      //i = pcie_send_1(hDev, i, k, buf_send);
      //jseb2d->pcie_send_1(hDev, i, k, buf_send);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);
      usleep(30000);
      //cout << "busy wait done " << endl;
      //stopWinDriver();
      if (exitRequested == true) {
	cout << "adcj:: exitRequested " << endl;
	break;
      }
	   } // get next nevent
 stopWinDriver();
} // teststart

// write event data with obuffer to a prdf file
// see newbasic/oBufferExample
// Use Jseb2 for read write to jseb. jseb was assigned in startWinDriver
void ADC::start_oevent()
{
 int   adc_data[64][32];
 int   writteneventcount = 0;
  

  PHDWORD  *buffer; // some work memory to hold the buffer
  oBuffer *ob;    
  int i;
  int status;
  int count;
  const int buffer_size = 256*1024*4 ;  // makes  4MB (specifies how many dwords, so *4)

  buffer = new PHDWORD [buffer_size];
 // some parameters:
  int irun = 20001; // run number
  int p1_id = 35001; // packet id 1
  int p2_id = 35002; // packet id 2

  //printf("START_OEVENT EXECUTING \n");

  
  if (isel_dcm == 1 ) {
    cout << "Writing to file " << obfilename.c_str() << endl;
  ob = new oBuffer (obfilename.c_str(), buffer, buffer_size, status, irun);
  }

  PHDWORD p1[1000];
  PHDWORD p2[2000];
  PHDWORD pdata[20000];

  for (i= 0; i < 1000; i++)
    {
      p1[i] = i;
    }

  for (i= 0; i < 2000; i++)
    {
      p2[i] = 2*i+1;
    }

  int max_evt_length = 1000 + 2000 + 200; // 200 is too much, but see above
  // we add an empty begin-run event just to be meticulous
  if (isel_dcm == 1 ) {
  ob->nextEvent( max_evt_length, BEGRUNEVENT ); 
  }
  //cout << "start_oevent START WINDRIVER " << endl;
  //startWinDriver();
 
  // adc control is on port 1; dcm2 is on port 2 of JSEB2.10.0
 
  //jseb2->SetFree();
  //cout << "ADCJ: get jseb2 lock " << endl;
 //jseb2->GetLock();
  //cout << "ADCJ: jseb2 lock obtained " << endl;
  unsigned int trport1 = 1;
  unsigned int trport2 = 2;
  unsigned int rcvport1 = 1;
  unsigned int rcvport2 = 2;
  UINT32 nwords = 1;
  UINT32 nbytes = nwords*4;
  bool readstatus = true; // read status flag

    bool wasUseDMA = jseb2->GetUseDMA();
    jseb2->SetUseDMA(false);

	 for (ia=0; ia<nevent; ia++) 
	   {
	     if(iwrite != 1) {
	       //printf(" sending trigger for event %d\n", ia);
	       //       scanf("%d",&is);
	     }
	     startWinDriver();
	     if (ia%200 == 0) {
	       printf("  busy reset  %d\n", ia);
	     }
	     bzero(pdata,20000);
	     if (readstatus == true)
	       {
	     for (ik =0; ik<nmod; ik++) 
	       {
		 
		 imod = ik+imod_start;

		  //     command no-op to enable module output enable
		  // otherwise the LVDS input at controller will be floating high
		     ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
		     buf_send[0]=(imod<<11)+(ichip<<8)+(8) + (0x0<<16);  // read out status
		     i=1;
		     k=1;
		     i = pcie_send_1(hDev, i, k, px);
		     //jseb2->InitTRX(trport2,4,rcvport2,4);
		     //jseb2->Write(trport2, 1, px);
		     // init receiver
		     nword =1;
		     py = read_array;
		     
		     i = pcie_rec_2(hDev,0,1,nword,iprint,py); 
		     //jseb2->Read(rcvport2,nword,py);

		     // read out status of adc
		     ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
		     buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_readback_status) + (0x0<<16);  // read out status
		     i=1;
		     k=1;
		     i = pcie_send_1(hDev, i, k, px);
		     //jseb2->InitTRX(trport2,4,rcvport2,8);
		     //jseb2->Write(trport2, 1, px);
		     usleep(10);

		     // read out 2 32 bit words

		     py = read_array;
		     
		     i = pcie_rec_2(hDev,0,2,nword,iprint,py);     // read out 2 32 bits words
		     //jseb2->Read(rcvport2,2,py);

		     //printf(" read array %x %x %x %x \n", read_array[0], read_array[1], read_array[2], read_array[3]);
//		 
		 //printf("module %d receive data word = %x, %x \n", imod, read_array[0], read_array[1]);
		 //printf (" header word = %x \n" ,((read_array[0] & 0xffff) >> 8));
		 //printf (" module address = %d \n" ,(read_array[0] & 0x1f));
		 //printf (" upper adc rx pll locked %d\n", ((read_array[0] >>31) & 0x1));
		 //printf (" upper adc rx dpa locked %d\n", ((read_array[0] >>30) & 0x1));
		 //printf (" upper adc rx aligment %d\n", ((read_array[0] >>29) & 0x1));
		 //printf (" upper adc rx data valid %d\n", ((read_array[0] >>28) & 0x1));
		 //printf (" lower adc rx pll locked %d\n", ((read_array[0] >>27) & 0x1));
		 //printf (" lower adc rx dpa locked %d\n", ((read_array[0] >>26) & 0x1));
		 //printf (" lower adc rx aligment %d\n", ((read_array[0] >>25) & 0x1));
		 //printf (" lower adc rx data valid %d\n", ((read_array[0] >>24) & 0x1));
		 //printf (" link pll locked %d\n", ((read_array[0] >>23) & 0x1));
		 //printf (" clock pll locked %d\n", ((read_array[0] >>22) & 0x1));
		 //printf (" trigger bufer empty %d\n", ((read_array[0] >>21) & 0x1));
	       } //end nmod module loop
	       } // readstatus flag


	     //      for (ijk=0; ijk<1000000000; ijk++) {
	     // EXTTRIG 
	     // this command was sent in exttrig but not in exttrig_xmit_dcm - leave it here
	     //if (iext_trig != 1)buf_send[0]= (0x2<<8)+ sp_cntrl_timing+ ((sp_cntrl_l1)<<16);
	     //i= 1;
	     //k= 1;
	     //if(iext_trig != 1) i = pcie_send_1(hDev, i, k, px);
	     //usleep(100);
	     //      }

	     // when using the xmit and an external trigger we do not wait for the trigger
	     // the data has already gone through to the dcm

#ifdef LOCAL_DCM_CONTROL

    if(isel_dcm == 1) 
	{
	  nread = 3;
	  i = pcie_rec(hDev,0,1,nread,iprint,py);     // read out 2 32 bits words from dcm
	  //jseb2->InitTRX(trport2,4,rcvport2,12);
	  //jseb2->Read(rcvport2,nread,py);
	  // comments from Dcm2_Board::DMAFrameIntoBuffer(Dcm2_FileBuffer *filebuffer
	  // Instruct FPGA5 to start sending data
	  // number of words to send minus header and 
	  // trailer words (these added by the FPGA)
	  ichip =5;
	  iadd = (imod_dcm<<11)+ (ichip<<8);
	  buf_send[0]=(imod_dcm <<11)+ (ichip << 8) + dcm2_5_readdata + ((nread-2)<< 16);  
	  /* number word to read- (header+trailer)*/
	  buf_send[1]=0x5555aaaa;       
	  // -1 for the counter
	  ik = pcie_send(hDev, 1, 1, px);  //** for dcm2 status read send 2 words **//
	  //jseb2->InitTRX(trport2,4,rcvport2,12);
	  //jseb2->Write(trport2, 2, px);
	  usleep(100);

	  // READING nread header words 
	  printf("READING %d header words \n",nread);
	  i = pcie_rec(hDev,0,2,nread,iprint,py); //dcm
	  
	  //jseb2->InitTRX(trport2,4,rcvport2,nread*4); // needed to read out single mode
	  //jseb2->Read(rcvport2,nread,py);

	  if(iwrite != 1) // use 10 to always write out diagnostics
	    {
	      printf(" header 1 = %x\n", read_array[0]);
	      printf(" header 2 = %x\n", read_array[1]);
	      printf(" header 3 = %x\n", read_array[2]);
	      printf(" header word = %x\n", (read_array[0]& 0xffff));
	      printf(" event number = %x\n", (((read_array[0])>>16)+((read_array[1] & 0xffff)<<16)));
	      printf(" word count = %x %d\n",read_array[2] & 0xffff , read_array[2] & 0xffff);
	      printf(" trail word = %x\n", ((read_array[2] >>16) & 0xffff));
	      //scanf("%d", &i );
	    } //if iwrite!=1

	  UINT32 _event_number;
	  //_event_number = (((read_array[0])>>16)+((read_array[1] & 0xffff)<<16)));

	  iread = read_array[2] & 0xffff;
	  nread = iread+1;
	  kword = (nread/2);
	  if(nread%2 !=0) kword = kword+1;
	  //
	  // Instruct FPGA5 to start sending data
	  // number of words to send minus header and trailer words
	  i = pcie_rec(hDev,0,1,nread,iprint,py);     // dcm read out 2 32 bits words
	  buf_send[0]=(imod_dcm <<11)+ (ichip << 8) + dcm2_5_readdata + ((nread-2)<< 16);  /* number word to read- (header+trailer)*/
	  buf_send[1]=0x5555aaaa;                                                      // -1 for the counter
	  ik = pcie_send(hDev, 1, 1, px);  
	  //jseb2->InitTRX(trport2,8,rcvport1,nread*4);
	  //jseb2->Write(trport2, 1, px);
	  usleep(10);
	  //printf("READ from pcie_rec DATA %d words  on port 1 \n",nread);
	  i = pcie_rec(hDev,0,2,nread,iprint,py); //dcm
	  
	  printf("READ from JSEB2 DATA %d words  on port 1 \n",nread);
	  // need to execute Read2 to read single word method; sets word count
	  // Read2 initializes port 2 for a read
	  //jseb2->Read2(rcvport1, nread, i,py); // same functions as pcie_rec(hDev,0,1,nread,iprint,py)
	  //jseb2->Read(rcvport1,nread,py); // dcm2 is on port 1
	  printf("EVENT 0x%x  has  %d ints \n",ia,nread );
	  


	  // here write out header to oBuffer

	  
	  if (nread == 1998) {
	    printf("CREATE NEW DATA EVENT %d 0x%x of size %d \n",ia,ia,nread);
	    ob->nextEvent( max_evt_length, DATAEVENT); // we start assembling a new data event
	  } // write event


	  for (i=0; i<(nread-1); i++) 
	    {
      
	       
	      //        u32Data = (idcm_read_array[2*i+1]<<16)+idcm_read_array[2*i+2];
	      u32Data = (read_array[i] & 0xffff0000) + (read_array[i+1] &0xffff);
	      read_array1[i] = u32Data;

	     
	      if (nread <= 1998) // only write out good data
		{
		  pdata[i] = u32Data;
		
		
	      if(iwrite != 1) // write to screen always
		{
		  
		  if(i%8 == 0) cout << i << " " ; //printf("%3d",i);
		  
		  //printf(" %9x",u32Data);
		  cout << hex << " " << u32Data << dec;
		  if(i%8 == 7) printf("\n");
		} //if iwrite!=1
		
		} // nread == 1998
	      
	    } // nread


	  
	  // add new data packet
	  if (nread == 1998) {
	    printf("WRITE NEW PACKET \n");
	    
	    ob->addUnstructPacketData (pdata, nread, p1_id, 4, ID4EVT);
	    writteneventcount++;
	  }

	    // this writes modified data to a text file
	  if(iwrite == 1) 
	    {
	      if (nread == 1998)
		{
		  fprintf(outf,"NUM WORDS %d\n", nread);
		  for (i=0; i<(nread-1); i++) 
		    {
		      //        u32Data = (idcm_read_array[2*i+1]<<16)+idcm_read_array[2*i+2];
		      fprintf(outf," %9x",read_array1[i]);
		  
		      if(i%8 == 7) fprintf(outf,"\n");
		    }
		  if((i%8) != 7) fprintf(outf,"\n");
		} // only write data with 1998 words (good data)
	    } //if iwrite==1
	  
	  if(iwrite != 1) 
	    {
	      if((i%8) != 7) printf("\n");
	      //scanf("%d", &i );
	      printf(" event number = %d \n", ( read_array1[1] & 0xffff));
	      printf(" flag word = %x \n", (read_array1[2] & 0xffff));
	      printf(" detector ID = %x \n", (read_array1[3] & 0xffff));
	      printf(" module address = %x \n", (read_array1[4] & 0xffff));
	      printf(" clock number = %x \n", (read_array1[5] & 0xffff));
	      printf(" FEM header = %x \n", (read_array1[6] & 0xffff));
	      printf(" FEM module address= %x \n", (read_array1[7] & 0xffff));
	      printf(" FEM event number = %d \n", (read_array1[8] & 0xffff));
	      printf(" FEM clock number = %x \n", (read_array1[9] & 0xffff));
	    } //if iwrite!=1


	  iparity =0;
	  //printf("LOOP over samples  \n");
	  for (is=0; is<4+(nsample*64); is++) 
	    {
	      if(is%2 == 0) u32Data = (read_array1[is+6] & 0xffff) <<16;
	      else 
		{
		  u32Data = u32Data + (read_array1[is+6] & 0xffff);
		  //         u32Data = u32Data+ ((read_array1[is+6] & 0xffff) <<16);
		  iparity = iparity ^ u32Data;
		}
	    } //for loop is 0-4+(nsample*64)
	  int parity_index1 = 64*nsample + 10; //778 for 12 samples
	  int parity_index2 = 64*nsample + 11; //779 for 12 samples
	  i= ((read_array1[parity_index1] & 0xffff) <<16) +(read_array1[parity_index2] & 0xffff);
	  if(i != iparity) 
	    {
	      //printf(" event = %d, Partity error....... = %x %x\n", ia, i, iparity);
	      //scanf("%d", &i);
	    } //if i!= iparity
	  //printf(" event = %d, Partity word = %x %x\n", ia, i, iparity);
	  
	  if(iwrite != 1) 
	    {
	      printf(" parity word = %x\n", iparity);
	      printf(" packet parity word = %x\n", (((read_array1[parity_index1] & 0xffff) <<16) +
						    (read_array1[parity_index2] & 0xffff)));
	      //scanf("%d", &i );
	      for (is=0; is<nsample; is++) 
		{
		  for (k=0; k<32; k++) 
		    {
		      adc_data[k*2][is] = read_array1[11+((k*nsample+is)*2)] & 0xffff;
		      adc_data[(k*2)+1][is] = read_array1[10+((k*nsample+is)*2)] & 0xffff;
		    }
		}
	      
	      if (nread == 1998) // only write out data if it is a good event ( has 1998 words)
		{
		  /*
		  for (is=0; is<64; is++) 
		    {
		      
		      printf(" channel %d ", is);
		      for (k=0; k<nsample; k++) 
			{
			  printf(" %4x", adc_data[is][k]);
			}
		      printf("\n");
		    }
		  */
		} // nread test
	      
	      //scanf("%d", &i );
	    } //if iwrite!=1
	} //end if dcm flag

#endif     
      // lowers the busy! (we think)
      //printf("Lower the BUSY \n");

      buf_send[0]= (0x2<<8)+sp_cntrl_busyrst+ (0<<16);
      i= 1;
      k= 1;
      i = pcie_send_1(hDev, i, k, buf_send);
      //jseb2d->pcie_send_1(hDev, i, k, buf_send);
      //jseb2->Write(trport2, 1, buf_send);
      usleep(10);

      ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
      buf_send[0]=(imod<<11)+(ichip<<8)+(8) + (0x0<<16);  // 8 = "nothing" (grabbing the backplane bus)
      i=1;
      k=1;
      i = pcie_send_1(hDev, i, k, buf_send);
      //jseb2d->pcie_send_1(hDev, i, k, buf_send);
      //jseb2->Write(trport2, 1, buf_send);
      usleep(10);

      
       stopWinDriver();

   } //end nevent a

	 //cout << "START_OEVENT ENDING " << endl;
  // we add an empty end-run event 
 if (isel_dcm == 1) {
  ob->nextEvent( max_evt_length, ENDRUNEVENT );
   // deleting the object will flush the current buffer
  delete ob;
 }
  
  jseb2->SetUseDMA(wasUseDMA);
  //printf("WROTE %d events \n",writteneventcount);
  //jseb2->SetFree();
  //stopWinDriver();

} // start_oevent




/**
 * setDcm2State
 * could be used used to download specific dcm2 state condistions: e.g. mask values
 */
void setDcm2State()
{


} // setDcm2State




// default is port 2 (adc)
int ADC::pcie_send_1(WDC_DEVICE_HANDLE hDev, int mode, int nword, UINT32 *buff_send)
{
/* imode =0 single word transfer, imode =1 DMA */
  static UINT32 *pcie_buf_send_1;
    int nwrite,i,j, iprint;
    static int ifr=0;
    //printf("pcie_send_1 ifr = %d \n",ifr);
    //if (ifr == 0) {
    // use wdc_started to prevent multiple execution of DMCContigBufLock execution since multiple
    // access to the WDC can occur. 
      if (wdc_started == 1) {
	wdc_started = 0;
      ifr=1;
      printf("ADC::pcie_send_1 EXECUTING WDC_DMA \n");
      dwDMABufSize = 140000;
      dwStatus = WDC_DMAContigBufLock(hDev, &pbuf_send, dwOptions_send, dwDMABufSize, &pDma_send);
      if (WD_STATUS_SUCCESS != dwStatus) {
             printf("Failed locking a send Contiguous DMA buffer. Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
      }
      pcie_buf_send_1 = (UINT32*)pbuf_send;
    }
    iprint =0;
    if(mode ==1 ) {
      for (i=0; i< nword; i++) {
        *(pcie_buf_send_1+i) = *buff_send++;
/*	printf("%d \n",*(buf_send+i));   */
      }
    }
    if(mode == 0) {
     nwrite = nword*4;
     /*setup transmiiter */
     dwAddrSpace =2;
     u32Data = 0x20000000;
     dwOffset = 0x20;
     WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
     dwAddrSpace =2;
     u32Data = 0x40000000+nwrite;
     dwOffset = 0x20;
     WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
     for (j=0; j< nword; j++) {
       dwAddrSpace =4;
       dwOffset = 0x0;
       u32Data = *buff_send++;
       WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
     }
     for (i=0; i<20000; i++) {
       dwAddrSpace =2;
       dwOffset = 0xC;
       WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset, &u32Data);
       if(iprint ==1) printf(" status reed %d %X \n", i, u32Data);
       if(((u32Data & 0x80000000) == 0) && iprint == 1) printf(" Data Transfer complete %d \n", i);
       if((u32Data & 0x80000000) == 0) break;
     }
    }
    if( mode ==1 ){
      nwrite = nword*4;
      WDC_DMASyncCpu(pDma_send);
/*
      printf(" nwrite = %d \n", nwrite);
      printf(" pcie_send hDev = %d\n", hDev);
      printf(" buf_send = %X\n",*buf_send);
*/
     /*setup transmiiter */
      dwAddrSpace =2;
      u32Data = 0x20000000;
      dwOffset = 0x20;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
      dwAddrSpace =2;
      u32Data = 0x40000000+nwrite;
      dwOffset = 0x20;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

/* set up sending DMA starting address */

      dwAddrSpace =2;
      u32Data = 0x20000000;
      dwOffset = 0x0;
      u32Data = pDma_send->Page->pPhysicalAddr & 0xffffffff;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

      dwAddrSpace =2;
      u32Data = 0x20000000;
      dwOffset = 0x4;
      u32Data = (pDma_send->Page->pPhysicalAddr >> 32) & 0xffffffff;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

/* byte count */
      dwAddrSpace =2;
      dwOffset = 0x8;
      u32Data = nwrite;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

/* write this will start DMA */
      dwAddrSpace =2;
      dwOffset = 0xc;
      u32Data = 0x00200000;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

      for (i=0; i<20000; i++) {
        dwAddrSpace =2;
	dwOffset = 0xC;
        WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset, &u32Data);
	if(iprint ==1) printf(" DMA status reed %d %X \n", i, u32Data);
	if(((u32Data & 0x80000000) == 0) && iprint == 1) printf(" DMA complete %d \n", i);
	if((u32Data & 0x80000000) == 0) break;
      }
      WDC_DMASyncIo(pDma_send);
    }
    return i;
    }




void ADC::setL1Delay(int l1delay)
 {
    l1_delay = l1delay;
 }



int ADC::pcie_rec_2(WDC_DEVICE_HANDLE hDev, int mode, int istart, int nword, int ipr_status, UINT32 *buff_rec)
{
/* imode =0 single word transfer, imode =1 DMA */
/* nword assume to be number of 16 bits word */

#include "wdc_defs.h"
#define  t1_tr_bar 0
#define  t2_tr_bar 4
#define  cs_bar 2

/**  command register location **/

#define  tx_mode_reg 0x28
#define  t1_cs_reg 0x18
#define  r1_cs_reg 0x1c
#define  t2_cs_reg 0x20
#define  r2_cs_reg 0x24

#define  tx_md_reg 0x28

#define  cs_dma_add_low_reg 0x0
#define  cs_dma_add_high_reg  0x4
#define  cs_dma_by_cnt 0x8
#define  cs_dma_cntrl 0xc
#define  cs_dma_msi_abort 0x10

/** define status bits **/

#define  cs_init  0x20000000
#define  cs_mode_p 0x8000000
#define  cs_mode_n 0x0
#define  cs_start 0x40000000
#define  cs_done  0x80000000

#define  dma_tr1  0x100000
#define  dma_tr2  0x200000
#define  dma_tr12 0x300000
#define  dma_3dw_trans 0x0
#define  dma_3dw_rec   0x40
#define  dma_in_progress 0x80000000

#define  dma_abort 0x2

    static DWORD dwAddrSpace;
    static DWORD dwDMABufSize;

    static UINT32 *buf_rec;
    static WD_DMA *pDma_rec;
    static DWORD dwStatus;
    static DWORD dwOptions_rec = DMA_FROM_DEVICE;
    static DWORD dwOffset;
    static UINT32 u32Data;
    static UINT64 u64Data;
    static PVOID pbuf_rec;
    int nread,i,j, iprint,icomp,is;
    static int ifr=0;

    if (ifr == 0) {
      ifr=1;
      dwDMABufSize = 140000;
      dwStatus = WDC_DMAContigBufLock(hDev, &pbuf_rec, dwOptions_rec, dwDMABufSize, &pDma_rec);
      if (WD_STATUS_SUCCESS != dwStatus) {
             printf("Failed locking a send Contiguous DMA buffer. Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
      }
      buf_rec = (UINT32*)pbuf_rec;
    }
    iprint =0;
 //   printf(" istart = %d\n", istart);
 //   printf(" mode   = %d\n", mode);
/** set up the receiver **/
    if((istart == 1) | (istart == 3)) {
// initalize transmitter mode register...
     dwAddrSpace =2;
     u32Data = 0xf0000008;
     dwOffset = tx_mode_reg;
     WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
     if(mode == 1) {
/* write this will abort previous DMA */
       dwAddrSpace =2;
       dwOffset = cs_dma_msi_abort;
       u32Data = dma_abort;
       WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
/* clear DMA register after the abort */
       dwAddrSpace =2;
       dwOffset = cs_dma_msi_abort;
       u32Data = 0;
       WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
     }
     /*initialize the receiver */
     dwAddrSpace =cs_bar;
     u32Data = cs_init;
     dwOffset = r2_cs_reg;
     WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
     /* write byte count **/
     dwAddrSpace =cs_bar;
     u32Data = cs_start+nword*4;
     dwOffset = r2_cs_reg;
     WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
     if(ipr_status ==1) {
      dwAddrSpace =cs_bar;
      u64Data =0;
      dwOffset = t2_cs_reg;
      WDC_ReadAddr64(hDev, dwAddrSpace, dwOffset, &u64Data);
      printf (" status word before read = %x, %x \n",(u64Data>>32), (u64Data &0xffff));
     }

     return 0;
    }
    if ((istart == 2) | (istart == 3)) {
//     if(ipr_status ==1) {
//      dwAddrSpace =2;
//      u64Data =0;
//      dwOffset = 0x18;
//      WDC_ReadAddr64(hDev, dwAddrSpace, dwOffset, &u64Data);
//      printf (" status word before read = %x, %x \n",(u64Data>>32), (u64Data &0xffff));
//     }
     if(mode == 0) {
      nread = nword/2+1;
      if(nword%2 == 0) nread = nword/2;
      for (j=0; j< nread; j++) {
       dwAddrSpace =t2_tr_bar;
       dwOffset = 0x0;
       u64Data =0xbad;
       WDC_ReadAddr64(hDev,dwAddrSpace, dwOffset, &u64Data);
//       printf("u64Data = %16X\n",u64Data);
       *buff_rec++ = (u64Data &0xffffffff);
       *buff_rec++ = u64Data >>32;
//       printf("%x \n",(u64Data &0xffffffff));
//       printf("%x \n",(u64Data >>32 ));
//       if(j*2+1 > nword) *buff_rec++ = (u64Data)>>32;
//       *buff_rec++ = 0x0;
      }
      if(ipr_status ==1) {
       dwAddrSpace =cs_bar;
       u64Data =0;
       dwOffset = t2_cs_reg;
       WDC_ReadAddr64(hDev, dwAddrSpace, dwOffset, &u64Data);
       printf (" status word after read = %x, %x \n",(u64Data>>32), (u64Data &0xffff));
      }
      return 0;
     }
     if( mode ==1 ){            ///**** not up to date ****///
      nread = nword*2;
      WDC_DMASyncCpu(pDma_rec);
/*
      printf(" nwrite = %d \n", nwrite);
      printf(" pcie_send hDev = %d\n", hDev);
      printf(" buf_send = %X\n",*buf_send);
*/
/*setup receiver
      dwAddrSpace =2;
      u32Data = 0x20000000;
      dwOffset = 0x1c;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
      dwAddrSpace =2;
      u32Data = 0x40000000+nread;
      dwOffset = 0x1c;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
*/
/* set up sending DMA starting address */

      dwAddrSpace =2;
      u32Data = 0x20000000;
      dwOffset = 0x0;
      u32Data = pDma_rec->Page->pPhysicalAddr & 0xffffffff;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

      dwAddrSpace =2;
      u32Data = 0x20000000;
      dwOffset = 0x4;
      u32Data = (pDma_rec->Page->pPhysicalAddr >> 32) & 0xffffffff;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

/* byte count */
      dwAddrSpace =2;
      dwOffset = 0x8;
      u32Data = nread;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

/* write this will start DMA */
//      dwAddrSpace =2;
//      dwOffset = 0xc;
//      u32Data = 0x00100040;
//      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

/* write this will start DMA */
      dwAddrSpace =2;
      dwOffset = cs_dma_cntrl;
      is = (pDma_rec->Page->pPhysicalAddr >> 32) & 0xffffffff;
      if(is == 0) {
//**         if(iwrite !=1 ) printf(" use 3dw \n");
       u32Data = dma_tr2+dma_3dw_rec;
      }
      else {
       u32Data = dma_tr2+dma_4dw_rec;
//**        if(iwrite !=1 ) printf(" use 4dw \n");
      }
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);


      icomp=0;
      for (i=0; i<20000; i++) {
        dwAddrSpace =2;
	dwOffset = 0xc;
        WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset, &u32Data);
	if(iprint ==1) printf(" DMA status read %d %X \n", i, u32Data);
	if(((u32Data & 0x80000000) == 0)) {
          icomp=1;
//          if(iprint == 1) printf(" DMA complete %d \n", i);
          printf(" DMA complete %d \n", i);
        }
	if((u32Data & 0x80000000) == 0) break;
      }
      if(icomp == 0) {
        printf("DMA timeout\n");
        return 1;
      }
      WDC_DMASyncIo(pDma_rec);
      for (i=0; i< nword; i++) {
        *buff_rec++ = *(buf_rec+i);
/*	printf("%d \n",*(buf_send+i));   */
      }
     }
    }
    return 0;
    }



// adc_setup
int ADC::adc_setup(WDC_DEVICE_HANDLE hDev, int imod, int iprint)
{
#define  sp_cntrl_timing            3
#define  sp_cntrl_init           0x30
#define  sp_cntrl_reset          0x28
#define  sp_cntrl_l1             0x24
#define  sp_cntrl_pulse          0x22

#define  sp_adc_readback_sub        4
#define  sp_adc_readback_transfer   1
#define  sp_adc_readback_read       2
#define  sp_adc_readback_status     3
#
#define  sp_adc_input_sub           2
#define  sp_adc_slowcntl_sub        1

#define  sp_adc_l1_delay            1
#define  sp_adc_evt_sample          2

#define  sp_adc_rd_link             3
#define  sp_adc_rd_cntrl            4

#define  sp_adc_sel_l1              5
#define  sp_adc_sel_pulse           6
#define  sp_adc_sel_test_trig       7

#define  sp_adc_sel_linux_rxoff     8

#define  sp_adc_u_adc_align        10
#define  sp_adc_l_adc_align        11
#define  sp_adc_pll_reset          12

#define  sp_adc_rstblk             13
#define  sp_adc_test_pulse         14

#define  sp_adc_dpa_reset          15

#define  sp_adc_spi_add            20
#define  sp_adc_spi_data           30

#define  sp_adc_lnk_tx_dreset      20
#define  sp_adc_lnk_tx_areset      21
#define  sp_adc_trg_tx_dreset      22
#define  sp_adc_trg_tx_areset      23
#define  sp_adc_lnk_rx_dreset      24
#define  sp_adc_lnk_rx_areset      25

#define  sp_adc_link_mgmt_reset    26
#define  sp_adc_link_conf_w        27
#define  sp_adc_link_conf_add      30
#define  sp_adc_link_conf_data_l   31
#define  sp_adc_link_conf_data_u   32

#define  sp_xmit_lastmod            1
#define  sp_xmit_rxanalogreset      2
#define  sp_xmit_rxdigitalreset     3
#define  sp_xmit_init               4

#define  sp_xmit_sub                1


    int ichip,ich,i,k;
    UINT32 buf_send[40000];
    UINT32 *px;



    px = buf_send;
    ichip = sp_adc_input_sub ;   // controller data go to ADC input section
    for (ich=0; ich<8; ich++) {

//    set spi address

     buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_spi_add + (ich<<16) ;
     i= 1;
     k= 1;
     //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
//
     if(iprint==1) printf(" set spi address to channel %d, module %d\n", ich, imod);
//       scanf("%d",&is);

//      buf_send[0]=(imod<<11)+(ichip<<8)+(mb_pmt_spi_)+((is & 0xf)<<16); //set spi address
//      i=1;
//        k=1;
//        i = pcie_send(hDev, i, k, px);
//       printf(" spi port %d \n",is);
//       scanf("%d",&ik);

//        imod=11;
//        ichip=5;
//
//
//     power reset the ADC
//
     buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
     buf_send[1]=(((0x0)<<13)+(0x8))+((0x03)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0x8, data =0x03;
//

     i=1;
     k=2;

     //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
     usleep(100);   // sleep for 100us
//
//     reove power reset
//
     buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
     buf_send[1]=(((0x0)<<13)+(0x8))+((0x00)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0x8, data =0x00;
//

     i=1;
     k=2;
//       i = pcie_send(hDev, i, k, px);
     //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
     usleep(100);   // sleep for 100us
//
//     reset ADC
//
     buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
     buf_send[1]=(((0x0)<<13)+(0x0))+((0x3c)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0x0, data =0x3c;
//

     i=1;
     k=2;
//       i = pcie_send(hDev, i, k, px);
     //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
     usleep(100);   // sleep for 100us
//
//       if(ich ==7) {
     if(iprint ==1) printf(" send last command ich= %d module %d\n", ich, imod);
//        scanf("%d",&is);
//       }
//
//    set LVDS termination  set addess 0x15  to 1 to 2x drive
//
     buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
     buf_send[1]=(((0x0)<<12)+(0x15))+((0x20)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0x15, data =0x1;
//
     i=1;
     k=2;
//       i = pcie_send(hDev, i, k, px);
     //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
     usleep(100);   // sleep for 100us

//
//    set fix pattern  0xa = sync pattern
//
     buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
     buf_send[1]=(((0x0)<<13)+(0xd))+((0xa)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0xd, data =0xa;
//

     i=1;
     k=2;
//      i = pcie_send(hDev, i, k, px);
     //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
     usleep(100);   // sleep for 100us
//
//       printf(" type 1 to continue \n");
//       scanf("%d",&is);
    }


    usleep(100);
//
//    set up for ADC alignment
//
    ichip = sp_adc_slowcntl_sub ;   // controller data go to ADC input section
//
//       pll reset
//
    if(iprint == 1)printf(" send pll reset\n");
//      scanf("%d",&is);
    buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_pll_reset + (0<<16) ;
    i= 1;
    k= 1;
    //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
    usleep(1000);

//
//       DPA reset
//
    if(iprint == 1) printf(" send ADC DPA reset module %d\n", imod);
//    scanf("%d",&is);
    buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_dpa_reset + (0<<16) ;
    i= 1;
    k= 1;
    //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
    usleep(1000);

//
//       upper ADC alignment
//
    if(iprint==1) printf(" send upper channel align module %d \n", imod);
//    scanf("%d",&is);
    buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_u_adc_align + (0<<16) ;
    i= 1;
    k= 1;
    //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
    usleep(1000);
//
//       lower ADC alignment
//
    if(iprint == 1) printf(" send lower channel align module %d\n", imod);
//      scanf("%d",&is);
    buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_l_adc_align + (0<<16) ;
    i= 1;
    k= 1;
    //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
    usleep(1000);
//
//
//

   ichip = sp_adc_input_sub ;   // controller data go to ADC input section
   for (ich=0; ich<8; ich++) {
    buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_spi_add + (ich<<16) ;
    i= 1;
    k= 1;
    //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
    usleep(100);
//
//    output offset binary code
//
    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
    buf_send[1]=(((0x0)<<12)+(0x14))+((0x0)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0x14, data =0x0;
//

    i=1;
    k=2;

    //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
    usleep(100);   // sleep for 100us

//
//    unset fix pattern  0x0 for normal data taking   --- set to test condition
//
    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
    buf_send[1]=(((0x0)<<13)+(0xd))+((0x0)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0xd, data =0x0;
//

    i=1;
    k=2;

    //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2,k,px); // write 1 word dma mode
    usleep(100);   // sleep for 100us
   }
//
//  test routine
//
//    set spi address

    ich =0;
    buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_spi_add + (ich<<16) ;
    i= 1;
    k= 1;
//    i = pcie_send_1(hDev, i, k, px);
    usleep(100);
//
//    remove channel H,G,F,E from the selection list
//
    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
    buf_send[1]=(((0x0)<<13)+(0x4))+((0x0)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0x4, data =0x0;
//

    i=1;
    k=2;

//    i = pcie_send_1(hDev, i, k, px);
    usleep(100);   // sleep for 100us
//
//    only keep channel a from the selection list
//
    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
    buf_send[1]=(((0x0)<<13)+(0x5))+((0x1)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0x5, data =0x1;
//

    i=1;
    k=2;

///    i = pcie_send_1(hDev, i, k, px);
    usleep(100);   // sleep for 100us
//
//    set fix pattern  0xa = sync pattern
//
     buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
     buf_send[1]=(((0x0)<<13)+(0xd))+((0xc)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0xd, data =0xc;
//

     i=1;
     k=2;
//      i = pcie_send(hDev, i, k, px);
//     i = pcie_send_1(hDev, i, k, px);
     usleep(100);   // sleep for 100us
//
//    restore selection list 1
//
    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
    buf_send[1]=(((0x0)<<13)+(0x4))+((0xf)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0x4, data =0x0;
//

    i=1;
    k=2;

//    i = pcie_send_1(hDev, i, k, px);
    usleep(100);   // sleep for 100us
//
//    restore selection list 2
//
    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
    buf_send[1]=(((0x0)<<13)+(0x5))+((0x3f)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0x5, data =0x1;
//

    i=1;
    k=2;

//    i = pcie_send_1(hDev, i, k, px);
    usleep(100);   // sleep for 100us



    return i;
} // adc_setup


