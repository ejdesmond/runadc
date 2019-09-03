/**
 * adcj.h
 * Control class for sphenix digitizer ADC
 * follows the functions in Sarah Campbel's sphenix_adc_test_exttrig_xmit_dcm_busy.cc
 * This code is executed in initialize_xmit and start_xmit
 * Define functions to encapsulate the control of the ADC
 * This adc class configures multiple xmit groups in a single digitizer crate
 * created: 01/23/2019
 */


#ifndef __ADC_H__
#define __ADC_H__
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "wdc_defs.h"
#include "wdc_lib.h"
#include "utils.h"
#include "status_strings.h"
#include "samples/shared/diag_lib.h"
#include "samples/shared/wdc_diag_lib.h"
#include "pci_regs.h"
#include "jseb2_lib.h"
#include <string>
#include "Dcm2_JSEB2.h"

#include <Dcm2_Controller.h>
#include <dcm2_runcontrol.h>
#include <Dcm2_Partition.h>
#include <Dcm2_Partitioner.h>
#include <Dcm2_Board.h>
#include <Dcm2_BaseObject.h>
#include <Dcm2_Reader.h>
#include <Dcm2_WinDriver.h>

#include "jseb2Controller.h"
#include <boost/interprocess/permissions.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/thread/mutex.hpp>
using namespace boost::interprocess;

#define DCMIUFNSIZE 256
#define DCMIUMAXNBPARTNERS 5


#define poweroff      0x0
#define poweron       0x1
#define configure_s30 0x2
#define configure_s60 0x3
#define configure_cont 0x20
#define rdstatus      0x80
#define loopback        0x04

#define dcm2_run_off  254
#define dcm2_run_on   255

#define dcm2_online   2
#define dcm2_setmask  3
#define dcm2_offline_busy 4
#define dcm2_load_packet_a 10
#define dcm2_load_packet_b 11
#define dcm2_offline_load 9
#define dcm2_status_read 20
#define dcm2_led_sel     29
#define dcm2_buffer_status_read 30
#define dcm2_status_read_inbuf 21
#define dcm2_status_read_evbuf 22
#define dcm2_status_read_noevnt 23
#define dcm2_zero 12
#define dcm2_compressor_hold 31


#define dcm2_5_readdata 4
#define dcm2_5_firstdcm 8
#define dcm2_5_lastdcm  9
#define dcm2_5_status_read 5
#define dcm2_5_source_id 25
#define dcm2_5_lastchnl 24

#define dcm2_packet_id_a 25
#define dcm2_packet_id_b 26
#define dcm2_hitformat_a 27
#define dcm2_hitformat_b 28

#define part_run_off  254
#define part_run_on   255
#define part_online   2
#define part_offline_busy 3
#define part_offline_hold 4
#define part_status_read 20
#define part_source_id 25


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
#define  dma_4dw_trans 0x0
#define  dma_3dw_rec   0x40
#define  dma_4dw_rec   0x60
#define  dma_in_progress 0x80000000


#define  dma_abort 0x2

#define  adc_cntrl_int 0x0
#define  adc_cntrl_loopback_on 0x1
#define  adc_cntrl_loopback_off 0x2
#define  adc_cntrl_offline 0x3
#define  adc_cntrl_online 0x4


#define  mb_cntrl_add     0x1
#define  mb_cntrl_test_on 0x1
#define  mb_cntrl_test_off 0x0
#define  mb_cntrl_set_run_on 0x2
#define  mb_cntrl_set_run_off 0x3
#define  mb_cntrl_set_trig1 0x4
#define  mb_cntrl_set_trig2 0x5
#define  mb_cntrl_load_frame 0x6
#define  mb_cntrl_load_trig_pos 0x7


#define  mb_feb_power_add 0x1
#define  mb_feb_conf_add 0x2
#define  mb_feb_pass_add 0x3

#define  mb_feb_lst_on          1
#define  mb_feb_lst_off         0
#define  mb_feb_rxreset         2
#define  mb_feb_align           3



#define  mb_feb_adc_align       1
#define  mb_feb_a_nocomp        2
#define  mb_feb_b_nocomp        3
#define  mb_feb_blocksize       4
#define  mb_feb_timesize        5
#define  mb_feb_mod_number      6
#define  mb_feb_a_id            7
#define  mb_feb_b_id            8
#define  mb_feb_max             9

#define  mb_feb_test_source    10
#define  mb_feb_test_sample    11
#define  mb_feb_test_frame     12
#define  mb_feb_test_channel   13
#define  mb_feb_test_ph        14
#define  mb_feb_test_base      15
#define  mb_feb_test_ram_data  16

#define  mb_feb_a_test         17
#define  mb_feb_b_test         18

#define  mb_feb_a_rdhed        21
#define  mb_feb_a_rdbuf        22
#define  mb_feb_b_rdhed        23
#define  mb_feb_b_rdbuf        24

#define  mb_feb_read_probe     30
#define  mb_feb_adc_reset      33

#define  mb_a_buf_status       34
#define  mb_b_buf_status       35
#define  mb_a_ham_status       36
#define  mb_b_ham_status       37

#define  mb_feb_a_maxwords     40
#define  mb_feb_b_maxwords     41

#define  mb_feb_hold_enable    42


#define  mb_pmt_adc_reset       1
#define  mb_pmt_spi_add         2
#define  mb_pmt_adc_data_load   3

#define  mb_xmit_conf_add 0x2
#define  mb_xmit_pass_add 0x3

#define  mb_xmit_modcount 0x1
#define  mb_xmit_enable_1 0x2
#define  mb_xmit_enable_2 0x3
#define  mb_xmit_test1 0x4
#define  mb_xmit_test2 0x5

#define   mb_xmit_testdata  10

#define  mb_xmit_rdstatus 20
#define  mb_xmit_rdcounters 21
#define  mb_xmit_link_reset    22
#define  mb_opt_dig_reset   23
#define  mb_xmit_dpa_fifo_reset    24
#define  mb_xmit_dpa_word_align    25

#define  mb_trig_run                1
#define  mb_trig_frame_size         2
#define  mb_trig_deadtime_size      3
#define  mb_trig_active_size        4
#define  mb_trig_delay1_size        5
#define  mb_trig_delay2_size        6
#define  mb_trig_enable             7

#define  mb_trig_calib_delay        8

#define  mb_trig_prescale0         10
#define  mb_trig_prescale1         11
#define  mb_trig_prescale2         12
#define  mb_trig_prescale3         13
#define  mb_trig_prescale4         14
#define  mb_trig_prescale5         15
#define  mb_trig_prescale6         16
#define  mb_trig_prescale7         17
#define  mb_trig_prescale8         18

#define  mb_trig_mask0             20
#define  mb_trig_mask1             21
#define  mb_trig_mask2             22
#define  mb_trig_mask3             23
#define  mb_trig_mask4             24
#define  mb_trig_mask5             25
#define  mb_trig_mask6             26
#define  mb_trig_mask7             27
#define  mb_trig_mask8             28

#define  mb_trig_rd_param          30
#define  mb_trig_pctrig            31
#define  mb_trig_rd_status         32
#define  mb_trig_reset             33
#define  mb_trig_calib             34
#define  mb_trig_rd_gps            35

#define  mb_trig_g1_allow_min      36
#define  mb_trig_g1_allow_max      37
#define  mb_trig_g2_allow_min      38
#define  mb_trig_g2_allow_max      39

#define  mb_trig_sel1              40
#define  mb_trig_sel2              41
#define  mb_trig_sel3              42
#define  mb_trig_sel4              43

#define  mb_trig_g1_width          45
#define  mb_trig_g2_width          46

#define  mb_trig_p1_delay          50
#define  mb_trig_p1_width          51
#define  mb_trig_p2_delay          52
#define  mb_trig_p2_width          53
#define  mb_trig_p3_delay          54
#define  mb_trig_p3_width          55
#define  mb_trig_pulse_delay       58
#define  mb_trig_output_select     59

#define  mb_trig_pulse1            60
#define  mb_trig_pulse2            61
#define  mb_trig_pulse3            62

#define  mb_trig_frame_trig        63
#define  mb_trig_rd_counter        70

#define  mb_gate_fake_sel          80
#define  mb_fake_gate_width        47
#define  mb_scaler_out_sel_0       81
#define  mb_scaler_out_sel_1       82

#define  mb_shaper_pulsetime        1
#define  mb_shaper_dac              2
#define  mb_shaper_pattern          3
#define  mb_shaper_write            4
#define  mb_shaper_pulse            5
#define  mb_shaper_entrig           6

#define  mb_feb_pmt_gate_size      47
#define  mb_feb_pmt_beam_delay     48
#define  mb_feb_pmt_beam_size      49
#define  mb_feb_pmt_trig_delay     87

#define  mb_feb_pmt_gate1_size     88
#define  mb_feb_pmt_beam1_delay    89
#define  mb_feb_pmt_beam1_size     90
#define  mb_feb_pmt_trig1_delay    91

#define  mb_feb_pmt_ch_set         50
#define  mb_feb_pmt_delay0         51
#define  mb_feb_pmt_delay1         52
#define  mb_feb_pmt_precount       53
#define  mb_feb_pmt_thresh0        54
#define  mb_feb_pmt_thresh1        55
#define  mb_feb_pmt_thresh2        56
#define  mb_feb_pmt_thresh3        57
#define  mb_feb_pmt_width          58
#define  mb_feb_pmt_deadtime       59
#define  mb_feb_pmt_window         60
#define  mb_feb_pmt_words          61
#define  mb_feb_pmt_cos_mul        62
#define  mb_feb_pmt_cos_thres      63
#define  mb_feb_pmt_mich_mul       64
#define  mb_feb_pmt_mich_thres     65
#define  mb_feb_pmt_beam_mul       66
#define  mb_feb_pmt_beam_thres     67
#define  mb_feb_pmt_en_top         68
#define  mb_feb_pmt_en_upper       69
#define  mb_feb_pmt_en_lower       70
#define  mb_feb_pmt_blocksize      71

#define  mb_feb_pmt_test           80
#define  mb_feb_pmt_clear          81
#define  mb_feb_pmt_test_data      82
#define  mb_feb_pmt_pulse          83

#define  mb_feb_pmt_rxreset        84
#define  mb_feb_pmt_align_pulse    85
#define  mb_feb_pmt_rd_counters    86

#define  mb_version               254

#define  sp_cntrl_sub               2
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

#define  sp_adc_sel_link_rxoff     8

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

#define  sp_adc_sel_caltrig        33

#define  sp_adc_calib_dac         100
#define  sp_adc_calib_ch          101
#define  sp_adc_calib_write       102
#define  sp_adc_calib_send        103
#define  sp_adc_calib_gate        104

#define  sp_xmit_lastmod            1
#define  sp_xmit_rxanalogreset      2
#define  sp_xmit_rxdigitalreset     3
#define  sp_xmit_init               4

#define  sp_xmit_sub                1
#define  sp_xmit_rxbytord          15

#define  dma_buffer_size        10000000
#define  sp_cntrl_busyrst           4

using namespace std;

class ADC {

 public:

  ADC();
  ~ADC();

  // initialize controller
   void setBusy();
 
  bool  initialize(UINT32 eventstotake);
  void setNumAdcBoards(int xmitgroup, int numboards);
  void setFirstAdcSlot(int xmitgroup, int firstdigslotnumber);
  void setSampleSize(int xmitgroup, int samplesize);
  void setDcm2State();
  void setL1Delay(int l1delay);
  void setJseb2Port(int portnumber);
  void adcResets(int nmodindex);
  void setJseb2Name(string jseb2name);
  bool startWinDriver();
  void stopWinDriver();
  void InitJseb2Dev();
  void freeDMAbuffer();
  void setObDatafileName(string datafilename);
  void setNumXmitGroups(int nxmitgroups);
 
  int adc_setup(WDC_DEVICE_HANDLE hDev, int imod, int iprint);
 
  //int pcie_send_1(WDC_DEVICE_HANDLE hDev, int mode, int nword, UINT32 *buff_send);

 
  //int pcie_rec_2(WDC_DEVICE_HANDLE hDev, int mode, int istart, int nword, int ipr_status, UINT32 *buff_rec);


 // start data taking


 
 
private:

 int partitionerNumb;
 char DCMIUFileName[DCMIUMAXNBPARTNERS][DCMIUFNSIZE];
 char dcmInterName[64];  // object name
  char partitionName[64]; // partition name
  //  std::vector<EventSample> AllData;
  string obfilename; // filename for oBuffer data
  Dcm2_Controller *controller;
  //WinDriver *windriver;
  Dcm2_WinDriver *windriver;
  WDC_DEVICE_HANDLE hDev; // jseb2 handle
  WDC_DEVICE_HANDLE hDev2; // jseb2 handle
  Dcm2_JSEB2 *jseb2;
  Dcm2_JSEB2 *padccontroljseb2;

  DWORD dwAddrSpace;
  DWORD dwDMABufSize;

 
  WD_DMA *pDma_send;
  DWORD dwStatus;
  DWORD dwOptions_send;
  DWORD dwOffset;
  UINT32 u32Data;
  


  string adcJseb2Name;
 
   // start static

    unsigned short u16Data;
    unsigned long long u64Data, u64Data1;
 
    long imod,ichip;
    // end static
    unsigned short *buffp;
    //Jseb2Device *jseb2d;
/*
    WDC_ADDR_MODE mode;
    WDC_ADDR_RW_OPTIONS options;
*/
    // start static
    UINT32 i,j,k,ifr,nread,iprint,iwrite,ik,il,is,checksum;
    UINT32 istop,newcmd,irand,ioffset,kword,lastchnl,ib;
    
    UINT32 send_array[40000],read_array[dma_buffer_size];

    UINT32 nmask,index,itmp,nword_tot,nevent,iv,ijk,islow_read;
    UINT32 imod_p,imod_trig,imod_shaper;
 
    int icomp_l,comp_s,ia,ic,ihuff;
    // end static
    UINT32 *idcm_send_p,*idcm_verify_p,*pbuffp_rec;
 
    PVOID pbuf;
    WD_DMA *pDma;

    DWORD dwOptions = DMA_FROM_DEVICE;
    UINT32 iread,icheck,izero;
    UINT32 buf_send[40000];
    // start static
    int  wdc_started;
    int   count,num,counta,nword,ireadback,nloop,ierror;
    int   nsend,iloop,inew,idma_readback,iadd,jevent;
    int   itest,iframe,irun,ichip_c,dummy1,itrig_c;
    int  idup,ihold,idouble,ihold_set,istatus_read;
    int  idone,tr_bar,t_cs_reg,r_cs_reg,dma_tr;
    int   timesize,ipulse,ibase,a_id,itrig_delay;
    int   iset,ncount,nsend_f,nwrite,itrig_ext;
    int   imod_xmit,idiv,isample, isel_xmit, isel_dcm;
    int   iframe_length, itrig,idrift_time,ijtrig;
    int   idelay0, idelay1, threshold0, threshold1, pmt_words;
    int   cos_mult, cos_thres, en_top, en_upper, en_lower;
    int   irise, ifall, istart_time, use_pmt, pmt_testpulse;
    int   ich_head, ich_sample, ich_frm,idebug,ntot_rec,nred;
    int   ineu,ibusy_send,ibusy_test,ihold_word,ndma_loop;
    int   irawprint,ifem_fst,ifem_lst,ifem_loop,imod_fem;
    int   pmt_deadtime,pmt_mich_window, imod_dcm;
    int   oframe,osample,odiv,cframe,csample,cdiv;
    int   idac_shaper, pmt_dac_scan,pmt_precount, ichoice;
    int   inewcode, p1_delay, p1_width, pulse_trig_delay;
    int   p2_delay,p2_width,itrig_pulse,p3_delay,p3_width;
    int   ich, nsample, l1_delay;
    int   conf_add,conf_data, iext_trig, igen;
    int   imod_start, imod_end, nmod, iparity, itest_ram;
    int   nstep, nstep_event, ipattern, nstep_dac, idac_shaper_load;
    int   _num_xmitgroups;
 

    struct xmitgroup {
      int firstdigslot_nm;
      int num_dig_boards;
      int imod_xmit;
      int nsamples;
    } _xmit_groups[4];

    // end static
    unsigned char    charchannel;
    unsigned char    carray[4000];
    struct timespec tim, tim2;
    
    int trport2;
    int   activejseb2port; //port to write to
    PVOID pbuf_send;
    PVOID pbuf_rec;
    WD_DMA *pDma_rec;
    UINT32 *buf_rec;
//    DWORD dwOptions_rec = DMA_FROM_DEVICE | DMA_ALLOW_CACHE | DMA_ALLOW_64BIT_ADDRESS;
    DWORD dwOptions_rec;

    UINT64 *buffp_rec64;
    UINT32 *buffp_rec32;
    UINT32 *px, *py, *py1;

    FILE *outf,*inpf;
    bool verbosity;
    bool debug;

};

#endif
