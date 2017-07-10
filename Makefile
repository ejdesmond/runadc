
    DCM2_BOOST_MAIN="${BOOST_MAIN}"
#   DCM2_JUNGO_MAIN="/home/phnxrc/haggerty/jseb/jungo/10.21/x86_64/WinDriver"

   OS_VER=3.10.0-327.el7.x86_64
   DCM2_JUNGO_MAIN=/export/software/WinVer122/WinDriver
   DCM2_SRC_DIR=/home/phnxrc/desmond/daq/build/

   DCM2_LIB_DIR=/export/software/Linux.${OS_VER}/lib/
   PHENIX_MAIN=/export/software/R17.0.0/online_distribution
   SPHENIX_MAIN_DIR=/home/phnxrc/softwarerepo/online_distribution
   SPHENIX_LIB_DIR=/home/phnxrc/softwarerepo/install/lib
   DCM2_JUNGO_LIB="wdapi1220"
#  BOOST_ROOT=/home/phnxrc/boost/boost_1_61_0
   BOOST_ROOT=/export/software/boost/boost_1_61_0


   ONLINE_MAIN=/export/software/Linux.${OS_VER}

   DCM2_CPPFLAGS="-DLINUX -DKERNEL_64BIT"
   DCM2_CXXFLAGS="$DCM2_CXXFLAGS -msse"

DBGFLAG = -g -v
ADDLDFLAGS="-lpthread -lrt"
INCLUDES =   -I${DCM2_SRC_DIR}  -I${BOOST_ROOT}/ -I${BOOST_ROOT}/boost -I${BOOST_ROOT}/boost/thread/ \
            -I/home/phnxrc/desmond/daq/build/includes -I/opt/phenix/include -I/usr/local/include -I/opt/phenix/include/pqxx  -I${DCM2_JUNGO_MAIN} -I${DCM2_JUNGO_MAIN}/include \
 -I${DCM2_JUNGO_MAIN}/include \
 -I${SPHENIX_MAIN_DIR}/newbasic \
 -I${DCM2_JUNGO_MAIN}/samples/shared \
 -I${DCM2_JUNGO_MAIN}/plx/lib \
 -I${DCM2_JUNGO_MAIN}/plx/diag_lib \
 -I${DCM2_JUNGO_MAIN}/my_projects 

# this is used for the g++ compiler
CXXFLAGS= -std=c++0x -DLinux  -g -O2 -Wall  -fPIC -DPIC  -DWITHTHREADS   $(INCLUDES) -I/usr/include/sqlplus  -I/opt/phenix/include  -I/home/desmond/Phenix/desmond/daq/build    -DHAVE_NAMESPACE_STD  -DHAVE_CXX_STRING_HEADER -DDLLIMPORT=""  $(DCM2_CPPFLAGS)


# library files

#C++             = g++ 
C++             = g++ 
G++             = gcc
C++FLAGS        =  -DLinux  -O2 -Wall -g  -fPIC -Wp,-MD -DPIC -Wl,-rpath=/export/software/boost/lib $(INCLUDES)	 \
				-DPRINTDBGMSG 

C++SUFFIX       = cc
LDFLAGS         =   -L${DCM2_LIB_DIR} -L${DCM2_JUNGO_MAIN}/lib -L/export/software/boost/lib -L${SPHENIX_LIB_DIR}  -lboost_thread -lboost_system -l${DCM2_JUNGO_LIB} -lpthread 

G++FLAGS        =  -DLinux -g  -O2 -Wall  -fPIC -Wp,-MD -DPIC -I. -DHAVE_STRING_H=1 -DHAVE_STDLIB_H=1 

G++SUFFIX       = cxx

C11_OBJS = \
	c11demo.o

ADC_TST_OBJS = \
	ADCTstServer.o \
	dcm2Impl.o \
	jseb2_lib.o \
	jseb2Controller.o \
	ADCexb.o

ADC_ONLY_OBJS = \
	jseb2_lib.o \
	ADCob.o \
	adconly.o 

ADC_TEST_OBJS = \
	adc.o \
	Jseb2Device.o \
	dcm2Impl.o \
	jseb2_lib.o \
	diag_lib.o \
	wdc_diag_lib.o \
	jseb2Controller.o \
	adctest.o 

ALLJSEB2_OBJS = \
	adc.o \
	Jseb2Device.o \
	dcm2Impl.o \
	jseb2_lib.o \
	diag_lib.o \
	wdc_diag_lib.o \
	jseb2ControllerNoWrite.o \
	alljsebadc.o 

RUNADC_OBJS= \
	adcj.o \
	Jseb2Device.o \
	dcm2Impl.o \
	jseb2_lib.o \
	diag_lib.o \
	wdc_diag_lib.o \
	jseb2Controller.o \
	runadc.o 

RUNADCGROUPS_OBJS= \
	adcj.o \
	Jseb2Device.o \
	dcm2Impl.o \
	jseb2_lib.o \
	diag_lib.o \
	wdc_diag_lib.o \
	jseb2Controller.o \
	runadcgroups.o 


ADC_OBJS = \
	ADCJ.o \
	Jseb2Device.o \
	dcm2Impl.o \
	jseb2_lib.o \
	diag_lib.o \
	wdc_diag_lib.o \
	jseb2Controller.o \
	ADCServer.o 

# external trigger - no xmit or dcm with busy reset command
ADC_EXTTRIG_OBJS = \
	jseb2_lib.o \
	diag_lib.o \
	wdc_diag_lib.o \
	sphenix_adc_test_exttrig_busy.o

# sarah's orig exttrig + xmit + dcm with reset busy added
ADC_EXTTRIG_BUSY_OBJS = \
	jseb2_lib.o \
	diag_lib.o \
	wdc_diag_lib.o \
	sphenix_adc_test_exttrig_xmit_dcm_busy.o

BUSYRESET_OBJS = \
	adccontrol.o \
	Jseb2Device.o \
	jseb2_lib.o \
	diag_lib.o \
	wdc_diag_lib.o \
	busyreset.o 

# sarah external trigger with xmit and dcm - no busy reset
SC_ADC_EXT_XMIT_OBJS = \
	jseb2_lib.o \
	diag_lib.o \
	wdc_diag_lib.o \
	sphenix_adc_test_exttrig_xmit_dcm_orig.o

# chi's original code
CHI_OBJS = \
	jseb2_lib.o \
	diag_lib.o \
	wdc_diag_lib.o \
	sphenix_adc_test_chi_xmit_dcm.o

DCM2_JSEB2_OBJS = \
	ADCRemServer.o \
	dcm2Impl.o \
	jseb2_lib.o \
	jseb2Controller.o \
	ADC.o


DCM2OBJS = \
	dcm2Server.o \
	dcm2Impl.o

DCM2_Q_OBJS = \
	dcm2qServer.o \
	dcm2Impl.o

DCM2_READER_OBJS = \
	dcm2_reader.o

DCM2_SA_OBJS = \
	dcm2.o

DCM2TESTOBJS = \
	dcm2TestServer.o  \
	dcm2Impl.o

DCM2CLIENTOBJS = \
	dcm2Client.o

JSEB2_SERVER_OBJS = \
	Testclass.o \
	jseb2ControllerObject.o \
	jseb2Server.o

ADC_LIB_OBJS = \
 jseb2_lib.o \
 diag_lib.o \
 wdc_diag_lib.o

JSEB2_OBJS = \
	jseb2.o

JSEB2_LOOPTEST_OBJS = \
	run2jsebs.o

SC_ADC_NOPULSE_OBJS = \
	jseb2_lib.o \
	sphenix_adc_test_nopulse.o

WIND_RESET_OBJS = \
	reset_windriver.o

DCM2_LIBS_OBJS = \
 Dcm2_BaseObject.o \
 Dcm2_Logger.o \
 Dcm2_FileBuffer.o \
 Dcm2_ControlledObject.o \
 Dcm2_WinDriver.o \
 Dcm2_Controller.o \
 Dcm2_JSEB2.o \
 Dcm2_Reader.o \
 Dcm2_Partition.o \
 Dcm2_Board.o \
 Dcm2_Partitioner.o \
 dcm2_runcontrol.o \
 msg_control.o

TESTSTRUCT_OBJS = \
 teststruct.o

c11demo: $(C11_OBJS)
	$(C++) -std=c++0x $(C++FLAGS)   -o $(ONLINE_MAIN)/bin/c11demo $(C11_OBJS)   $(LDFLAGS) $(SYSLIBS)

adctstserver: $(ADC_TST_OBJS)
	$(C++) $(C++FLAGS)   -o $(ONLINE_MAIN)/bin/adctstserver $(ADC_TST_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2 -lNoRootEvent -lmessage

adcserver:  $(ADC_OBJS)
	$(C++) $(C++FLAGS)   -o $(ONLINE_MAIN)/bin/adcserver $(ADC_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2 -lNoRootEvent -lmessage

adconly: $(ADC_ONLY_OBJS)
	$(C++) $(C++FLAGS) -o /home/phnxrc/bin/adconly $(ADC_ONLY_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2 -lNoRootEvent 

adctest:  $(ADC_TEST_OBJS)
	$(C++) $(C++FLAGS)   -o $(ONLINE_MAIN)/bin/adctest $(ADC_TEST_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2 -lNoRootEvent -lmessage

adcremserver: $(DCM2_JSEB2_OBJS)
	$(C++) $(C++FLAGS)   -o $(ONLINE_MAIN)/bin/adcremserver $(DCM2_JSEB2_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2


busyreset:  $(BUSYRESET_OBJS)
	$(C++) $(C++FLAGS)   -o $(ONLINE_MAIN)/bin/busyreset $(BUSYRESET_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2 -lNoRootEvent -lmessage

alljsebadc: $(ALLJSEB2_OBJS)
	$(C++) $(C++FLAGS)   -o $(ONLINE_MAIN)/bin/alljsebadc $(ALLJSEB2_OBJS)   $(LDFLAGS) $(SYSLIBS)  -lDcm2  -lNoRootEvent -lmessage

dcm2server: $(DCM2OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/dcm2server $(DCM2OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2

runadc: $(RUNADC_OBJS)
	$(C++) $(C++FLAGS)   -o $(ONLINE_MAIN)/bin/runadc $(RUNADC_OBJS)   $(LDFLAGS) $(SYSLIBS)  -lDcm2  -lNoRootEvent -lmessage

runadcgroups: $(RUNADCGROUPS_OBJS)
	$(C++) $(C++FLAGS)   -o $(ONLINE_MAIN)/bin/runadcgroups $(RUNADCGROUPS_OBJS)   $(LDFLAGS) $(SYSLIBS)  -lDcm2  -lNoRootEvent -lmessage


dcm2qserver: $(DCM2_Q_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/dcm2qserver $(DCM2_Q_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2

dcm2: $(DCM2_SA_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/dcm2 $(DCM2_SA_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2

dcm2libs: $(DCM2_LIBS_OBJS)
	$(C++) $(C++FLAGS)  -shared -Wall -Werror -fPIC   -o $(ONLINE_MAIN)/lib/libDcm2.so $(DCM2_LIBS_OBJS)

adclibs: $(ADC_LIB_OBJS)
	$(C++) $(C++FLAGS)  -shared -Wall -Werror -fPIC   -o $(ONLINE_MAIN)/lib/libADC.so $(adc_LIB_OBJS)

dcm2reader: $(DCM2_READER_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/dcm2reader $(DCM2_READER_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2

dcm2adc: $(DCM2_ADC_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/dcm2adc $(DCM2_ADC_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2



sphenix_adc_test_exttrig_xmit_dcm_orig: $(SC_ADC_EXT_XMIT_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/sphenix_adc_test_exttrig_xmit_dcm_orig $(SC_ADC_EXT_XMIT_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2

sphenix_adc_test_exttrig_xmit_dcm_busy: $(ADC_EXTTRIG_BUSY_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/sphenix_adc_test_exttrig_xmit_dcm_busy $(ADC_EXTTRIG_BUSY_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2

sphenix_adc_test_exttrig_busy: $(ADC_EXTTRIG_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/sphenix_adc_test_exttrig_busy $(ADC_EXTTRIG_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  

scadcnopulse: $(SC_ADC_NOPULSE_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/scadcnopulse $(SC_ADC_NOPULSE_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2


chitest: $(CHI_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/chitest $(CHI_OBJS)   $(LDFLAGS) \
					$(SYSLIBS) 

resetwind: $(WIND_RESET_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/resetwind $(WIND_RESET_OBJS)   $(LDFLAGS) $(SYSLIBS) 

# test socket connection 
dcm2testserver: $(DCM2TESTOBJS)
	$(C++) $(C++FLAGS) -o dcm2testserver $(DCM2TESTOBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2


dcm2Client: $(DCM2CLIENTOBJS)
	$(C++) $(C++FLAGS) -o dcm2client $(DCM2CLIENTOBJS)   $(LDFLAGS) \
					$(SYSLIBS)  

jseb2Server: $(JSEB2_SERVER_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/jseb2Server $(JSEB2_SERVER_OBJS)   $(LDFLAGS) \
					$(SYSLIBS)  -lDcm2

jseb2:  $(JSEB2_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/jseb2 $(JSEB2_OBJS)   $(LDFLAGS) \
					$(SYSLIBS) -lDcm2

jseb2M: jseb2M.o
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/jseb2M jseb2M.o   $(LDFLAGS) \
					$(SYSLIBS) -lDcm2

run2jsebs:  $(JSEB2_LOOPTEST_OBJS)
	$(C++) $(C++FLAGS) -o $(ONLINE_MAIN)/bin/run2jsebs $(JSEB2_LOOPTEST_OBJS)   $(LDFLAGS) \
					$(SYSLIBS) -lDcm2

staticobj: staticobj.o
	$(C++) $(C++FLAGS) -o staticobj staticobj.o   $(LDFLAGS) \
					$(SYSLIBS)

getglibcver: getglibcversion.o
	$(C++) $(C++FLAGS) -o getglibcver getglibcversion.o    \
					$(SYSLIBS) 

teststruct: $(TESTSTRUCT_OBJS)
	$(C++) $(C++FLAGS) -o teststruct teststruct.o   $(LDFLAGS) \
					$(SYSLIBS)


JAVAC =  $(JAVAHOME)/bin/javac
CLASSPATH = .:$(JAVA_HOME)/jre/lib/rt.jar

# cpp applications



# JAVA APPLICATIONS

ADC_CONTROLS_SRC = \
	ADCServerMain.java

DCM2_CONTROLS_SRC = \
	Dcm2ControlsFrame.java \
	Dcm2CrateDialog.java \
	DigitizerCrateDialog.java

DCM2_CLIENT_SRC = \
	Dcm2ClientMainFrame.java

adccontrols: $(ADC_CONTROLS_SRC)
	javac -deprecation -classpath  $(CLASSPATH) -Xlint $(ADC_CONTROLS_SRC)

dcm2controls: $(DCM2_CONTROLS_SRC)
	javac -deprecation -classpath  $(CLASSPATH) -Xlint $(DCM2_CONTROLS_SRC)

SPClient: SPClient.java
	$(JAVAC) -deprecation -classpath $(CLASSPATH) SPClient.java

dcm2client: $(DCM2_CLIENT_SRC)
	javac -deprecation -classpath  $(CLASSPATH) -Xlint $(DCM2_CLIENT_SRC)

#%.o: %.$(C++SUFFIX)
#	g++ -c  $(INCLUDES) $(C++FLAGS) $(DCM2_CPPFLAGS)  $<

%.o: %.c
	cc -c $(INCLUDES) $(C++FLAGS) $<
