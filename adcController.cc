/**
 * adcController
 * Sets up adc controller for internal and external trigger
 * Usage: for external trigger requires calls to the following:
 *  setNumAdcBoards(int numboards)
 *  setFirstAdcSlot(int slotnumber)
 *  setSampleSize(int samplesize)
 *  setL1Delay(int l1delay)
 *  initialize_exttrig(UINT32 events, int nwrite){ 
 *  start_exttrig_readout
 *
 */
#include "Dcm2_JSEB2.h"
#include "adcController.h"

#include <oBuffer.h>
#include <phenixTypes.h>
#include <packetConstants.h>
#include <EventTypes.h>
#include <oEvent.h>

using namespace std;
extern bool exitRequested;
#define FAILURE 1

ADCController::ADCController(){  
  dwOptions_send = DMA_TO_DEVICE;
  dwOptions_send = DMA_TO_DEVICE | DMA_ALLOW_CACHE;
  dwOptions_rec = DMA_FROM_DEVICE | DMA_ALLOW_64BIT_ADDRESS;
  adcJseb2Name = string("JSEB2.5.0"); // default adc JSEB2
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

  l1_delay = 48;
  trport2 = 2; // port 2 for adc jseb2 access
  nsample = 31; // default sample size

  debugFuncs = false;
  debug      = false;
} // ADCController ctor
  ADCController::~ADCController(){  

  } // dtor

 
   void ADCController::setNumAdcBoards(int numboards){  
        nmod = numboards;
   } // setNumAdcBoards
   void ADCController::setFirstAdcSlot(int firstdigslotnumber){ 
     imod_start = firstdigslotnumber;
   } // setFirstAdcSlot
   void ADCController::setSampleSize(int samplesize){
    nsample = samplesize;
   } // setSampleSize

  void ADCController::setL1Delay(int l1delay){  

        l1_delay = l1delay;

  } // setL1Delay

void ADCController::adcResets(int nmodindex){ 
 
  if (debugFuncs){
    cout << "ADCController:adcResets " << endl;
  }
  printf("Execute pulse generator code \n");

} // adcResets

  void ADCController::setJseb2Name(string jseb2name){  
  adcJseb2Name = jseb2name;

  } // setJseb2Name
bool ADCController::startWinDriver(){
   
  wdc_started = 1;

  if (debugFuncs){
    cout << "ADCController:startWinDriver - Open WinDriver " << endl;
  }
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
 

  return true;

} // startWinDriver

void ADCController::stopWinDriver(){

  if (debugFuncs){ 
    cout << "ADC::stopWinDriver called " << endl;
  }

  wdc_started = 0;
  delete controller;
  windriver->Exit();
} // stopWinDriver

  void ADCController::freeDMAbuffer(){  
    WDC_DMABufUnlock(pDma_send);
  } // freeDMAbuffer
/**
 * setObDatafileName
 * set the name of the obuffer file to write to
 */
  void ADCController::setObDatafileName(string datafilename){  
      obfilename = datafilename;

  } // setObDatafileName
  
 
  int ADCController::adc_setup(WDC_DEVICE_HANDLE hDev, int imod, int iprint){  


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


   void ADCController::setBusy(){  
     buf_send[0]= (0x2<<8)+sp_cntrl_busyrst+ (0<<16);
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, buf_send);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);

      ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
      buf_send[0]=(imod<<11)+(ichip<<8)+(8) + (0x0<<16);  // 8 = "nothing" (grabbing the backplane bus)
      i=1;
      k=1;
      //i = pcie_send_1(hDev, i, k, buf_send);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);


   } // setBusy

bool ADCController::initialize_xmit(UINT32 events)
{
  return true;

} // initialize_xmit

/**
 * initialize_exttrig
 * Initialize the adc controller for readout through the adc controller using
 * external triggering. This code is from sphenix_adc_test_exttrig_busy
 * parameters: events : number of events to write, nwrite = 1 to write data to file
 */
bool ADCController::initialize_exttrig(UINT32 events, int nwrite){ 



    nread = 4096*2+6; /*16384 32768, 65536+4;  number of byte to be readout */
    ifr=0;
 
    iprint =0;
    icheck =0;
    istop=0;
   isel_dcm = 0; 
    isel_xmit = 0; // not useing xmit board
    iext_trig = 1;

    iwrite = nwrite; // write enable flag

     nloop = 1;
     printf(" number of loops %d \n",nloop);

     // set number of events to take
     nevent = events;
     cout << " Using 1st module slot number " << imod_start << endl;

     cout << " number of FEM module used = " << nmod << endl;
 
     
     if(iwrite ==1) 
       {
	 outf = fopen("/home/phnxrc/desmond/daq/txt_output/adc_exttrigtestdata.txt","w");
       }


     cout << " using enter L1 delay " << l1_delay << endl;

     cout << " using  number of samples = " << nsample << endl;
  

     px = buf_send;
     py = read_array;
//     imod =6;
     imod_xmit = imod_start+nmod;
     ichip=6;

//    for (j=0; j<nloop; j++) {
//       if(inew == 1) {
     dwAddrSpace =2;
     u32Data = 0xf0000008;
     dwOffset = 0x28;
     WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
//       }
     ifr=0;

/** initialize **/
/*      if(j ==0) { */
     buf_send[0]=0x0;
     buf_send[1]=0x0;
     i=1;
     k=1;
     //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2, 1, buf_send);
//
//
//
//
//   send init to the controller
//
     printf(" initialize controller \n");

     buf_send[0]= (0x2<<8)+sp_cntrl_timing+ (sp_cntrl_init<<16);
     i= 1;
     k= 1;
     //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2, 1, buf_send);
     usleep(10);
     buf_send[0]= (0x2<<8)+sp_cntrl_timing+ (sp_cntrl_init<<16);
     i= 1;
     k= 1;
     //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2, 1, buf_send);
//
//   send reset to the controller
//
//     printf(" type 1 to send reset \n");
//     scanf("%d",&is);

     buf_send[0]= (0x2<<8)+ sp_cntrl_timing+ (sp_cntrl_reset<<16);
     i= 1;
     k= 1;
     //i = pcie_send_1(hDev, i, k, px);
     jseb2->Write(trport2, 1, buf_send);

//   set sample size  -- to 10
//     printf(" type 1 to send sample size");
//     scanf("%d",&is)
     printf(" send sample size to controller \n");

     for (ik =0; ik<nmod; ik++) {
      imod = ik+imod_start;
      ichip = sp_adc_slowcntl_sub;
      buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_evt_sample + ((nsample-1)<<16) ;
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);

//   set L1 delay  -- to 255
      printf(" set L1 delay %d to module %d\n", l1_delay,imod);
//      printf(" type 1 to set L1 delay \n");
//      scanf("%d",&is);
      if(iext_trig!=0) 
	buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_l1_delay + (l1_delay<<16) ;
      else buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_l1_delay + (0xff<<16) ;
      //      printf(" buf_send = %x\n", buf_send[0]);
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);
//
//   set dat2link off
//
     printf(" set dat2link off \n");
//    scanf("%d",&is);
      buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_rd_link + (0<<16) ;
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);
      //
      //   set dat2control  on
      //
      //     printf(" type 1 to send sample size");
      //     scanf("%d",&is);      
      buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_rd_cntrl + (1<<16) ;
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);
      //
      //   set pulse trigger off
      //
      //     printf(" type 1 to send sample size");
      //     scanf("%d",&is);
      buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_pulse + (0x0<<16) ;
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);
      //
      //   set test trigger off
      //
      //     printf(" type 1 to send sample size");
      //     scanf("%d",&is);
      buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_test_trig + (0x0<<16) ;
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);
      //
      //   set select L1 trigger on
      //
      //printf(" type 1 to select L1 trigger \n");
      //scanf("%d",&is);
      buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_l1 + (0x1<<16) ;
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, px);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);
      //      printf(" set up complete type 1 to continue \n");
      //      scanf("%d",&is);

      printf(" call adc setup  module %d\n", imod);
      i = adc_setup(hDev,imod, 1);
      printf(" done with adc_setup\n", imod);
      usleep(30000);
      //scanf("%d",&is);
      //sp_adc_sel_link_rxoff
     }
//sp_xmit_lastmod

      buf_send[0]= (0x2<<8)+sp_cntrl_busyrst+ (0<<16);
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, buf_send);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);

      ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
      buf_send[0]=(imod<<11)+(ichip<<8)+(8) + (0x0<<16);  // 8 = "nothing" (grabbing the backplane bus)
      i=1;
      k=1;
      //i = pcie_send_1(hDev, i, k, buf_send);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);

 
  } // initialize_exttrig

void ADCController::start_oevent(){  }

/**
 * start_exttrig_readout
 * read out the adc through the adc controller using an external trigger
 *
 */
  void ADCController::start_exttrig_readout(){ 

 
  }  // start_exttrig_readout
  void ADCController::start_nopulse_readout(){  }

/**
 * adcreadout_exttrig
 * Read out the adc digitizer through the adc controller using
 * an external trigger to trigger the data
 * This code is from sphenix_adc_test_exttrig_busy.
 * The function initialize_exttrig is assumed to be called previous to 
 * this function to initialize the adc for external trigger readout
 */
   int ADCController::adcreadout_exttrig(int eventstotake){ 


     int   adc_data[64][32];
     for (j=0; j<nloop; j++) 
       {
	 	 printf(" start loop %d \n", j);
	 //   send L1 trigger to the controller
	 for (ia=0; ia<nevent; ia++) 
	   {
	     //	     printf(" event %d",ia);
	     
	     for (ik =0; ik<nmod; ik++) 
	       {
		 
		 imod = ik+imod_start;
		 nword =1;
		 py = read_array;
		 
		 printf(" module %d \n", imod);
		 // printf(" read array %x %x %x %x \n", read_array[0], read_array[1], read_array[2], read_array[3]);

		 if (iext_trig==1) // || iext_trig!=1)
		   {
		     //     command no-op to enable module output enable
		     // otherwise the LVDS input at controller will be floating high
		     ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
		     buf_send[0]=(imod<<11)+(ichip<<8)+(8) + (0x0<<16);  // read out status
		     i=1;
		     k=1;
		     //i = pcie_send_1(hDev, i, k, px);
		     jseb2->Write(trport2, 1, buf_send);
		     // init receiver
		     //i = pcie_rec_2(hDev,0,1,nword,iprint,py);
		     jseb2->Read(trport2, nword, py);
		     // read out status
		     ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
		     buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_readback_status) + (0x0<<16);  // read out status
		     i=1;
		     k=1;
		     //i = pcie_send_1(hDev, i, k, px);
		     jseb2->Write(trport2, 1, buf_send);
		     usleep(10);
		     // read out 2 32 bit words
		     py = read_array;
		     //i = pcie_rec_2(hDev,0,2,nword,iprint,py);     // read out 2 32 bits words
		     jseb2->Read(trport2, nword, py);
		   } //if iext_trig

//		 printf(" read array %x %x %x %x \n", read_array[0], read_array[1], read_array[2], read_array[3]);
//		 
//		 printf("module %d receive data word = %x, %x \n", imod, read_array[0], read_array[1]);
//		 printf (" header word = %x \n" ,((read_array[0] & 0xffff) >> 8));
//		 printf (" module address = %d \n" ,(read_array[0] & 0x1f));
//		 printf (" upper adc rx pll locked %d\n", ((read_array[0] >>31) & 0x1));
//		 printf (" upper adc rx dpa locked %d\n", ((read_array[0] >>30) & 0x1));
//		 printf (" upper adc rx aligment %d\n", ((read_array[0] >>29) & 0x1));
//		 printf (" upper adc rx data valid %d\n", ((read_array[0] >>28) & 0x1));
//		 printf (" lower adc rx pll locked %d\n", ((read_array[0] >>27) & 0x1));
//		 printf (" lower adc rx dpa locked %d\n", ((read_array[0] >>26) & 0x1));
//		 printf (" lower adc rx aligment %d\n", ((read_array[0] >>25) & 0x1));
//		 printf (" lower adc rx data valid %d\n", ((read_array[0] >>24) & 0x1));
//		 printf (" link pll locked %d\n", ((read_array[0] >>23) & 0x1));
//		 printf (" clock pll locked %d\n", ((read_array[0] >>22) & 0x1));
//		 printf (" trigger bufer empty %d\n", ((read_array[0] >>21) & 0x1));
	       } //end module loop
	     


	     //      for (ijk=0; ijk<1000000000; ijk++) {
	     if (iext_trig != 1)buf_send[0]= (0x2<<8)+ sp_cntrl_timing+ ((sp_cntrl_l1)<<16);
	     i= 1;
	     k= 1;
	     if(iext_trig != 1) {
	       //i = pcie_send_1(hDev, i, k, px);
	       jseb2->Write(trport2, 1, buf_send);
	     }
	     usleep(100);
	     //      }
      
	     if(iext_trig == 1) {
	       int got_trig =0; //was ik
	       printf(" Wait for trigger \n");
	       while (got_trig != 1) {
		 imod = imod_start;
		 nword =1;
		 py = read_array;

		 //     command no-op to enable module output enable
		 // otherwise the LVDS input at controller will be floating high
		 ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
		 buf_send[0]=(imod<<11)+(ichip<<8)+(8) + (0x0<<16);  // read out status
		 i=1;
		 k=1;
		 //i = pcie_send_1(hDev, i, k, px);
		 jseb2->Write(trport2, 1, buf_send);

		 //i = pcie_rec_2(hDev,0,1,nword,iprint,py);       // init receiver
		 jseb2->Read(trport2, nword, py);
		 ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
		 buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_readback_status) + (0x0<<16);  // read out status
		 i=1;
		 k=1;
		 //i = pcie_send_1(hDev, i, k, px);
		 jseb2->Write(trport2, 1, buf_send);
		 usleep(10);
		 py = read_array;
		 //i = pcie_rec_2(hDev,0,2,nword,iprint,py);     // read out 2 32 bits words
		 jseb2->Read(trport2, nword, py);
		 if (((read_array[0] >>21) & 0x1) == 0) {
		   got_trig=1;
		   //		   printf(" trigger received %d \n", got_trig);
		 }
	       } //end while loop
	     } //end if ext trig

	     printf(" foound trigger\n");

	     ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
	     for (ik =0; ik<nmod; ik++) 
	       {
		 imod = ik+imod_start;
		 
		 //
		 buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_readback_transfer + (0<<16) ;
		 i= 1;
		 k= 1;
		 //i = pcie_send_1(hDev, i, k, px);
		 jseb2->Write(trport2, 1, buf_send);
		 usleep(100);

		 nread =2+(64*nsample/2)+1;
		 //i = pcie_rec_2(hDev,0,1,nread,iprint,py);     // read out 2 32 bits words
		 jseb2->Read(trport2, nread, py);
		 buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_readback_read + (1<<16) ;
		 i= 1;
		 k= 1;
		 //i = pcie_send_1(hDev, i, k, px);
		 jseb2->Write(trport2, 1, buf_send);
		 usleep(100);
		 //
		 //
		 //i = pcie_rec_2(hDev,0,2,nread,iprint,py);     // read out 2 32 bits words
		 jseb2->Read(trport2, nread, py);
		 if (iwrite == 1) {
		   fprintf(outf," %x\n", read_array[0]);
		   fprintf(outf," %x\n", read_array[1]);
		 }
		 
		 k=0;
		 for (is=0; is< (nread-2); is++) {
		   if(iwrite == 1) {
		     fprintf(outf," %8X", read_array[is+2]);
		   }
		   else {
		     if(is%8 ==0) printf(" %d ", is);
		     printf(" %x", read_array[is+2]);
		   }
		   k=k+1;
		   if(iwrite == 1) {
		     if((k%8) ==0) fprintf(outf,"\n");
		   }
		   else {
		     if((k%8) ==0) printf("\n");
		   }
		 }
		 if(iwrite == 1) fprintf(outf,"\n");
		 else printf("\n");
		 
		 if(iwrite != 1) {
		   for (is=0; is< nsample; is++ ) {
		     for (k=0; k< 32; k++) {
		       //        adc_data[(k*2)][is] = read_array[(is*32)+k+2] & 0xffff;
		       //        adc_data[((k*2)+1)][is] = (read_array[(is*32)+k+2] >>16) & 0xffff;
		       adc_data[(k*2)][is] = read_array[(k*nsample)+is+2] & 0xffff;
		       adc_data[((k*2)+1)][is] = (read_array[(k*nsample)+is+2] >>16) & 0xffff;
		     }
		   }

		   for (is=0; is<64; is++) {
		     printf(" channel %d ", is);
		     for (k=0; k<nsample; k++) {
		       printf(" %4x", adc_data[is][k]);
		     }
		     printf("\n");
		   }
		 } //end iwrite

	       } //end nmod loop k
	     //     fclose(inputf);

	     //it lowers the busy! (we think)

      buf_send[0]= (0x2<<8)+sp_cntrl_busyrst+ (0<<16);
      i= 1;
      k= 1;
      //i = pcie_send_1(hDev, i, k, buf_send);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);

      ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
      buf_send[0]=(imod<<11)+(ichip<<8)+(8) + (0x0<<16);  // 8 = "nothing" (grabbing the backplane bus)
      i=1;
      k=1;
      //i = pcie_send_1(hDev, i, k, buf_send);
      jseb2->Write(trport2, 1, buf_send);
      usleep(10);



	     //#define  sp_adc_readback_sub        4
	     //#define  sp_adc_readback_transfer   1
	     //#define  sp_adc_readback_read       2
	     //#define  sp_adc_readback_status     3
	     
	   } //end nevent a
       } //end nloop

 
   } // adcreadout_exttrig
   int ADCController::adcreadout_nopulse(int eventstotake){  }

/**
 * testadccontroller_readout
 * This reads data from the adc controller. It is the code in teststart in adcj.cc
 * This reads data with no external trigger
 */
int ADCController::testadccontroller_readout()
{
  PHDWORD  *buffer; // some work memory to hold the buffer
  oBuffer *ob;    
  int i,k,ia,ik;
  int status;
  int count;
  
  UINT32 nwords = 1;
  UINT32 nbytes = nwords*4;
  bool skipstatusread = true;

  if (debugFuncs){ 
    cout << "ADCController::testadccontroller_readout take "<< nevent << " events" << endl;
  }

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
		 //i = pcie_send_1(hDev, i, k, px);
		 jseb2->Write(trport2, 1, buf_send);
		 
		 // init receiver
		 nword =1;
		 py = read_array;
		 //i = pcie_rec_2(hDev,0,1,nword,iprint,py); // initialize port 2 for single read
		 jseb2->Read(trport2, nread, py);
		 // EJD 08/15/2017 read from port 2;
		 // TODO ; 08/15/2017 ; initialize jseb2 for single word read
		 // require init of receiver with InitReceiver or InitTRX(tr,nbytes,rcv,nbytes);
		 //jseb2->Receiver(int receiver, UINT32 nbytes);
		 // HERE this reads on the same port as the write
		 jseb2->InitReceiver(trport2, nword * 4); // requires nbytes argument 2
		 jseb2->Read(trport2, 1, py);

		 ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
		 buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_readback_status) + (0x0<<16);  // read out status
		 i=1;
		 k=1;
		 //i = pcie_send_1(hDev, i, k, px);
		 jseb2->Write(trport2, 1, buf_send);
		 usleep(10);

		     py = read_array;
		     printf("READ %d words \n",nword );
		     //i = pcie_rec_2(hDev,0,2,nword,iprint,py);     // read nread words - single word read
		     jseb2->Read(trport2, nread, py);
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



} // testadccontroller_readout
