
#include <Dcm2_JSEB2.h>

// Dcm2 includes
#include <Dcm2_Logger.h>
#include "status_strings.h"

// Boost includes
#include <boost/interprocess/permissions.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

// standard includes
#include <iostream>

// specializations for UINT16/32/64.
const UINT16 width_traits<UINT16>::mask = 0xffff;
const UINT32 width_traits<UINT32>::mask = 0xffffffff;
const UINT64 width_traits<UINT64>::mask = 0xffffffffffffffffULL;
const unsigned int width_traits<UINT16>::wordsPerQword = 4;
const unsigned int width_traits<UINT32>::wordsPerQword = 2;
const unsigned int width_traits<UINT64>::wordsPerQword = 1;

using namespace boost::interprocess;
using namespace std;
//#define DEBUG_DMA


Dcm2_JSEB2::Dcm2_JSEB2() {

  _verbosity = LOG;
  _hDev = NULL;

  _ppbuff_send = NULL;
  _ppbuff_recv = NULL;

  _pDma_send = NULL;
  _pDma_recv = NULL;
  
  _useDMA = true;
  _waitOnDMA = true;
  _timeOutDMA = true;

  _useLock = true;
  _hasLock = false;

  pcie_format = 0x00;

  return;
}

Dcm2_JSEB2::~Dcm2_JSEB2() {
  Close();
  return;
}

/// This method initializes the object for use and can
/// be called without interfering with use by another
/// process. It opens access to the device driver and allocates
/// some small in and out DMA buffers for use by the read/write
/// functions.
///
/// \return true if successful
///
bool Dcm2_JSEB2::Init() {

  // Ensure shared memory objects (mutex lock) should be group writeable
  // These are located in /dev/shm
  umask(002);

  // Retrieve the device resource description
  WD_PCI_CARD_INFO deviceInfo;
  BZERO(deviceInfo);
  deviceInfo.pciSlot = _deviceSlot;

  dwStatus = WDC_PciGetDeviceInfo(&deviceInfo);
  if (dwStatus != WD_STATUS_SUCCESS) {
    if (_verbosity >= SEVERE) {
      Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::Init()") 
	<< "Failed to retrieve the JSEB2 resource information; return code: " 
	<< Stat2Str(dwStatus) 
	<< Dcm2_Logger::Endl;
    }
    return false;
  }

  // From Chi:
  // NOTE: You can modify the device's resources information here, if
  //   necessary (mainly the deviceInfo.Card.Items array or the items number -
  //   deviceInfo.Card.dwItems) in order to register only some of the resources
  //   or register only a portion of a specific address space, for example.
  // Open a handle to the device
  //    deviceInfo.Card.Item[1].dwOptions |= WD_ITEM_DO_NOT_MAP_KERNEL;
  //    deviceInfo.Card.Item[1].I.Mem.dwBytes = 1000;
  //    deviceInfo.Card.Item[1].I.Mem.dwPhysicalAddr += 0x10;

  PJSEB2_DEV_CTX pDevCtx = NULL;
  WDC_DEVICE_HANDLE hDev = NULL;

  // Allocate memory for the JSEB2 device content 
  pDevCtx = (PJSEB2_DEV_CTX)malloc(sizeof(JSEB2_DEV_CTX));
  if (!pDevCtx) {
    if (_verbosity >= SEVERE) {
      Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::Init()") 
	<< "Failed allocating memory for JSEB2 device context" 
	<< Dcm2_Logger::Endl;
    }
    return false;
  }

  BZERO(*pDevCtx);

  // Open a WDC device handle
  //dwStatus = WDC_PciDeviceOpen(&hDev, &deviceInfo, pDevCtx, NULL, NULL, NULL);
  dwStatus = WDC_PciDeviceOpen(&hDev, &deviceInfo, pDevCtx);
  if (dwStatus != WD_STATUS_SUCCESS) {
    if (_verbosity >= SEVERE) {
      Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::Init()") 
	<< "Failed opening a WDC device handle; return code: " 
	<< Stat2Str(dwStatus) 
	<< Dcm2_Logger::Endl;
    }

    free(pDevCtx);

    return false;
  }

  free(pDevCtx);

  // Set handle in Dcm2_JSEB2 object
  _hDev = hDev;

  // JSEB2 FIFO length
  // Default buffer size (40 B)
  // 10 words by default as most common use is 
  // short status requests (will grow on demand)
  _dwTransmitDMABufSize = 40; 

  // Allocate Transmitting DMA Memory
  _dwTransmitDMAOptions = DMA_TO_DEVICE;
  // required to prevent system meltdowns on multiple allocations
  _dwTransmitDMAOptions |= DMA_ALLOW_64BIT_ADDRESS;
  // required for successful transmit dma (Altera DMA bug for transmits?)
  _dwTransmitDMAOptions |= DMA_KBUF_BELOW_16M;
  dwStatus = WDC_DMAContigBufLock(_hDev, &_ppbuff_send, _dwTransmitDMAOptions, 
				  _dwTransmitDMABufSize, &_pDma_send);
  if (dwStatus != WD_STATUS_SUCCESS) {
    if (_verbosity >= SEVERE) {
      Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::Init()") 
	<< "Failed locking a transmitting Contiguous DMA buffer; return code: "
	<< Stat2Str(dwStatus) 
	<< Dcm2_Logger::Endl;
    }

    _pDma_send = NULL;
    return false;
  }

  // Default buffer size (40 B)
  // 10 words by default as most common use 
  // is short status checks (will grow on demand)
  _dwReceiveDMABufSize = 40;

  // Allocate Receving DMA Memory
  _dwReceiveDMAOptions = DMA_FROM_DEVICE;
  // required to prevent system meltdowns on multiple allocations
  _dwReceiveDMAOptions |= DMA_ALLOW_64BIT_ADDRESS;
  dwStatus = WDC_DMAContigBufLock(_hDev, &_ppbuff_recv, _dwReceiveDMAOptions,
				  _dwReceiveDMABufSize, &_pDma_recv);
  if (WD_STATUS_SUCCESS != dwStatus) {
    if (_verbosity >= SEVERE) {
      Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::Init()") 
	<< "Failed locking a receiving Contiguous DMA buffer; return code: " 
	<< Stat2Str(dwStatus) 
	<< Dcm2_Logger::Endl;
    }
    
    _pDma_recv = NULL;
    return false;
  }

  return true;
}

/// This method closes the object's access to the device driver
/// and deallocates the dynamic DMA buffers first opened in Init().
///
/// \return true if successful
///
bool Dcm2_JSEB2::Close() {
  
  bool status = true;

  // Free transmit buffer memory
  if (!_pDma_send) {
    if (_verbosity >= DEBUG) {
      Dcm2_Logger::Cout(DEBUG,"Dcm2_JSEB2::Close()") 
	<< "Found NULL transmit buffer pointer" 
	<< Dcm2_Logger::Endl;
    }
  } else {
    dwStatus = WDC_DMABufUnlock(_pDma_send);
    if (WD_STATUS_SUCCESS != dwStatus) {
      if (_verbosity >= SEVERE) {
	Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::Close()") 
	  << "Failed unlocking a Contiguous DMA buffer; return code: " 
	  << Stat2Str(dwStatus) 
	  << Dcm2_Logger::Endl;
      }

      status = false;
    }
  }

  // Free receive buffer memory
  if (!_pDma_recv) {
    if (_verbosity >= DEBUG) {
      Dcm2_Logger::Cout(DEBUG,"Dcm2_JSEB2::Close()") 
	<< "Found NULL receive buffer pointer" 
	<< Dcm2_Logger::Endl;
    }
  } else {
    dwStatus = WDC_DMABufUnlock(_pDma_recv);
    if (dwStatus != WD_STATUS_SUCCESS) {
      if (_verbosity >= SEVERE) {
	Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::Close()") 
	  << "Failed unlocking a Contiguous DMA buffer; return code: " 
	  << Stat2Str(dwStatus) 
	  << Dcm2_Logger::Endl;
      }

      status = false;
    }
  }

  // Free device handle
  if (!_hDev) {
    if (_verbosity >= DEBUG) {
      Dcm2_Logger::Cout(DEBUG,"Dcm2_JSEB2::Close()") 
	<< "Found NULL device handle" 
	<< Dcm2_Logger::Endl;
    }
  } else {
    // Close the device
    dwStatus = WDC_PciDeviceClose(_hDev);
    if (WD_STATUS_SUCCESS != dwStatus) {
      if (_verbosity >= SEVERE) {
	Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::Close()") 
	  << "Failed to close the device handle; return code: " 
	  << Stat2Str(dwStatus) 
	  << Dcm2_Logger::Endl;
      }

      status = false;
    }
  }

  return status;
}

/// This method initializes the object for use as a
/// receiver JSEB2 inside an SEB (like would be used
/// as part of the EvB). Since there is only one EvB
/// process there is no use of the interprocess mutex.
/// DMAs are controlled by the process creating the
/// object and so are not managed by the object itself.
///
/// \return true if successful
///
bool Dcm2_JSEB2::SetReceiverDefaults() {

  SetUseLock(false);
  SetUseDMA(true);
  SetWaitOnDMA(false);
  SetTimeOutDMA(false);

  return true;
}

/// This method initializes the object for use as a
/// controller JSEB2 inside the DCM2 slow control server
/// host. Since there can be multiple processes running,
/// the JSEB2 is locked during use. Slow control transactions
/// let the object manage the DMA. Slow control DMAs timeout
/// if not progressing so that failure conditions can be
/// handled while maintaining the responsiveness for the DAQ.
///
/// \return true if successful
///
bool Dcm2_JSEB2::SetControllerDefaults() {

  SetUseLock(true);
  SetUseDMA(true);
  SetWaitOnDMA(true);
  SetTimeOutDMA(true);

  return true;
}

/// This method initializes the object for use as a
/// fake FEM JSEB2 inside one of the test stands. The
/// behavior is currently identical to the slow controller
/// settings.
///
/// \return true if successful
///
bool Dcm2_JSEB2::SetFEMDefaults() {

  SetUseLock(true);
  SetUseDMA(true);
  SetWaitOnDMA(true);
  SetTimeOutDMA(true);

  return true;
}

/// This method prepares the FPGA for a transaction.
///
/// \param[in] transmitter TR code [TR_ONE,TR_TWO,TR_BOTH]
/// \param[in] nbytesOut Data size to transmit
/// \param[in] receiver RCV code [RCV_ONE,RCV_TWO,RCV_BOTH]
/// \param[in] nbytesIn Data size to receive
///
/// \return true if successful
///
bool Dcm2_JSEB2::InitTRX(int transmitter, UINT32 nbytesOut, 
			 int receiver, UINT nbytesIn) {
  // Prepare the JSEB2 TRX port(s) for use.

  /// \todo The future JSEB2 fpga codes will require both FIFOs to be flushed
  /// prior to lengths being set on either ports, even for read-only or 
  /// write-only use. Unfortunately this breaks the independent TR & RCV 
  /// software design and requires trivial but extensive changes to the 
  /// controller software. I'm writing this single function so that if 
  /// something like this happens again it will be easier to implement
  /// throughout the slow control framework.

  // Flush both FIFOs
  //ClearTransmitterFIFO(transmitter);
  //ClearReceiverFIFO(receiver);

  // in case the received words are on a different
  // port as in loopback between ports (not sure if this
  // will ever happen in real world use)
  //if(transmitter != receiver)
  //  {
  //    ClearTransmitterFIFO(receiver);
  //    ClearReceiverFIFO(transmitter);
  //  }

  // set optical send/receive lengths

  //if(nbytesOut != 0)
  //  {
  //    SetTransmitLength(transmitter,nbytesOut);
  //  }
  if (nbytesIn != 0) { // going back to the old way of doing things for now
    ClearReceiverFIFO(receiver);
    SetReceiveLength(receiver,nbytesIn);
  }

  return true;
}

/// This method initalizes just the transmitter for sending words.
///
/// \param[in] transmitter TR code [TR_ONE,TR_TWO,TR_BOTH]
/// \param[in] nbytes Data size to transmit
///
/// \return true if successful
///
bool Dcm2_JSEB2::InitTransmitter(int transmitter, UINT32 nbytes) {
  ClearTransmitterFIFO(transmitter);
  SetTransmitLength(transmitter,nbytes);

  return true;
}

/// This method flushes the transmitter optical FIFO memory.
///
/// \param[in] transmitter TR code [TR_ONE,TR_TWO,TR_BOTH]
///
/// \return true if successful
///
bool Dcm2_JSEB2::ClearTransmitterFIFO(int transmitter) {

  if ( transmitter == TR_ONE ) {
    // initalize optical transmitter
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		    JSEB2_TR1_OFFSET, JSEB2_TRX_INIT_COMMAND);
  } else if ( transmitter == TR_TWO ) {
    // initalize optical transmitter
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		    JSEB2_TR2_OFFSET, JSEB2_TRX_INIT_COMMAND);
  } else if ( transmitter == TR_BOTH ) {
    // call initialization sequence on both #1 and #2
    ClearTransmitterFIFO(TR_ONE);
    ClearTransmitterFIFO(TR_TWO);
  }
  
  return true;
}

/// This method tells the optical transmitter how many
/// bytes will pass through it.
///
/// \param[in] transmitter TR code [TR_ONE,TR_TWO,TR_BOTH]
/// \param[in] nbytes Data size to transmit
///
/// \return true if successful
///
bool Dcm2_JSEB2::SetTransmitLength(int transmitter, UINT32 nbytes) {

  if ( transmitter == TR_ONE ) {
    // set the transfer size
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		    JSEB2_TR1_OFFSET, JSEB2_TRX_SIZE_COMMAND+nbytes);
  } else if ( transmitter == TR_TWO ) {
    // set the transfer size
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		    JSEB2_TR2_OFFSET, JSEB2_TRX_SIZE_COMMAND+nbytes);
  } else if ( transmitter == TR_BOTH ) {
    // call initialization sequence on both #1 and #2
    SetTransmitLength(TR_ONE,nbytes);
    SetTransmitLength(TR_TWO,nbytes);
  }

  return true;
}

/// This method initalizes just the receiver for reading back words.
///
/// \param[in] receiver RCV code [RCV_ONE,RCV_TWO,RCV_BOTH]
/// \param[in] nbytes Data size to receive
///
/// \return true if successful
///
bool Dcm2_JSEB2::InitReceiver(int receiver, UINT32 nbytes) {
  ClearReceiverFIFO(receiver);
  SetReceiveLength(receiver,nbytes);
  
  return true;
}

/// This method flushes the receiver optical FIFO memory.
///
/// \param[in] receiver RCV code [RCV_ONE,RCV_TWO,RCV_BOTH]
///
/// \return true if successful
///
bool Dcm2_JSEB2::ClearReceiverFIFO(int receiver) {

  if ( receiver == RCV_ONE ) {
    // initalize optical transmitter
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		    JSEB2_RCV1_OFFSET, JSEB2_TRX_INIT_COMMAND);
  } else if ( receiver == RCV_TWO ) {
    // initalize optical transmitter
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		    JSEB2_RCV2_OFFSET, JSEB2_TRX_INIT_COMMAND);
  } else if ( receiver == RCV_BOTH ) {
    // call initialization sequence on both #1 and #2
    ClearReceiverFIFO(RCV_ONE);
    ClearReceiverFIFO(RCV_TWO);
  }

  return true;
}

/// This method tells the optical receiver how many
/// bytes will pass through it.
///
/// \param[in] receiver RCV code [RCV_ONE,RCV_TWO,RCV_BOTH]
/// \param[in] nbytes Data size to receive
///
/// \return true if successful
///
bool Dcm2_JSEB2::SetReceiveLength(int receiver, UINT32 nbytes) {

  if ( receiver == RCV_ONE ) {
    // set the transfer size
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		    JSEB2_RCV1_OFFSET, JSEB2_TRX_SIZE_COMMAND + nbytes);
  } else if ( receiver == RCV_TWO ) {
    // set the transfer size
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		    JSEB2_RCV2_OFFSET, JSEB2_TRX_SIZE_COMMAND + nbytes);
  } else if ( receiver == RCV_BOTH ) {
    // call initialization sequence on both #1 and #2
    SetReceiveLength(RCV_ONE,nbytes);
    SetReceiveLength(RCV_TWO,nbytes);
  }

  return true;
}

/// This method transmits out data via DMA. It begins by
/// relaying the physical memory address and size to the FPGA's
/// DMA engine. It then starts the DMA with options for the PCIe
/// packet formatting. If requested the code manages the DMA
/// progress and will timeout if it stalls. Otherwise, it starts
/// the DMA and returns immediately with the DMA still running.
///
/// \param[in] transmitter TR code [TR_ONE,TR_TWO,TR_BOTH]
/// \param[in] nbytes Data size to transmit
/// \param[in] pDma Pointer to the DMA buffer
///
/// \return true if successful
///
bool Dcm2_JSEB2::Transmit(int transmitter, UINT32 nbytes, WD_DMA *pDma) {
  
  // lower 32bits of dma buffer address
  WDC_WriteAddr32(_hDev, 
		  JSEB2_COMMAND_ADDRSPACE, 
		  JSEB2_DMA_ADDR_LOW_OFFSET, 
		  (pDma->Page->pPhysicalAddr & 0xffffffff));

  // upper 32bits of dma buffer address
  u32data = ((pDma->Page->pPhysicalAddr >> 32) & 0xffffffff);
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		  JSEB2_DMA_ADDR_HIGH_OFFSET, u32data);

  // 64bit addresses require different pcie packet formatting
  if (u32data == 0) {
    pcie_format = FORMAT_3DW_MEM_TO_OPT; // 3DW formating required below 4GB
  } else {
    pcie_format = FORMAT_4DW_MEM_TO_OPT; // 4DW formating required above 4GB
  }

  // set dma size in nbytes
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, JSEB2_DMA_SIZE_OFFSET, nbytes);

  // sync memory with cpu cache
  // ejd 09/12/2016 try adding back in
  DMASyncCpu(pDma, nbytes);

  // select transmitter and start dma
  if ( transmitter == TR_ONE ) {
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, JSEB2_DMA_COMMAND_OFFSET, 
		    JSEB2_DMA_ON_TRX1_COMMAND | pcie_format);
  } else if ( transmitter == TR_TWO ) {
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, JSEB2_DMA_COMMAND_OFFSET, 
		    JSEB2_DMA_ON_TRX2_COMMAND | pcie_format);
  } else if ( transmitter == TR_BOTH ) {
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, JSEB2_DMA_COMMAND_OFFSET, 
		    JSEB2_DMA_ON_TRX3_COMMAND | pcie_format);
  }

  if (_waitOnDMA) {
    // wait for transmit to complete
    UINT32 polls;
    UINT32 waits;
    UINT32 oldBytesRemaining;
    UINT32 newBytesRemaining;
    
    polls = 0;
    waits = 0;
    oldBytesRemaining = 0;
    
    while (!HasDMACompleted()) {
      if (_timeOutDMA) {
	// new dma progress count
	newBytesRemaining = GetBytesLeftToDMA();

	// if progress has been made, reset counters
	if (newBytesRemaining != oldBytesRemaining) {
	  polls = 0;
	  waits = 0;
	} else if(polls > 40000) {
	  // else if counter too high abort

	  // stop polling... wait for the controller FIFO to drain (2 ms)
	  WDC_Sleep(2000,WDC_SLEEP_BUSY);
	  ++waits;
	}

	// by now the controller should have drained
	if (waits > 10) {
	  if (_verbosity >= WARNING) {
	    Dcm2_Logger::Cout(WARNING,"Dcm2_JSEB2::Transmit()") 
	      << "DMA timeout with " << newBytesRemaining << " of " 
	      << nbytes << " bytes missing" 
	      << Dcm2_Logger::Endl;
	  }

	  AbortDMA();
	  return false;
	}

	// update for next cycle
	oldBytesRemaining = newBytesRemaining;
	++polls;
      }
    }
      
    // sync memory with cpu cache
    // ejd 09/12/16 try adding back in
    DMASyncIo(pDma,nbytes); // outgoing sync not needed
  }

  return true;
}

/// This method receives in data via DMA. It begins by
/// relaying the physical memory address and size to the FPGA's
/// DMA engine. It then starts the DMA with options for the PCIe
/// packet formatting. If requested the code manages the DMA
/// progress and will timeout if it stalls. Otherwise, it starts
/// the DMA and returns immediately with the DMA still running.
///
/// \param[in] receiver RCV code [RCV_ONE,RCV_TWO,RCV_BOTH]
/// \param[in] nbytes Data size to receive
/// \param[in,out] pDma Pointer to the DMA buffer
///
/// \return true if successful
///
bool Dcm2_JSEB2::Receive(int receiver, UINT32 nbytes, WD_DMA *pDma) {
  
  // lower 32bits of dma buffer address
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		  JSEB2_DMA_ADDR_LOW_OFFSET, 
		  (pDma->Page->pPhysicalAddr & 0xffffffff));

  // upper 32bits of dma buffer address
  u32data = ((pDma->Page->pPhysicalAddr >> 32) & 0xffffffff);
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		  JSEB2_DMA_ADDR_HIGH_OFFSET, u32data);
#ifdef DEBUG_DMA
  cout << "JSEB2:Receive u32data = " << u32data << endl;
#endif
  // 64bit addresses require different pcie packet formatting
  if (u32data == 0) {
    pcie_format = FORMAT_3DW_OPT_TO_MEM; // 3DW formating required below 4GB
  } else {
    pcie_format = FORMAT_4DW_OPT_TO_MEM; // 4DW formating required above 4GB
  }
#ifdef DEBUG_DMA
  cout << "JSEB2:Receive set dma size " << nbytes << endl;
#endif
  // set dma size in nbytes
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		  JSEB2_DMA_SIZE_OFFSET, nbytes);
#ifdef DEBUG_DMA
  cout << "JSEB2::Receive DMA sync" << endl;
#endif
  // sync memory with cpu cache
  // ejd 09/12/2016 enable DMASyncCpu
  DMASyncCpu(pDma, nbytes); // incoming sync not needed

  // select receiver and start dma
  if (receiver == RCV_ONE) {
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		    JSEB2_DMA_COMMAND_OFFSET, 
		    JSEB2_DMA_ON_TRX1_COMMAND | pcie_format);
  } else if (receiver == RCV_TWO) {
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		    JSEB2_DMA_COMMAND_OFFSET, 
		    JSEB2_DMA_ON_TRX2_COMMAND | pcie_format);
  } else if (receiver == RCV_BOTH) {
    WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		    JSEB2_DMA_COMMAND_OFFSET, 
		    JSEB2_DMA_ON_TRX3_COMMAND | pcie_format);
  }
#ifdef DEBUG_DMA
  cout << "jseb2::Read start wait on DMA " << endl;
#endif
  if (_waitOnDMA) {
    // wait for transmit to complete
    UINT32 polls;
    UINT32 waits;
    UINT32 oldBytesRemaining;
    UINT32 newBytesRemaining;

    polls = 0;
    waits = 0;
    oldBytesRemaining = 0;
#ifdef DEBUG_DMA
    cout << "jseb2: test DMC completion " << endl;
#endif
    while (!HasDMACompleted()) {
#ifdef DEBUG_DMA
    cout << "jseb2: test DMC NOT completed " << endl;
#endif
      if (_timeOutDMA) {
	// new dma progress count
	newBytesRemaining = GetBytesLeftToDMA();
#ifdef DEBUG_DMA
	cout << "jseb2: DMA bytesRemaining =  " << newBytesRemaining << " oldBR = " << oldBytesRemaining  << endl;
#endif
	// if progress has been made, reset counters
	if (newBytesRemaining != oldBytesRemaining) {
	  polls = 0;
	  waits = 0;
	} else if (polls > 40000) { // else if counter too high abort
	  // stop polling... wait for the controller FIFO to drain (2 ms)
	  WDC_Sleep(2000,WDC_SLEEP_BUSY);
	  #ifdef DEBUG_DMA
	  cout << "jseb2::waits= " << waits << " oldbr = " << oldBytesRemaining << " newBR = " << newBytesRemaining << endl;
          #endif
	  ++waits;
	}

	// by now the controller should have drained
	if (waits > 10) {
	  AbortDMA();

	  if (_verbosity >= WARNING) {
	    Dcm2_Logger::Cout(WARNING,"Dcm2_JSEB2::Receive()") 
	      << "DMA timeout with " << newBytesRemaining 
	      << " of " << nbytes << " bytes missing" 
	      << Dcm2_Logger::Endl;
	  }

	  return false;
	}

	// update for next cycle
	oldBytesRemaining = newBytesRemaining;
	++polls;
      } // has dma timeout
    } // had dma completed
#ifdef DEBUG_DMA
    cout << "JSEB2: DMA complete " << endl;
#endif      
    // sync memory with cpu cache
    // ejd 09/12/2016 enable DMASyncIo
    DMASyncIo(pDma, nbytes);
  }
  
  return true;
}

/// This method syncs the processor cache. At one time
/// I was trying things to force partial cache syncing.
/// But this is now deprecated and should be removed.
///
/// \param[in] pDma Pointer to the DMA buffer
/// \param[in] nbytes Data size
///
/// \return true if successful
///
void Dcm2_JSEB2::DMASyncCpu(WD_DMA *pDma, unsigned int nbytes) {
  //WDC_DMASyncCpu(pDma);

  return;
}

/// This method syncs the processor cache. At one time
/// I was trying things to force partial cache syncing.
/// But this is now deprecated and should be removed.
///
/// \param[in] pDma Pointer to the DMA buffer
/// \param[in] nbytes Data size
///
/// \return true if successful
///
void Dcm2_JSEB2::DMASyncIo(WD_DMA *pDma, unsigned int nbytes) {
  //WDC_DMASyncIo(pDma);

  return;
}

bool Dcm2_JSEB2::SetPciSlot(WD_PCI_SLOT islot) {
  _deviceSlot = islot;
  return true;
}

/// This method resizes the internal transmit DMA buffer
/// base on the size of data to send. It deallocates the current
/// DMA buffer and then allocates a new DMA buffer of the
/// requested length.
///
/// \param[in] nbytes Data size
///
/// \return true if successful
///
bool Dcm2_JSEB2::ResetTransmitBufferSize(unsigned int nbytes) {

  if (_verbosity >= DEBUG) {
    Dcm2_Logger::Cout(DEBUG,"Dcm2_JSEB2::ResetTransmitBufferSize()") 
      << "DMA buffer resizing from " << _dwTransmitDMABufSize 
      << " to " << nbytes << " bytes" 
      << Dcm2_Logger::Endl;
  }

  // Free transmit buffer memory
  dwStatus = WDC_DMABufUnlock(_pDma_send);
  if (dwStatus != WD_STATUS_SUCCESS) {
    if (_verbosity >= SEVERE) {
      Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::ResetTransmitBufferSize()") 
	<< "Failed unlocking the transmitting DMA buffer; return code: " 
	<< Stat2Str(dwStatus) 
	<< Dcm2_Logger::Endl;
    }
    return false;
  }
 
  // reset object information on DMA buffer sizes
  _dwTransmitDMABufSize = nbytes;
  
  // Allocate Transmitting DMA Memory
  dwStatus = WDC_DMAContigBufLock(_hDev, &_ppbuff_send, _dwTransmitDMAOptions, 
				  _dwTransmitDMABufSize, &_pDma_send);
  if (dwStatus != WD_STATUS_SUCCESS) {
    if (_verbosity >= SEVERE) {
      Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::ResetTransmitBufferSize()") 
	<< "Failed relocking a transmitting DMA buffer; return code: " 
	<< Stat2Str(dwStatus) 
	<< Dcm2_Logger::Endl;
    }

    return false;
  }  

  return true;
}

/// This method resizes the internal receive DMA buffer
/// base on the size of data to read back. It deallocates the current
/// DMA buffer and then allocates a new DMA buffer of the
/// requested length.
///
/// \param[in] nbytes Data size
///
/// \return true if successful
///
bool Dcm2_JSEB2::ResetReceiveBufferSize(unsigned int nbytes) {

  if (_verbosity >= DEBUG) {
    Dcm2_Logger::Cout(DEBUG,"Dcm2_JSEB2::ResetReceiveBufferSize()") 
      << "DMA buffer resizing from " << _dwTransmitDMABufSize 
      << " to " << nbytes << " bytes" 
      << Dcm2_Logger::Endl;
  }

  // Free receive buffer memory
  dwStatus = WDC_DMABufUnlock(_pDma_recv);
  if (dwStatus != WD_STATUS_SUCCESS) {
    if (_verbosity >= SEVERE) {
      Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::ResetTransmitBufferSize()") 
	<< "Failed unlocking the receiving DMA buffer; return code: " 
	<< Stat2Str(dwStatus) 
	<< Dcm2_Logger::Endl;
    }

    return false;
  }

  // reset object information on DMA buffer sizes
  _dwReceiveDMABufSize = nbytes;
  
  // Allocate Receving DMA Memory
  dwStatus = WDC_DMAContigBufLock(_hDev, &_ppbuff_recv, _dwReceiveDMAOptions, 
				  _dwReceiveDMABufSize, &_pDma_recv);
  if (dwStatus != WD_STATUS_SUCCESS) {
    if (_verbosity >= SEVERE) {
      Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::ResetTransmitBufferSize()") 
	<< "Failed relocking a receiving DMA buffer; return code: " 
	<< Stat2Str(dwStatus) 
	<< Dcm2_Logger::Endl;
    }

    return false;
  }  
  
  return true;
}

/// This method tells the FPGA to interupt and end the current DMA
/// engine process.
///
/// \return true if successful
///
bool Dcm2_JSEB2::AbortDMA() {

  // tell dma engine to abort
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		  JSEB2_DMA_ABORT_OFFSET, JSEB2_DMA_ABORT_COMMAND);

  // clear dma abort
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		  JSEB2_DMA_ABORT_OFFSET, JSEB2_DMA_ABORT_CLEAR);

  return true;
}

/// This method is a wrapper for the WinDriver command of the same name and
/// was used during the mixed 32bit/64bit development to prevent the compiler
/// from breaking up 64bit access into two 32bit commands.
///
/// \param[in] dwAddrSpace Register address space
/// \param[in] dwOffset Register address offset
/// \param[in] u64data Register word
///
void Dcm2_JSEB2::WriteAddr64(DWORD dwAddrSpace, DWORD dwOffset, UINT64 u64data) {

  //---Explaination----------------------------------------//
  // In a 32bit executable on the 64bit OS, the Windriver
  // ReadAddr64() command is broken into two 32bit reads.
  // Either Windriver or the JSEB2 moves the data forward
  // on every read, meaning the 2x32bit reads miss 1/2 the
  // data. This assembly mimicks the compiled 64bit code
  // and forces the single 64bit read.

  // The replacement assembly code somehow screws up the timing
  // and will corrupt reads (probably writes too) every once in a while. 
  // Reverting to the old way of doing things we are now 64bit-only.

  // ---replace with 64bit assembly---
  WDC_WriteAddr64(_hDev, dwAddrSpace, dwOffset, u64data);

  /*
  // get a pointer to the register address and offset
  WDC_ADDR_DESC *pAddrDesc = WDC_GET_ADDR_DESC(_hDev, dwAddrSpace);
  UINT64 *pmem = (UINT64*)(WDC_MEM_DIRECT_ADDR(pAddrDesc)+(UPTR)(dwOffset));

  // get a place to hold the result
  volatile UINT64 v64data = u64data;
  volatile UINT64* pv64data = &v64data;

  // call the assembly to read the data in
  __asm__ __volatile__
    (
     "movq (%0), %%xmm0 \n\t"
     "movq %%xmm0, (%1) \n\t"
     : "=&r" (pv64data), "=&r" (pmem)
     : "0" (pv64data), "1" (pmem)
     : "%xmm0"
     );
  */

  return;
}

/// This method is a wrapper for the WinDriver command of the same name and
/// was used during the mixed 32bit/64bit development to prevent the compiler
/// from breaking up 64bit access into two 32bit commands.
///
/// \param[in] dwAddrSpace Register address space
/// \param[in] dwOffset Register address offset
/// 
/// \return UINT64 Register word
///
UINT64 Dcm2_JSEB2::ReadAddr64(DWORD dwAddrSpace, DWORD dwOffset) {

  //---Explaination----------------------------------------//
  // In a 32bit executable on the 64bit OS, the Windriver
  // ReadAddr64() command is broken into two 32bit reads.
  // Either Windriver or the JSEB2 moves the data forward
  // on every read, meaning the 2x32bit reads miss 1/2 the
  // data. This assembly mimicks the compiled 64bit code
  // and forces the single 64bit read.

  // The replacement assembly code somehow screws up the timing
  // and will corrupt reads every once in a while. Reverting to the
  // old way of doing things we are now 64bit-only.

  // ---replace with 64bit assembly---
  UINT64 u64data = 0x0;
  WDC_ReadAddr64(_hDev, dwAddrSpace, dwOffset, &u64data);
  return u64data;

  // get a pointer to the register address and offset
  //WDC_ADDR_DESC *pAddrDesc = WDC_GET_ADDR_DESC(_hDev, dwAddrSpace);
  //UINT64 *pmem = (UINT64*)(WDC_MEM_DIRECT_ADDR(pAddrDesc)+(UPTR)(dwOffset));

  // get a place to hold the result
  //volatile UINT64 v64data = 1;
  //volatile UINT64* pv64data = &v64data;

  // call the assembly to read the data in
  //__asm__ __volatile__
  //  (
  //   "movq (%0), %%xmm0 \n\t"
  //   "movq %%xmm0, (%1) \n\t"
  //   : "=&r" (pmem), "=&r" (pv64data)
  //   : "0" (pmem), "1" (pv64data)
  //   : "%xmm0"
  //   );

  //return v64data;
}

/// This method composes and returns the DAQ status word for the JSEB2
///
/// \return UINT32 status word
///
UINT32 Dcm2_JSEB2::GetStatus() {

  u32data = 0x0;
  return u32data;
}

/// This method gets the FPGA transmitter register value.
///
/// \param[in] transmitter TR code [TR_ONE,TR_TWO,TR_BOTH]
///
/// \return UINT32 register word
///
UINT32 Dcm2_JSEB2::GetTransmitterStatusWord(int transmitter) {
 
  if (transmitter == TR_ONE) {
    WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, JSEB2_TR1_OFFSET, &u32data);
  } else if (transmitter == TR_TWO) {
    WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, JSEB2_TR2_OFFSET, &u32data);
  }

  return u32data;
}

/// This method gets the FPGA receiver register value.
///
/// \param[in] receiver RCV code [RCV_ONE,RCV_TWO,RCV_BOTH]
///
/// \return UINT32 register word
///
UINT32 Dcm2_JSEB2::GetReceiverStatusWord(int receiver) {

  if (receiver == RCV_ONE) {
    WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, JSEB2_RCV1_OFFSET, &u32data);
  } else if (receiver == RCV_TWO) {
    WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, JSEB2_RCV2_OFFSET, &u32data);
  }

  return u32data;
}

/// This method gets the FPGA mode register value.
///
/// \return UINT32 register word
///
UINT32 Dcm2_JSEB2::GetModeStatusWord() {

  WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		 JSEB2_HOLD_MODE_OFFSET, &u32data);

  return u32data;
}

/// This method gets the FPGA DMA progress register
///
/// \return UINT32 number of bytes
///
UINT32 Dcm2_JSEB2::GetBytesLeftToDMA() {

  WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		 JSEB2_DMA_SIZE_OFFSET, &u32data);

  return u32data;
}

/// This method checks the DMA completion bit in the DMA status word
///
/// \return UINT32 true if DMA is complete
///
bool Dcm2_JSEB2::HasDMACompleted() {
  // check for DMA completed bit
  if ((GetDMAStatusWord() & 0x80000000) != 0) {
    return false;
  }

  return true;
}

/// This method gets the FPGA DMA command register value
///
/// \return UINT32 register word
///
UINT32 Dcm2_JSEB2::GetDMAStatusWord() {

  WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		 JSEB2_DMA_COMMAND_OFFSET, &u32data);
  return u32data;
}

/// This method gets the number of words on the optical receiver FIFO. This
/// requires a new FPGA code from Chi that has not yet been implemented and
/// so bails if used.
///
/// \param[in] trx TRX port number [RCV_ONE,RCV_TWO,RCV_BOTH]
///
/// \return UINT32 number of words
///
UINT32 Dcm2_JSEB2::GetNumberOfBytesOnFIFO(int trx) {

  if (_verbosity >= SEVERE) {
    Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::GetNumberOfWordsOnFIFO") 
      << "This function is still unsupported by the current 1008 FPGA code" 
      << Dcm2_Logger::Endl;
  }

  return 0;

  // someday the JSEB2 FPGA will keep track of this stuff

  // UINT64 u64data = ReadAddr64(JSEB2_COMMAND_ADDRSPACE, JSEB2_WORD_COUNT_OFFSET);
  // UINT32 nwords = 0;
  // if (trx == RCV_ONE) {
  //   nwords += ((u64data >> 47) & 0x3FFF);
  // } else if (trx == RCV_TWO) {
  //   nwords += ((u64data >> 32) & 0x7FFF);
  // } else if (trx == RCV_BOTH) { // maybe?
  //   if (_verbosity >= SEVERE) {
  //     Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::GetNumberOfWordsOnFIFO") 
  // 	<< "This function is not yet reverse enginneered for TRX=3 mode" 
  // 	<< Dcm2_Logger::Endl;
  //   }
  //   return 0;
  //   nwords += ((u64data >> 47) & 0x3FFF);
  //   nwords += ((u64data >> 32) & 0x7FFF);
  // }
  // return nwords*4 // or*8;
}

/// This method gets the words off on the optical receiver FIFO.
///
/// \param[in] trx TRX port number [RCV_ONE,RCV_TWO,RCV_BOTH]
/// \param[in] nwords number of words to dump out
/// \param[in/out] pwords storage for the words
///
/// \return true if successful
///
bool Dcm2_JSEB2::GetContentOfFIFO(int trx, unsigned int nwords, UINT32 *pwords) {
  
  if (_verbosity >= SEVERE) {
    Dcm2_Logger::Cout(SEVERE,"Dcm2_JSEB2::GetContentOfFIFO") 
      << "This function is still unsupported." 
      << Dcm2_Logger::Endl;
  }

  return false;

  // odd number of words through the dcm group leaves a single word
  // stuck someplace I haven't been able to find.

  // storage for data on FIFO buffers
  UINT64 longsTRX1[1025] = {0x0};
  UINT64 *plongsTRX1 = (UINT64*)&longsTRX1;

  UINT64 longsTRX2[1025] = {0x0};
  UINT64 *plongsTRX2 = (UINT64*)&longsTRX2;

  // storage for reconstituted words
  UINT32 wordsOnJSEB2[2049] = {0x0};

  //unsigned int nwordsOnJSEB2 = bytesLeftOnJSEB2/4;
  //unsigned int nlongsOnJSEB2 = nwordsOnJSEB2/2;
  //if (nwordsOnJSEB2%2 != 0) {
  //  nlongsOnJSEB2++;
  // }
      
  // toggle into direct read mode
  bool usedDMA = GetUseDMA();
  if (usedDMA) SetUseDMA(false);

  // grab the data directly off the registers
  Read(RCV_ONE,1025,plongsTRX1);
  Read(RCV_TWO,1025,plongsTRX2);

  // return to previous setting
  if (usedDMA) SetUseDMA(true);

  for (unsigned int ilong = 0; ilong < 1025; ++ilong) {

    cout << "TRX1 & TRX2: ";
    cout.width(16); cout.fill('0'); 
    cout << hex << longsTRX1[ilong] << dec << " ";
    cout.width(16); cout.fill('0'); 
    cout << hex << longsTRX2[ilong] << dec << endl;
    
    // translate the register words into data words
    UINT64 byteA = ((longsTRX1[ilong] & 0x0000FFFF00000000) >> 32);
    UINT64 byteB = ((longsTRX1[ilong] & 0x000000000000FFFF)      );
    UINT64 byteC = ((longsTRX2[ilong] & 0xFFFF000000000000) >> 48);
    UINT64 byteD = ((longsTRX2[ilong] & 0x0000FFFF00000000) >> 32);
    
    UINT32 word_odd  = ((byteD << 16) + byteB);
    UINT32 word_even = ((byteC << 16) + byteA);
	  
    wordsOnJSEB2[2*ilong] = word_odd;
    wordsOnJSEB2[2*ilong+1] = word_even;
  }

  // pass through the translated words to find the last real data word
  for (unsigned int i = 0; i < 2049; ++i) {
    
    unsigned int iword1 = 2048-i;
    unsigned int iword2 = 2048-i-2;

    if (wordsOnJSEB2[iword1] != wordsOnJSEB2[iword2]) {
      cout << "last word = " << iword2 << " " 
	   << hex << wordsOnJSEB2[iword2] << dec << endl;
      break;
    }
  }

  // print out new words...
  cout << "   (--) ";
  for (unsigned int iword = 1; iword < 2049+1; ++iword) {
    cout.width(8); cout.fill('0'); 
    cout << hex << wordsOnJSEB2[iword-1] << dec << " ";
    if (iword%8 == 0) {
      cout << endl; 
      if (iword < 2049) cout << "   (--) ";
    }
  }
  cout << endl;

  return true;
}

/// This method establishes and sets the interprocess mutex lock for the
/// JSEB2 device. If the mutex is already locked, the code will wait
/// indefinitely at this point.
///
/// \return true if successfully locked
///
bool Dcm2_JSEB2::GetLock() {

  // open or create the mutex
  named_mutex mutex(open_or_create, GetID().c_str(), permissions(0664));
  mutex.lock();
  _hasLock = true;

  return true;
}

/// This method establishes and sets the interprocess mutex lock for the
/// JSEB2 device, but bails if enough time has passed.
///
/// \param[in] sec Timeout threshold in seconds
///
/// \return true if successfully locked
///
bool Dcm2_JSEB2::WaitForLock(long sec) {

  // open or create the mutex
  named_mutex mutex(open_or_create, GetID().c_str(), permissions(0664));

  boost::posix_time::ptime 
    now(boost::posix_time::second_clock::universal_time());
  boost::posix_time::time_duration wait = boost::posix_time::seconds(sec);
  boost::posix_time::ptime timeout = now + wait;

  bool status = mutex.timed_lock(timeout);
  if (!status) {
    _hasLock = false;
    return false;
  }

  _hasLock = true;
  
  return true;
}

/// This method removes the interprocess mutex from the system.
///
/// \return true if successful
///
bool Dcm2_JSEB2::DestroyLock() {
  // open or create the mutex
  named_mutex mutex(open_or_create, GetID().c_str(), permissions(0664));
  mutex.remove(GetID().c_str());
  _hasLock = false;

  return true;
}

/// This method releases the lock breifly and then re-establishes the lock.
///
/// \return true if successful
///
bool Dcm2_JSEB2::PauseLock() {
  // Give up and then rerequest lock
  SetFree();
  GetLock();
  return true;
}

/// This method releases the lock and pauses the thread for 1 us to explicitly 
/// enforce process sharing of the JSEB2 resource.
///
/// \return true if successful
///
bool Dcm2_JSEB2::SetFree() {  
  // open or create the mutex
  named_mutex mutex(open_or_create, GetID().c_str(), permissions(0664));
  mutex.unlock();
  _hasLock = false;
  
  // 1 microsecond wait force before relocking
  WDC_Sleep(1,WDC_SLEEP_BUSY);
  return true;
}

/// This method instructs the FPGA to return a hold command to the
/// partitioner board when the FIFO contents grow past a set threshold.
/// Argument must be less than 0x7fff bytes.
///
/// \param[in] maxBytes Hold threshold in bytes
///
/// \return true if successful
///
bool Dcm2_JSEB2::EnableHolds(UINT16 maxBytes = 0x7fff) {

  _holdByteLimit = maxBytes;

  // tell the two transmitters to return holds
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		  JSEB2_TR1_OFFSET, JSEB2_TRX_HOLD_COMMAND);
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		  JSEB2_TR2_OFFSET, JSEB2_TRX_HOLD_COMMAND);

  // tell fpga when to exert holds
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		  JSEB2_HOLD_MODE_OFFSET, _holdByteLimit);
  
  return true;
}

/// This method manually transmits the hold command.
///
/// \return true if successful
///
bool Dcm2_JSEB2::RaiseHold() {
  // manually excert hold
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		  JSEB2_HOLD_MODE_OFFSET,  
		  JSEB2_HOLD_MODE_COMMAND | _holdByteLimit );
  return true;
}

/// This method manually releases the hold command.
///
/// \return true if successful
///
bool Dcm2_JSEB2::ReleaseHold() {
  // go back to auto holds
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		  JSEB2_HOLD_MODE_OFFSET, _holdByteLimit );
  return true;
}

/// This method returns the JSEB2 to the default mode with respect
/// to holds.
///
/// \return true if successful
///
bool Dcm2_JSEB2::DisableHolds() {
  // tell the two transceivers to no longer return holds
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, JSEB2_TR1_OFFSET, 0x0);
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, JSEB2_TR2_OFFSET, 0x0);
  
  // clear the hold mode offset
  WDC_WriteAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, JSEB2_HOLD_MODE_OFFSET, 0x0);
  
  return true;
}

/// This method allows quick error recovery for the JSEB2 card. It stops
/// DMAs and flushes FIFOs.
///
/// \return true if successful
///
bool Dcm2_JSEB2::CleanUp(int trx) {

  if (_verbosity >= WARNING) {
    Dcm2_Logger::Cout(WARNING,"Dcm2_JSEB2::CleanUp()")
      << "Aborting DMA" 
      << Dcm2_Logger::Endl;
  }
  AbortDMA();

  if (_verbosity >= WARNING) {
    Dcm2_Logger::Cout(WARNING,"Dcm2_JSEB2::CleanUp()") 
      << "Flushing Transmitter FIFO on " << trx 
      << Dcm2_Logger::Endl;
  }
  ClearTransmitterFIFO(trx);
  
  if (_verbosity >= WARNING) {
    Dcm2_Logger::Cout(WARNING,"Dcm2_JSEB2::CleanUp()") 
      << "Flushing Receiver FIFO on " << trx 
      << Dcm2_Logger::Endl;
  }
  ClearReceiverFIFO(trx);

  return true;
}

/// This method dumps a long form status print to the logger.
///
/// \return true if successful
///
bool Dcm2_JSEB2::Print() {

  if (_verbosity >= LOG) {
    string align = "      ";

    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") << align << Dcm2_Logger::Endl;

    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << GetID() << " - JSEB2 card on bus " << GetPciBus() 
      << " in slot " << GetPciSlot() 
      << Dcm2_Logger::Endl;

    UINT32 word0 = 0x0;
    UINT32 word1 = 0x0;
    UINT32 word2 = 0x0;
    UINT32 word3 = 0x0;
    UINT32 word4 = 0x0;
    UINT32 word5 = 0x0;
    UINT32 word6 = 0x0;

    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "DMA command register word:    0x";
    WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		   JSEB2_DMA_COMMAND_OFFSET, &word0);
    cout.width(8); cout.fill('0'); 
    cout << hex << word0 << dec 
	 << Dcm2_Logger::Endl;

    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "DMA size register word:       0x";
    WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		   JSEB2_DMA_SIZE_OFFSET, &word1);
    cout.width(8); cout.fill('0'); 
    cout << hex << word1 << dec 
	 << Dcm2_Logger::Endl;
    
    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "Transmitter #1 register word: 0x";
    WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		   JSEB2_TR1_OFFSET, &word2);
    cout.width(8); cout.fill('0'); 
    cout << hex << word2 << dec 
	 << Dcm2_Logger::Endl;
    
    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "Transmitter #2 register word: 0x";
    WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		   JSEB2_TR2_OFFSET, &word3);
    cout.width(8); cout.fill('0'); 
    cout << hex << word3 << dec 
	 << Dcm2_Logger::Endl;

    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "Receiver #1 register word:    0x";
    WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		   JSEB2_RCV1_OFFSET, &word4);
    cout.width(8); cout.fill('0'); 
    cout << hex << word4 << dec 
	 << Dcm2_Logger::Endl;

    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "Receiver #2 register word:    0x";
    WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		   JSEB2_RCV2_OFFSET, &word5);
    cout.width(8); cout.fill('0'); 
    cout << hex << word5 << dec 
	 << Dcm2_Logger::Endl;
    
    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "Mode register word:           0x";
    WDC_ReadAddr32(_hDev, JSEB2_COMMAND_ADDRSPACE, 
		   JSEB2_HOLD_MODE_OFFSET, &word6);
    cout.width(8); cout.fill('0'); 
    cout << hex << word6 << dec 
	 << Dcm2_Logger::Endl;
    
    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "Hold Mode: " 
      << boolalpha << ((word6 & 0x80000000) == 0x80000000) << noboolalpha;
    cout << "  Hold Setting: " << (word6 & 0x7FFF) << " bytes" 
	 << Dcm2_Logger::Endl;
    
    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "    " << "TRX on Port 1" 
      << Dcm2_Logger::Endl;
    
    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "    " << "    " << "Completed transmit: ";
    cout.width(5); cout.fill(' '); 
    cout << boolalpha << ((word2 & 0x80000000) == 0x80000000) << noboolalpha;
    cout << "  Remaining data: " << (word2 & 0xFFFFF) << " bytes" 
	 << Dcm2_Logger::Endl;

    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "    " << "    " << "Completed receive:  ";
    cout.width(5); cout.fill(' '); 
    cout << boolalpha << ((word4 & 0x80000000) == 0x80000000) << noboolalpha;
    cout << "  Remaining data: " << (word4 & 0xFFFFFFF) << " bytes" 
	 << Dcm2_Logger::Endl;
    
    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "    " << "TRX on Port 2" 
      << Dcm2_Logger::Endl;
    
    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "    " << "    " << "Completed transmit: ";
    cout.width(5); cout.fill(' '); 
    cout << boolalpha << ((word3 & 0x80000000) == 0x80000000) << noboolalpha;
    cout << "  Remaining data: " << (word3 & 0xFFFFF) << " bytes" 
	 << Dcm2_Logger::Endl;
    
    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align  << "    " << "    " << "    " 
      << "    " << "Completed receive:  ";
    cout.width(5); cout.fill(' '); 
    cout << boolalpha << ((word5 & 0x80000000) == 0x80000000) << noboolalpha;
    cout << "  Remaining data: " << (word5 & 0xFFFFFFF) << " bytes" 
	 << Dcm2_Logger::Endl;
    
    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "    " << "DMA Engine" 
      << Dcm2_Logger::Endl;

    Dcm2_Logger::Cout(LOG,"Dcm2_JSEB2::Print()") 
      << align << "    " << "    " << "    " << "    " 
      << "Active DMA: " << boolalpha << (!HasDMACompleted()) << noboolalpha;
    cout << "  TRX Setting: " << hex << ((word0 & 0x300000) >> 20) << dec;
    cout << "  Remaining data: " << GetBytesLeftToDMA() 
	 << Dcm2_Logger::Endl;
  }
  
  return true;
}
