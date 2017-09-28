#ifndef __DCM2_JSEB2_H__
#define __DCM2_JSEB2_H__

//=================================
/// \file DCM2_JSEB2.h
/// \brief The JSEB2 PCIe board object.
/// \author Michael P. McCumber
//=================================

// type definition
#ifndef UINT16
/// a convenient form for UINT64/32/16 templates 
#define UINT16 unsigned short 
#endif

// Dcm2 includes
#include <Dcm2_BaseObject.h>

// WinDriver includes
#include <windrvr.h>
#include <wdc_defs.h>

// boost includes
#include <boost/static_assert.hpp>

// standard includes
#include <iostream>
#include <limits>

using namespace std;
/// \class Dcm2_JSEB2
/// \brief the class description for the JSEB2 PCIe board. 
///
/// It can be used as a crate controller, the
/// partitioner readout, or a toy FEM
/// (if it has been reclocked). The
/// object contains an input and output
/// DMA buffers. Transceiving can be
/// performed on either TRX port. Receiving
/// is supported on either TRX or both.
/// DMAs can be managed by this object or 
/// they managed outside of the object 
/// the user.
///
class Dcm2_JSEB2 : public Dcm2_BaseObject {

public:

  Dcm2_JSEB2();
  virtual ~Dcm2_JSEB2();

  /// \todo figure out which of these methods can be moved to private

  /// gets card info and grabs the two DMA buffers for simple read/writes
  bool Init();
  /// drops card info and releases the two DMA buffers
  bool Close();

  /// SEB receiver JSEB2 settings
  bool SetReceiverDefaults();
  /// DCM2 controller JSEB2 settings
  bool SetControllerDefaults();
  /// Fake FEM JSEB2 settings
  bool SetFEMDefaults();

  /// single setup function (for better future proofing)
  bool InitTRX(int transmitter, UINT32 nbytesOut, 
	       int receiver, UINT32 nbytesIn);
  /// tell fpga how much data to expect and clear fifo
  bool InitTransmitter(int transmitter, UINT32 nbytes);
  /// tell fpga how much data to expect and clear fifo
  bool InitReceiver(int receiver, UINT32 nbytes);
  /// flush the transmitter FIFO
  bool ClearTransmitterFIFO(int transmitter);
  /// flush the receiver FIFO
  bool ClearReceiverFIFO(int receiver);
  /// only tell the FPGA how much tranmit data to expect
  bool SetTransmitLength(int transmitter, UINT32 nbytes);
  /// only tell the FPGA how much receive data to expect
  bool SetReceiveLength(int receiver, UINT32 nbytes);

  /// send bytes from a DMA buffer
  bool Transmit(int transmitter, UINT32 nbytes, WD_DMA *pDma);
  /// receive bytes into a DMA buffer
  bool Receive(int receiver, UINT32 nbytes, WD_DMA *pDma);
  /// returns false if a DMA is still ongoing
  bool HasDMACompleted();
  /// returns a count of data left to DMA
  UINT32 GetBytesLeftToDMA();
  /// kills running DMA
  bool AbortDMA();
  /// set the option for DMA use (DMA or word-by-word)
  void SetUseDMA(bool useDMA) {_useDMA = useDMA;}
  /// get the option for DMA use (DMA or word-by-word)
  bool GetUseDMA() {return _useDMA;}
  /// set the option for DMA control (wait for completion or not)
  void SetWaitOnDMA(bool waitOnDMA) {_waitOnDMA = waitOnDMA;}
  /// get the option for DMA control (wait for completion or not)
  bool GetWaitOnDMA() {return _waitOnDMA;}
  /// set the option for DMA timeouts (about on inactivity or not)
  void SetTimeOutDMA(bool timeOutDMA) {_timeOutDMA = timeOutDMA;}
  /// get the option for DMA timeouts (about on inactivity or not)
  bool GetTimeOutDMA() {return _timeOutDMA;}
  /// reallocate larger DMA buffers as needed
  bool ResetTransmitBufferSize(unsigned int nbytes);
  /// reallocate larger DMA buffers as needed
  bool ResetReceiveBufferSize(unsigned int nbytes);
  /// wrapper for syncing processor cache with the internal DMA buffer
  bool SyncCPU() {WDC_DMASyncCpu(_pDma_send); return true;}
  /// wrapper for syncing processor cache with the internal DMA buffer
  bool SyncIO() {WDC_DMASyncIo(_pDma_recv); return true;}

  /// 16bit word write variation
  bool Write(int transmitter, unsigned int nwords, UINT16 *pwords) { 
    return __Write(transmitter,nwords,pwords); 
  }
  /// 32bit word write variation
  bool Write(int transmitter, unsigned int nwords, UINT32 *pwords) { 
    return __Write(transmitter,nwords,pwords); 
  }
  /// 64bit word write variation
  bool Write(int transmitter, unsigned int nwords, UINT64 *pwords) { 
    return __Write(transmitter,nwords,pwords); 
  }

  /// 16bit word read variation
  bool Read(int receiver, unsigned int nwords, UINT16 *pwords) { 
    return __Read(receiver,nwords,pwords); 
  }
  /// 32bit word read variation
  bool Read(int receiver, unsigned int nwords, UINT32 *pwords) { 
    return __Read(receiver,nwords,pwords); 
  } 
  /// 64bit word read variation
  bool Read(int receiver, unsigned int nwords, UINT64 *pwords) { 
    return __Read(receiver,nwords,pwords); 
  } 

  /// 16bit word read variation
  bool Read2(int receiver, unsigned int nwords,int istart, UINT16 *pwords) { 
    return __Read2(receiver,nwords,istart,pwords); 
  }
  /// 32bit word read variation
  bool Read2(int receiver, unsigned int nwords, int istart,UINT32 *pwords) { 
    return __Read2(receiver,nwords,istart,pwords); 
  } 
  /// 64bit word read variation
  bool Read2(int receiver, unsigned int nwords, int istart,UINT64 *pwords) { 
    return __Read2(receiver,nwords,istart,pwords); 
  } 

  /// return the DAQ status word
  UINT32 GetStatus(); 
  /// get the status word from optical transmitter
  UINT32 GetTransmitterStatusWord(int transmitter); 
  /// get the status word from optical receiver
  UINT32 GetReceiverStatusWord(int receiver);       
  /// get the status word from DMA command register
  UINT32 GetDMAStatusWord();
  /// get the status word from the mode register
  UINT32 GetModeStatusWord();

  /// returns Jungo location struct 
  WD_PCI_SLOT GetDeviceSlot() {return _deviceSlot;}
  /// returns bus number
  DWORD GetPciBus() {return _deviceSlot.dwBus;}
  /// returns slot number
  DWORD GetPciSlot() {return _deviceSlot.dwSlot;}
  /// sets the Jungo location struct
  bool SetPciSlot(WD_PCI_SLOT islot);
 
  /// returns a pointer to the jungo jseb2 
  /// device handle for direct jungo-like interaction
  WDC_DEVICE_HANDLE GetHandle() {return _hDev;}

  /// JSEB2 wrapper for jungo's WriteAddr64 function
  void WriteAddr64(DWORD dwAddrSpace, DWORD dwOffset, UINT64 u64data);
  /// JSEB2 wrapper for jungo's ReadAddr64 function
  UINT64 ReadAddr64(DWORD dwAddrSpace, DWORD dwOffset);

  /// wrapper for processor syncing against a provided DMA buffer
  void DMASyncIo(WD_DMA *pDma, unsigned int nbytes);
  /// wrapper for processor syncing against a provided DMA buffer
  void DMASyncCpu(WD_DMA *pDma, unsigned int nbytes);

  /// set the interprocess mutex lock (JSEB2.<bus>.<slot>) usage setting 
  void SetUseLock(bool useLock) {_useLock = useLock;}
  /// get the interprocess mutex lock (JSEB2.<bus>.<slot>) usage setting
  bool GetUseLock() {return _useLock;}
  /// locks the JSEB2 for access only by this object
  bool GetLock();
  /// normal GetLock() with time-out functionality
  bool WaitForLock(long sec);
  /// completely removes lock from the system (clean up only)
  bool DestroyLock();
  /// askes JSEB2 pointer if this object has the device locked
  bool HasLock() {return _hasLock;}
  /// momentarily releases lock to allow another object access
  bool PauseLock();
  /// releases the lock
  bool SetFree();

  /// automatically send hold if FIFO contains maxBytes
  bool EnableHolds(UINT16 maxBytes);
  /// manually exert hold
  bool RaiseHold();
  /// manually release the hold
  bool ReleaseHold();
  /// return the JSEB2 from hold mode to normal TRX operation
  bool DisableHolds();

  /// failure recovery (while locked, abort dma, flush FIFOs)
  bool CleanUp(int trx);

  /// long form status
  bool Print();

  /// Chi's FPGA firmware Vendor ID
  static const UINT32 JSEB2_VENDOR_ID = 0x1172;
  /// Chi's FPGA firmware Device ID
  static const UINT32 JSEB2_DEVICE_ID = 0x4;

  /// code for tranmitter on port #1
  static const int TR_ONE = 1;
  /// code for tranmitter on port #2
  static const int TR_TWO = 2;
  /// code for combined transmits over port #1 and #2
  static const int TR_BOTH = 3;

  /// code for receiver on port #1
  static const int RCV_ONE = 1;
  /// code for receiver on port #2
  static const int RCV_TWO = 2;
  /// code for combined receives over port #1 and #2
  static const int RCV_BOTH = 3;

private:

  /// template for transmitting word arrays
  template<typename T> bool __Write(int transmitter, 
				    unsigned int nwords, 
				    T *pwords);
  /// template for receiving word arrays
  template<typename T> bool __Read(int receiver, 
				   unsigned int nwords, 
				   T *pwords);

  /// template for receiving word arrays
  template<typename T> bool __Read2(int receiver, 
				   unsigned int nwords, 
				    int istart,
				   T *pwords);

  //---development---
  /// get the number of words on each receiver FIFO
  UINT32 GetNumberOfBytesOnFIFO(int receiver);
  /// extract the FIFO content
  bool GetContentOfFIFO(int receiver, unsigned int nwords, UINT32 *pwords);
  //------only-------

  /// JSEB2 device location on PCIe bus
  WD_PCI_SLOT _deviceSlot;
  /// JSEB2 device handle 
  WDC_DEVICE_HANDLE _hDev;

  /// pre-allocated outgoing DMA memory buffer
  WD_DMA *_pDma_send;
  /// pointer to outgoing buffer
  PVOID _ppbuff_send;
  /// memory buffer size, bytes
  DWORD _dwTransmitDMABufSize;
  /// outgoing dma options
  DWORD _dwTransmitDMAOptions;
  /// pre-allocated incoming DMA memory buffer
  WD_DMA *_pDma_recv;
  /// pointer to incoming buffer
  PVOID _ppbuff_recv;
  /// memory buffer size, bytes
  DWORD _dwReceiveDMABufSize;  
  /// outgoing dma options
  DWORD _dwReceiveDMAOptions;

  /// true = DMAs are watched for completion and CPU syncing handled, 
  /// false = DMAs are initiated only
  bool _useDMA;
  /// true = DMAs are watched for completion and CPU syncing handled, 
  /// false = DMAs are initiated only
  bool _waitOnDMA;
  /// true = DMAs are aborted automatically on inactivity, 
  /// false = nothing done about inactivity
  bool _timeOutDMA;

  /// true = lock device when in use
  bool _useLock;
  /// true = the device is locked by this object (and not another)
  bool _hasLock;

  /// receiving FIFO limit before holds are transmited
  UINT32 _holdByteLimit;

  /// tranceiver #1 in/out register address 
  static const DWORD JSEB2_TRX1_ADDRSPACE    = 0;
  /// command register address
  static const DWORD JSEB2_COMMAND_ADDRSPACE = 2;
  /// tranceiver #2 in/out register address
  static const DWORD JSEB2_TRX2_ADDRSPACE    = 4;

  /// flush FIFO command
  static const UINT32 JSEB2_TRX_INIT_COMMAND = 0x20000000;
  /// set length command
  static const UINT32 JSEB2_TRX_SIZE_COMMAND = 0x40000000;
  /// enable holds command
  static const UINT32 JSEB2_TRX_HOLD_COMMAND = 0x8000000;

  /// command offset for transmitter #1
  static const UINT32 JSEB2_TR1_OFFSET  = 0x18;
  /// command offset for transmitter #2
  static const UINT32 JSEB2_TR2_OFFSET  = 0x20;
  /// command offset for receiver #1
  static const UINT32 JSEB2_RCV1_OFFSET = 0x1c;
  /// command offset for receiver #2
  static const UINT32 JSEB2_RCV2_OFFSET = 0x24;
  
  /// offset for DMA address's lower 32 bits
  static const UINT32 JSEB2_DMA_ADDR_LOW_OFFSET = 0x0;
  /// offset for DMA address's upper 32 bits
  static const UINT32 JSEB2_DMA_ADDR_HIGH_OFFSET= 0x4;

  /// offset for the DMA length
  static const UINT32 JSEB2_DMA_SIZE_OFFSET = 0x8;

  /// offset for start DMA commands
  static const UINT32 JSEB2_DMA_COMMAND_OFFSET  = 0xc;
  /// start DMA on transceiver #1
  static const UINT32 JSEB2_DMA_ON_TRX1_COMMAND = 0x00100000;
  /// start DMA on transceiver #2
  static const UINT32 JSEB2_DMA_ON_TRX2_COMMAND = 0x00200000;
  /// start DMA on transceiver #1 and #2
  static const UINT32 JSEB2_DMA_ON_TRX3_COMMAND = 0x00300000;

  /// offset for abort DMA command
  static const UINT32 JSEB2_DMA_ABORT_OFFSET  = 0x10;
  /// command to abort the DMA  
  static const UINT32 JSEB2_DMA_ABORT_COMMAND = 0x2;
  /// clear the abort DMA command
  static const UINT32 JSEB2_DMA_ABORT_CLEAR   = 0x0;

  /// offset for hold commands
  static const UINT32 JSEB2_HOLD_MODE_OFFSET  = 0x28;
  /// raise hold command
  static const UINT32 JSEB2_HOLD_MODE_COMMAND = 0x40000000;
  /// disable holds
  static const UINT32 JSEB2_HOLD_MODE_CLEAR   = 0x0;

  /// offset for the hold threshold
  static const UINT32 JSEB2_WORD_COUNT_OFFSET = 0x40;

  /// DMA option, unused
  static const UINT32 TRCVR_BOTH = 0x300000;
  /// DMA option, unused
  static const UINT32 TRCVR_TWO  = 0x200000;
  /// DMA option, unused
  static const UINT32 TRCVR_ONE  = 0x100000;

  /// PCIe packet size 128 bytes
  static const UINT32 PAYLOAD_128B = 0x00000;
  /// PCIe packet size 256 bytes
  static const UINT32 PAYLOAD_256B = 0x10000;
  /// PCIe packet size 512 bytes
  static const UINT32 PAYLOAD_512B = 0x20000;
  /// PCIe packet size 1024 bytes
  static const UINT32 PAYLOAD_1KB  = 0x30000;
  /// PCIe packet size 2048 bytes
  static const UINT32 PAYLOAD_2KB  = 0x40000;

  /// TLP Digest bit in PCIe packet header
  static const UINT32 TD_OFF = 0x0000;
  /// TLP Digest bit in PCIe packet header
  static const UINT32 TD_ON  = 0x8000;
  
  /// Error Protection(?) bit in the PCIe packet header
  static const UINT32 EP_OFF = 0x0000;
  /// Error Protection(?) bit in the PCIe packet header
  static const UINT32 EP_ON  = 0x4000; 

  /// Relaxed ordering bit in the PCIe packet header
  static const UINT32 RELAXED_ORDERING_OFF = 0x0000;
  /// Relaxed ordering bit in the PCIe packet header
  static const UINT32 RELAXED_ORDERING_ON  = 0x2000;

  /// No snoop bit in the PCIe packet header
  static const UINT32 NOSNOOP_OFF = 0x0000;
  /// No snoop bit in the PCIe packet header
  static const UINT32 NOSNOOP_ON  = 0x1000;

  /// PCIe packet traffic class 0
  static const UINT32 TRAFFIC_CLASS_0 = 0x000;
  /// PCIe packet traffic class 1
  static const UINT32 TRAFFIC_CLASS_1 = 0x100;
  /// PCIe packet traffic class 2
  static const UINT32 TRAFFIC_CLASS_2 = 0x200;
  /// PCIe packet traffic class 3
  static const UINT32 TRAFFIC_CLASS_3 = 0x300;
  /// PCIe packet traffic class 4
  static const UINT32 TRAFFIC_CLASS_4 = 0x400;
  /// PCIe packet traffic class 5
  static const UINT32 TRAFFIC_CLASS_5 = 0x500;
  /// PCIe packet traffic class 6
  static const UINT32 TRAFFIC_CLASS_6 = 0x600;
  /// PCIe packet traffic class 7
  static const UINT32 TRAFFIC_CLASS_7 = 0x700;

  /// PCIe packet specification for transmits from a 32 bit address
  static const UINT32 FORMAT_3DW_MEM_TO_OPT = 0x00;
  /// PCIe packet specification for transmits from a 64 bit address
  static const UINT32 FORMAT_4DW_MEM_TO_OPT = 0x20;
  /// PCIe packet specification for receive into a 32 bit address
  static const UINT32 FORMAT_3DW_OPT_TO_MEM = 0x40;
  /// PCIe packet specification for receive into a 64 bit address
  static const UINT32 FORMAT_4DW_OPT_TO_MEM = 0x60;

  /// storage for the PCIe format options
  UINT32 pcie_format;

  /// unused(?)
  static const UINT32 TYPE = 0x0; 

  /// fixed short term dword storage
  DWORD dwStatus; 
  /// fixed short term boolean storage
  bool status;
  /// fixed short term 32 bit storage
  UINT32 u32data;
  /// fixed short term 64 bit storage
  UINT64 u64data;

  /// \class JSEB2_INT_RESULT
  /// \brief JSEB2 interupt jungo description struct (from Chi)
  typedef struct {
    /// Number of interrupts received 
    DWORD dwCounter;
    /// Number of interrupts not yet handled 
    DWORD dwLost; 
    /// See WD_INTERRUPT_WAIT_RESULT values in windrvr.h 
    WD_INTERRUPT_WAIT_RESULT waitResult;
    /// Interrupt type that was actually enabled 
    /// (MSI/MSI-X/Level Sensitive/Edge-Triggered)
    DWORD dwEnabledIntType;
    /// Message data of the last received MSI/MSI-X
    /// (Windows Vista); N/A to line-based interrupts)
    DWORD dwLastMessage;
  } JSEB2_INT_RESULT;

  /// unused(?)
  typedef void (*JSEB2_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
				    JSEB2_INT_RESULT *pIntResult);
  /// unused(?)
  typedef void (*JSEB2_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev,
				      DWORD dwAction);

  /// \class JSEB2_DEV_CTX
  /// \brief JSEB2 jungo description struct (from Chi)
  typedef struct {
    /// interupt handler
    JSEB2_INT_HANDLER   funcDiagIntHandler;
    /// event handler
    JSEB2_EVENT_HANDLER funcDiagEventHandler; 
  } JSEB2_DEV_CTX, *PJSEB2_DEV_CTX;

};

/// Template traits class for different flavors of Read/Write
template<typename T> 
struct width_traits {
  BOOST_STATIC_ASSERT(sizeof(T) == 0);
  typedef T mask_type;
  typedef unsigned int subword_type;
  const static mask_type mask;
  const static subword_type wordsPerQword;
  static void shift_data(T& data) { return; }
};

/// Template specialization for UINT16
template<> 
struct width_traits<UINT16> {
  typedef UINT16 mask_type;
  typedef unsigned int subword_type;
  const static mask_type mask;
  const static subword_type wordsPerQword;
  static void shift_data(UINT64& data) { data >>= 8*sizeof(UINT16); }    
};

/// Template specialization for UINT32
template<> 
struct width_traits<UINT32> {
  typedef UINT32 mask_type;
  typedef unsigned int subword_type;
  const static mask_type mask;
  const static subword_type wordsPerQword;
  static void shift_data(UINT64& data) { data >>= 8*sizeof(UINT32); }    
};

/// Template specialization for UINT64
template<> 
struct width_traits<UINT64> {
  typedef UINT64 mask_type;
  typedef unsigned int subword_type;
  const static mask_type mask;
  const static subword_type wordsPerQword;
  // clear the bits in this case
  static void shift_data(UINT64& data) { data = 0; }
};

/// This template is used by the Write() functions to transmit arrays
/// of words regardless of the number of bits per word. This can be done
/// for DMA or direct word-by-word writes. TR_BOTH has limited support
/// in the FPGA and so has not been fully implemented or tested here.
///
/// \param[in] transmitter TR port code [TR_ONE,TR_TWO,TR_BOTH]
/// \param[in] nwords Number of words
/// \param[in] pwords A pointer to the beginning of the array
///
/// \return true if successful
///
template<typename T> 
bool Dcm2_JSEB2::__Write(int transmitter, unsigned int nwords, T *pwords) {

  // lock JSEB2 if needed
  bool wasLocked = false;
  if (_useLock) {
    if (_hasLock) {
      wasLocked = true;
    } else {
      //cout << "JSEB2__Write get lock " << endl;
      wasLocked = false;
      GetLock();
    }
  }
  //cout << "JSEB2__Write got lock " << endl;

  // send data via DMA
  if (_useDMA) {

    if (( transmitter != TR_ONE ) && 
	( transmitter != TR_TWO ) && 
	( transmitter != TR_BOTH )) {
      std::cerr << "Dcm2_JSEB2::Write() - "
		<< "Invalid DMA transmitter selection." 
		<< std::endl;
      return false;
    }

    // calculate DMA size
    UINT32 nbytes;
    nbytes = (UINT32)(nwords*sizeof(T));

    // allocate a larger buffer if needed
    if (nbytes > _dwTransmitDMABufSize) {
      //cout << "ResetTransmitBufferSize to " << nbytes*2 << endl;
      if (!ResetTransmitBufferSize(nbytes*2)) {
	/// \todo switch these prints to the Dcm2_Logger if possible
	std::cerr << "Dcm2_JSEB2::Write() - " 
		  << "Unable to allocate sufficient resources to "
		  << "complete the requested DMA" 
		  << std::endl;
	return false;
      }
    }

    // get pointer to DMA buffer
    T *pbuff_words = static_cast<T*>(_ppbuff_send);

    // copy data into DMA buffer
    for (unsigned int iword = 0; iword < nwords; ++iword) {
      *(pbuff_words+iword) = *pwords++;
    }

    // ready transmitter ***
    InitTransmitter(transmitter, nbytes);

    // begin transmit DMA
    status = Transmit(transmitter, nbytes, _pDma_send);
    if (!status) {
      std::cerr << "Dcm2_JSEB2::Write() - " 
		<< "Transmit returned false." 
		<< std::endl;
      return false;
    }

  } else { // or send via direct register writes

    // calculate transfer size
    UINT32 nbytes;
    nbytes = (UINT32)(nwords*sizeof(T));

    // ready transmitter
    InitTransmitter(transmitter, nbytes);

    DWORD dwAddrSpace;

    if (transmitter == TR_ONE) {
      dwAddrSpace = JSEB2_TRX1_ADDRSPACE;
    } else if (transmitter == TR_TWO) {
      dwAddrSpace = JSEB2_TRX2_ADDRSPACE;
    } else if (transmitter == TR_BOTH) {
      std::cerr << "Dcm2_JSEB2::Write() - " 
		<< "TR_BOTH unsupported for direct writes." 
		<< std::endl;
      return false;
    } else {
      std::cerr << "Dcm2_JSEB2::Write() - "
		<< "Invalid direct-write transmitter selection." 
		<< std::endl;
      return false;
    }

    // calculate number of 64bit writes
    unsigned int qwords;
    unsigned int remainder;
    remainder = nwords%width_traits<T>::wordsPerQword;
    if (remainder == 0) {
      qwords = nwords/width_traits<T>::wordsPerQword;
    } else {
      qwords = nwords/width_traits<T>::wordsPerQword + 1;
    }

    // repackage data into 64bit wide array and send
    UINT64 temp64data;
    for (unsigned int iword = 0; iword < qwords-1; ++iword) {
      u64data = 0x0;
      temp64data = 0x0;

      for (unsigned int jword = 0; 
	   jword < width_traits<T>::wordsPerQword; 
	   ++jword) {
	temp64data = (UINT64)(*pwords++ & width_traits<T>::mask);
	u64data += (temp64data << (jword*8*sizeof(T)));
      }
	  
      WriteAddr64(dwAddrSpace, 0x0, u64data);
    }

    // send last 64bit word
    if (remainder == 0) remainder = width_traits<T>::wordsPerQword;

    u64data = 0x0;
    temp64data = 0x0;
    for (unsigned int jword = 0; jword < remainder; ++jword) {
      temp64data = (UINT64)(*pwords++ & width_traits<T>::mask);
      u64data += (temp64data << (jword*8*sizeof(T)));
    }

    WriteAddr64(dwAddrSpace, 0x0, u64data);
  }

  // unlock if needed
  if (_useLock) {
    if (!wasLocked) {
      SetFree();
    }
  }

  return true;
}

template<typename T> 
bool Dcm2_JSEB2::__Read2(int receiver, unsigned int nwords,int istart, T *pwords) {

  // lock JSEB2 if needed
  bool wasLocked = false;
  if (_useLock) {
    if (_hasLock) {
      wasLocked = true;
    } else {
      wasLocked = false;
      GetLock();
    }
  }
  //cout << "JSEB2:__Read2 direct " << endl;
      DWORD dwAddrSpace;

      if ( receiver == RCV_ONE ) {
	dwAddrSpace = JSEB2_TRX1_ADDRSPACE;
      } else if ( receiver == RCV_TWO ) {
	dwAddrSpace = JSEB2_TRX2_ADDRSPACE;
      } else if ( receiver == RCV_BOTH ) {
	std::cerr << "Dcm2_JSEB2::Read() - " 
		  << "RCV_BOTH unsupported for direct writes." 
		  << std::endl;
	return false;
      } else {
	std::cerr << "Dcm2_JSEB2::Read() - " 
		  << "Invalid direct-read receiver selection." 
		  << std::endl;
	return false;
      }
 UINT32 u32Data;
 DWORD dwOffset;
#define  tx_mode_reg 0x28
#define  cs_init  0x20000000
#define  cs_bar 2
#define  cs_start 0x40000000
#define  r2_cs_reg 0x24
// initalize transmitter mode register...
     dwAddrSpace =2;
     u32Data = 0xf0000008;
     dwOffset = tx_mode_reg;
     WDC_WriteAddr32(_hDev, dwAddrSpace, dwOffset, u32Data);
    /*initialize the receiver */
     dwAddrSpace =cs_bar;
     u32Data = cs_init;
     dwOffset = r2_cs_reg; // 0x24 port 2 receive offset
     WDC_WriteAddr32(_hDev, dwAddrSpace, dwOffset, u32Data);
     /* write byte count **/
     dwAddrSpace =cs_bar;
     u32Data = cs_start+nwords*4;
     dwOffset = r2_cs_reg; // port 2 receive offset
     WDC_WriteAddr32(_hDev, dwAddrSpace, dwOffset, u32Data);


  // unlock if needed
  if (_useLock) {
    if (!wasLocked) {
      SetFree();
    }
  }

  return true;
} // __Read2

/// This template is used by the Read() functions to transmit arrays
/// of words regardless of the number of bits per word. This can be done
/// for DMA or direct word-by-word writes. RCV_BOTH is fully supported
/// in the FPGA and so has been fully implemented and tested.
///
/// \param[in] receiver RCV port code [RCV_ONE,RCV_TWO,RCV_BOTH]
/// \param[in] nwords Number of words
/// \param[out] pwords A pointer to the beginning of the array
///
/// \return true if successful
///
template<typename T> 
bool Dcm2_JSEB2::__Read(int receiver, unsigned int nwords, T *pwords) {

  // lock JSEB2 if needed
  bool wasLocked = false;
  if (_useLock) {
    if (_hasLock) {
      wasLocked = true;
    } else {
      wasLocked = false;
      GetLock();
    }
  }

  // send data via DMA
  if (_useDMA) {
    if (( receiver != RCV_ONE ) && 
	( receiver != RCV_TWO ) && 
	( receiver != RCV_BOTH )) {
      std::cerr << "Dcm2_JSEB2::Read() - "
		<< "Invalid DMA receiver selection." 
		<< std::endl;
      return false;
    }

    // calculate DMA size
    UINT32 nbytes;
    nbytes = (UINT32)(nwords*sizeof(T));

    // allocate a larger buffer if needed
    if (nbytes > _dwReceiveDMABufSize) {
      if (!ResetReceiveBufferSize(nbytes*2)) {
	std::cerr << "Dcm2_JSEB2::Write() - "
		  << "Unable to allocate sufficient resources "
		  << "to complete the requested DMA" 
		  << std::endl;
	return false;
      }
    }

    // get pointer to DMA buffer
    T *pbuff_words = static_cast<T*>(_ppbuff_recv);

    // begin receive DMA
    status = Receive(receiver, nbytes, _pDma_recv);
    if (!status) {
      std::cerr << "Dcm2_JSEB2::Read() - Receive returned false." << std::endl;
      return false;
    }

    // copy data out of DMA buffer
    for (unsigned int iword = 0; iword < nwords; ++iword) {
      *pwords++ = *(pbuff_words+iword);
    }

  } else { // or get via direct register reads
    //cout << "JSEB2:__Read direct " << endl;
      DWORD dwAddrSpace;

      if ( receiver == RCV_ONE ) {
	dwAddrSpace = JSEB2_TRX1_ADDRSPACE;
      } else if ( receiver == RCV_TWO ) {
	dwAddrSpace = JSEB2_TRX2_ADDRSPACE;
      } else if ( receiver == RCV_BOTH ) { 
	std::cerr << "Dcm2_JSEB2::Read() - " 
		  << "RCV_BOTH unsupported for direct writes." 
		  << std::endl;
	return false;
      } else {
	std::cerr << "Dcm2_JSEB2::Read() - " 
		  << "Invalid direct-read receiver selection." 
		  << std::endl;
	return false;
      }

      // calculate number of 64bit reads
      unsigned int qwords;
      unsigned int remainder;
      remainder = nwords%width_traits<T>::wordsPerQword;
      if (remainder == 0) {
	qwords = nwords/width_traits<T>::wordsPerQword;
      } else {
	qwords = nwords/width_traits<T>::wordsPerQword + 1;
      }
      //cout << "Dcm2_JSEB: DIRECT READING nwords = "<< nwords << " QWORDS = "  << qwords  << endl;
      UINT32 tmp_buff[8000];
      UINT32 *buff_rec = tmp_buff;
      UINT32 buf_lo, buf_hi;
      // read and repackage data into desired width
      for (unsigned int iword = 0; iword < qwords-1; ++iword) {
	u64data = ReadAddr64(dwAddrSpace, 0x0);
       if (iword < 5) {
	 buf_lo = (u64data &0xffffffff);
	 buf_hi = u64data >>32;
	 //cout << " JSEB2 read word " << hex << u64data << dec << endl;
	 //cout << " JSEB2 tmp_buff low " << hex << buf_lo << " HI " << buf_hi << dec << endl;
       }

	for (unsigned int jword = 0; 
	     jword < width_traits<T>::wordsPerQword; 
	     ++jword) {
	  *pwords++ = (u64data) & width_traits<T>::mask;
	  width_traits<T>::shift_data(u64data);
	}
      }

      // read last word
      u64data = ReadAddr64(dwAddrSpace, 0x0);

      if (remainder == 0) remainder = width_traits<T>::wordsPerQword;

      for (unsigned int jword = 0; jword < remainder; ++jword) {
	*pwords++ = (u64data) & width_traits<T>::mask;
	width_traits<T>::shift_data(u64data);
      }
  }

  // unlock if needed
  if (_useLock) {
    if (!wasLocked) {
      SetFree();
    }
  }

  return true;
}

#endif // __DCM2_JSEB2_H__
