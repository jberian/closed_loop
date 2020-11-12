
/***********************************************************************************
  Filename:     mDrip.c

  Description:  Medtronic-enabled BLE interface

  Version:      1
  Revision:     86
  Date:         Jun 14th, 2016

***********************************************************************************/

/***********************************************************************************
* INCLUDE
*/
#include "crc_4b6b.h"
#include "init.h"
#include "constants.h"
#include "interrupts.h"
#include "medtronicRF.h"
#include "smartRecovery.h"
#include "hal_board.h"
#include "hal_uart.h"
#include "bleComms.h"
#include "timingController.h"
#include "dataProcessing.h"
#include "freqManagement.h"
#include "pumpCommands.h"

/******************************************************************************
* GLOBAL VARIABLES
*/

         char   __xdata dataPacket              [ 90 ]                     ;
         char   __xdata dataErr                                            ;
unsigned int    __xdata dataLength                                         ;

         char   __xdata bleRxBuffer             [ SIZE_OF_UART_RX_BUFFER ] ;
         char   __xdata bleTxBuffer             [ SIZE_OF_UART_TX_BUFFER ] ;
         char   __xdata bleTxing                                           ;
         int    __xdata bleTxIndexIn                                       ;
         int    __xdata bleTxIndexOut                                      ;
         int    __xdata bleRxIndexIn                                       ;
         int    __xdata bleRxIndexOut                                      ;
         char   __xdata bleRxFlag                                          ;
        
         long   __xdata sgv, lastSgv                                       ;
         long   __xdata raw                                                ;
         long   __xdata lastraw                  [ 9 ]                     ;
         long   __xdata rawSgv                                             ;
         int    __xdata bgReading                                          ;
         char   __xdata adjValue                                           ;
unsigned char   __xdata warmUp                                             ;
         float  __xdata isig                                               ;
         float  __xdata pumpIOB                                            ;
         float  __xdata pumpBattery                                        ;
unsigned char   __xdata sensorAge                                          ;
         float  __xdata pumpReservoir                                      ;
         
         float  __xdata calFactor                                          ;
         char   __xdata bgUnits                                            ;
         char   __xdata carbUnits                                          ;
         char   __xdata lastCalMoment            [ 2 ]                     ;
         char   __xdata nextCalMoment            [ 2 ]                     ;
         char   __xdata mySentryFlag                                       ;
         char   __xdata minilinkFlag                                       ;
         char   __xdata bleSentFlag                                        ;
         char   __xdata sendFlag                                           ;
         char   __xdata queryPumpFlag                                      ;
         char   __xdata timeUpdatedFlag                                    ;
         char   __xdata resetRequested                                     ;

         char   __xdata glucoseDataSource                                  ;
         
         char   __xdata rfMode                                             ;
         char   __xdata rfRXOverflows                                      ;
         char   __xdata rfTXOverflows                                      ; 
         char   __xdata minilinkID               [ 3 ]                     ;
         char   __xdata pumpID                   [ 3 ]                     ;
         char   __xdata glucometerID             [ 3 ]                     ;
         char   __xdata glucometerEnable                                   ;
         char   __xdata minilinkBW                                         ;
         char   __xdata pumpBW                                             ;
         char   __xdata glucometerBW                                       ;
         char   __xdata genericFreq              [ 3 ]                     ;
         char   __xdata genericBW                                          ;
         char   __xdata minilinkRSSI             [ 33 ]                    ;
         char   __xdata pumpRSSI                 [ 33 ]                    ;
         char   __xdata glucometerRSSI           [ 33 ]                    ;
         char   __xdata lastMinilinkChannel                                ;
         char   __xdata lastPumpChannel                                    ;
         char   __xdata lastGlucometerChannel                              ;
         char   __xdata bestMinilinkChannel      [ 2 ]                     ;
         char   __xdata bestPumpChannel          [ 2 ]                     ;
         char   __xdata bestGlucometerChannel    [ 2 ]                     ;
         char   __xdata bestMinilinkFreqFound                              ;
         char   __xdata bestPumpFreqFound                                  ;
         char   __xdata bestGlucometerFreqFound                            ;
         
         char   __xdata txRFMode                                           ;
         
unsigned int    __xdata timingTable              [ 8 ][ 2 ]                ;
         char   __xdata missesTable              [ 8 ]                     ;
         int    __xdata fiveMinAdjTable          [ 8 ]                     ;
         char   __xdata timingTableCorrect                                 ;
         char   __xdata timingTableFrozen                                  ;

         char   __xdata lastMinilinkSeqNum                                 ;
         char   __xdata expectedSeqNum                                     ;
unsigned int    __xdata lastTimeStamp                                      ;
unsigned int    __xdata correctionTimeStamp                                ;
         char   __xdata lastRSSI                                           ;
         char   __xdata lastRSSIMode                                       ;
         char   __xdata rfFrequencyMode                                    ;
         char   __xdata syncMode                                           ;
         
unsigned char   __xdata systemTimeHour                                     ;
unsigned char   __xdata systemTimeMinute                                   ;
unsigned char   __xdata systemTimeSecond                                   ;
unsigned char   __xdata systemTimeSecondTimer                              ;
unsigned char   __xdata systemTimeSynced                                   ;
         
unsigned int    __xdata timeCounter                                        ;
unsigned int    __xdata timeCounterOnGap                                   ;
unsigned int    __xdata timeCounterOn                                      ;
unsigned int    __xdata timeCounterOff                                     ;
unsigned int    __xdata timeCounterOffGap                                  ;
unsigned char   __xdata timeCounterUndetermined                            ;
unsigned int    __xdata timeCounterHoldProcess                             ;
unsigned int    __xdata timeCounterBolusSnooze                             ;
unsigned char   __xdata timeCounterBolusSnoozeEnable                       ;
unsigned int    __xdata timeCounterPumpAwake                               ;
unsigned int    __xdata timeCounterTxInhibit                               ;


unsigned int    __xdata lookForMinilinkTimer                               ;
unsigned int    __xdata lookForMySentryTimer                               ;
unsigned int    __xdata rfOnTimer                                          ;
unsigned int    __xdata glucometerTimer                                    ;
unsigned int    __xdata reCalTimer                                         ;
unsigned int    __xdata bleCommsWatchdogTimer                              ;
         char   __xdata rfState                  [ 2 ]                     ;

unsigned int    __xdata txTimer                                            ;
unsigned char   __xdata txing                                              ;
         char   __xdata lastPumpCommandSent                                ;
unsigned int    __xdata lastPumpCommandLengthSent                          ;
         char   __xdata startRFTxFlag                                      ;
         char   __xdata adjustPumpFreqFlag                                 ;
         char   __xdata reconfigFlag                                       ;
         char   __xdata adjustSensitivityFlag                              ;
        
         char   __xdata pumpCommandsFlags                                  ; 
         float  __xdata bolusInsulinAmount                                 ;
         float  __xdata tempBasalRate                                      ;
unsigned int    __xdata tempBasalMinutes                                   ;
         float  __xdata tempBasalActiveRate                                ;
unsigned int    __xdata tempBasalTimeLeft                                  ;
unsigned int    __xdata tempBasalTimeTotal                                 ;
         
         int    __xdata historySgv               [ 16 ]                    ;
         int    __xdata historyRawSgv            [ 16 ]                    ;
         int    __xdata historyRaw               [ 16 ]                    ;
         char   __xdata historySgvValid          [ 16 ]                    ;
         char   __xdata historyRawSgvValid       [ 16 ]                    ;
         char   __xdata historyRawValid          [ 16 ]                    ;

unsigned char   __xdata minilinkRetransmit                                 ;
unsigned char   __xdata minilinkMinRSSI                                    ;

unsigned char   __xdata controlLoopMode                                    ;
         int    __xdata controlLoopThresHigh                               ;
         int    __xdata controlLoopThresLow                                ;
         int    __xdata controlLoopMaxTarget                               ;
         char   __xdata controlLoopAggressiveness                          ;

unsigned char   __xdata pumpTargetsHour          [ MAX_NUM_TARGETS ]       ;
unsigned char   __xdata pumpTargetsMinute        [ MAX_NUM_TARGETS ]       ;
unsigned char   __xdata pumpTargetsMin           [ MAX_NUM_TARGETS ]       ;
unsigned char   __xdata pumpTargetsMax           [ MAX_NUM_TARGETS ]       ;
unsigned char   __xdata pumpTargetsValidRanges                             ;
         
unsigned char   __xdata pumpBasalsHour           [ MAX_NUM_BASALS ]        ;
unsigned char   __xdata pumpBasalsMinute         [ MAX_NUM_BASALS ]        ;
unsigned char   __xdata pumpBasalsAmount         [ MAX_NUM_BASALS ]        ;
unsigned char   __xdata pumpBasalsValidRanges                              ;

unsigned char   __xdata insulinSensHour          [ MAX_NUM_INSULIN_SENS ]  ;
unsigned char   __xdata insulinSensMinute        [ MAX_NUM_INSULIN_SENS ]  ;
unsigned char   __xdata insulinSensAmount        [ MAX_NUM_INSULIN_SENS ]  ;
unsigned char   __xdata insulinSensValidRanges                             ;

         char   __xdata basalVectorsInit                                   ;
         int    __xdata basalIOBfuture           [ MAX_NUM_FUTURE_IOBS ]   ;
         int    __xdata basalIOBvector           [ MAX_NUM_IOB_REGISTERS ] ;
         int    __xdata bolusIOBvector           [ MAX_NUM_IOB_REGISTERS ] ;
unsigned int    __xdata responseToInsulin        [ MAX_NUM_IOB_REGISTERS ] ;
         int    __xdata iobAccumBasal                                      ;
         int    __xdata iobAccumBolus                                      ;
         
unsigned char   __xdata durationIOB                                        ;
unsigned int    __xdata maxBolus                                           ;
unsigned int    __xdata maxBasal                                           ;

         int    __xdata maxAbsorptionRate                                  ;
         char   __xdata maxAbsorptionRateEnable                            ;

         int    __xdata sensitivityUsed                                    ;
         float  __xdata basalUsed                                          ;
         
         float  __xdata measuredSensitivity                                ;
         char   __xdata measuredSensitivityValid                           ;
         float  __xdata measuredBasal                                      ;
         char   __xdata measuredBasalValid                                 ;
         
         float  __xdata tbdBasalIOB                                        ;
         float  __xdata instantIOB                                         ;
         float  __xdata basalIOB                                           ;
         float  __xdata basalIOBOriginal                                   ;
         float  __xdata bolusIOB                                           ;
         float  __xdata totalIOB                                           ;
         
extern unsigned char  __xdata pumpCmdQueuePendingNumber                    ;
extern          char  __xdata adjustPumpFreqFlag                           ;
extern          char  __xdata startRFTxFlag                                ;

extern char           __xdata rfMessage            [SIZE_OF_RF_BUFFER]      ;
extern int            __xdata rfMessagePointerOut                           ;
         
//Remove this after the test
char channel;
unsigned long freqSelected;
int expectedSgv;
int expectedSgvWoIOB;
unsigned char controlState;
int targetSgv;

float instantIOBHist[6];
int   sgvDeriv[6];
char  numHist = 0;
int   sensHist[6];
int   basalHist[6];
int   basalHistNum = 0;
char  numRes = 0;
char  safeMode; 

void powerManagementCtrl (void);

#ifdef _INIT_INFO_JESUS_
#define _INIT_INFO_
void initInfo (void) // Jesus
{
    pumpID[0] = 0x36;
    pumpID[1] = 0x01;
    pumpID[2] = 0x29;
    pumpRSSI[15] = 0xBB;    
    minilinkRetransmit = 0x01;
    minilinkMinRSSI = 0xFF;
    glucoseDataSource = 0x00;
    bestPumpFreqFound = 1;
  /*
    pumpID[0] = 0x42;
    pumpID[1] = 0x37;
    pumpID[2] = 0x63;
    pumpRSSI[13] = 0xBB; 
    minilinkRetransmit = 0x01;
    minilinkMinRSSI = 0xFF;
    rfFrequencyMode |= 0x02;
    glucoseDataSource = 0x02;
    bestPumpFreqFound = 1;
  */
  
    minilinkID[0] = 0x1F;
    minilinkID[1] = 0xFF;
    minilinkID[2] = 0x1E;
    minilinkRSSI[8] = 0xA9;
    timingTable[0][0] = 0x058C;
    timingTable[0][1] = 0x0041;
    timingTable[1][0] = 0x064A;
    timingTable[1][1] = 0x0041;
    timingTable[2][0] = 0x048D;
    timingTable[2][1] = 0x0055;
    timingTable[3][0] = 0x0537;
    timingTable[3][1] = 0x0037;
    timingTable[4][0] = 0x05D7;
    timingTable[4][1] = 0x005A;
    timingTable[5][0] = 0x06F4;
    timingTable[5][1] = 0x0046;
    timingTable[6][0] = 0x060E;
    timingTable[6][1] = 0x005A;
    timingTable[7][0] = 0x066D;
    timingTable[7][1] = 0x0055;
   
   /*
    minilinkID[0] = 0x22;
    minilinkID[1] = 0xE0;
    minilinkID[2] = 0x38;
    minilinkRSSI[6] = 0xA9;
    timingTable[0][0] = 0x064F;
    timingTable[0][1] = 0x0050;
    timingTable[1][0] = 0x050A;
    timingTable[1][1] = 0x0055;
    timingTable[2][0] = 0x074E;
    timingTable[2][1] = 0x0055;
    timingTable[3][0] = 0x0582;
    timingTable[3][1] = 0x0037;
    timingTable[4][0] = 0x06CC;
    timingTable[4][1] = 0x0041;
    timingTable[5][0] = 0x0442;
    timingTable[5][1] = 0x003C;
    timingTable[6][0] = 0x0514;
    timingTable[6][1] = 0x0046;
    timingTable[7][0] = 0x0695;
    timingTable[7][1] = 0x0037;
    */
  bestMinilinkFreqFound = 1;
  timingTableCorrect = 1;
  timingTableFrozen = 1;
  calculateFiveMinAdjustment();
  
  glucometerID[0] = 0xC0;
  glucometerID[1] = 0x0C;
  glucometerID[2] = 0x0F;
  glucometerEnable = 1;
  glucometerRSSI[6] = 0xAE; 
  bestGlucometerFreqFound = 1;
    
  controlLoopMode              = 1;
  controlLoopMaxTarget         = 130;
  adjustSensitivityFlag        = 0x03; //0x87 ;
  timeCounterBolusSnoozeEnable = 1;
  
  maxAbsorptionRate = 300; //5.0 U/h * 40
  maxAbsorptionRateEnable = 0;
  
  controlLoopAggressiveness = 5;
}
#endif

#ifdef _INIT_INFO_SUSANA_
#define _INIT_INFO_
void initInfo (void) // Susana
{ 
  /*
  pumpID[0] = 0x23; // 722
  pumpID[1] = 0x23;
  pumpID[2] = 0x55;
  pumpRSSI[10] = 0xAF;    
  minilinkRetransmit = 0x01;
  minilinkMinRSSI = 0xFF;
  glucoseDataSource = 0x02;
  bestPumpFreqFound = 1;
  */
  
  pumpID[0] = 0x93; // 723
  pumpID[1] = 0x13;
  pumpID[2] = 0x17;
  pumpRSSI[16] = 0xB3;    
  minilinkRetransmit = 0x01;
  minilinkMinRSSI = 0xFF;
  glucoseDataSource = 0x00;
  bestPumpFreqFound = 1;
  
  minilinkID[0] = 0x28;
  minilinkID[1] = 0x8A;
  minilinkID[2] = 0x90;

  //minilinkRSSI[6] = 0x89;
  //bestMinilinkFreqFound = 1;
  timingTable[0][0] = 0x0451;
  timingTable[0][1] = 0x0050;
  timingTable[1][0] = 0x067C;
  timingTable[1][1] = 0x005A;
  timingTable[2][0] = 0x067C;
  timingTable[2][1] = 0x004B;
  timingTable[3][0] = 0x066D;
  timingTable[3][1] = 0x0055;
  timingTable[4][0] = 0x0569;
  timingTable[4][1] = 0x004B;
  timingTable[5][0] = 0x05A5;
  timingTable[5][1] = 0x004B;
  timingTable[6][0] = 0x05DC;
  timingTable[6][1] = 0x0041;
  timingTable[7][0] = 0x0640;
  timingTable[7][1] = 0x005A;
  timingTableCorrect = 1;
  timingTableFrozen = 1;
  calculateFiveMinAdjustment();
  
  glucometerID[0] = 0xC1;
  glucometerID[1] = 0x1A;
  glucometerID[2] = 0x4E;
  glucometerEnable = 1;
  glucometerRSSI[0] = 0x00; 
  bestGlucometerFreqFound = 0;
  
  rfFrequencyMode = 0x07; 
  
  controlLoopMode              = 1;
  controlLoopMaxTarget         = 140;
  adjustSensitivityFlag        = 0x87 ;
  timeCounterBolusSnoozeEnable = 0;
  
  maxAbsorptionRate = 180; //4.5 U/h * 40
  maxAbsorptionRateEnable = 0;
  
  controlLoopAggressiveness = 5;
}
#endif

#ifdef _INIT_INFO_ALEXIA_
#define _INIT_INFO_
void initInfo (void) // Alexia
{
  pumpID[0] = 0x60;
  pumpID[1] = 0x98;
  pumpID[2] = 0x76;
  pumpRSSI[9] = 0xA9;    
  bestPumpFreqFound = 1;
  minilinkRetransmit = 0x01;
  minilinkMinRSSI = 0xFF;
  glucoseDataSource = 0x00;
  
  minilinkID[0] = 0x20;
  minilinkID[1] = 0xA2;
  minilinkID[2] = 0xB6;

  minilinkRSSI[9] = 0xB2;
  bestMinilinkFreqFound = 1;
  timingTable[0][0] = 0x05A5;
  timingTable[0][1] = 0x0046;
  timingTable[1][0] = 0x0668;
  timingTable[1][1] = 0x0041;
  timingTable[2][0] = 0x05A5;
  timingTable[2][1] = 0x0046;
  timingTable[3][0] = 0x064A;
  timingTable[3][1] = 0x003C;
  timingTable[4][0] = 0x0677;
  timingTable[4][1] = 0x0037;
  timingTable[5][0] = 0x04DD;
  timingTable[5][1] = 0x0046;
  timingTable[6][0] = 0x0618;
  timingTable[6][1] = 0x004B;
  timingTable[7][0] = 0x0578;
  timingTable[7][1] = 0x0050;
  timingTableCorrect = 1;
  timingTableFrozen = 1;
  calculateFiveMinAdjustment();
  
  glucometerID[0] = 0xC2;
  glucometerID[1] = 0xC8;
  glucometerID[2] = 0x50;
  glucometerEnable = 1;
  glucometerRSSI[9] = 0xAC; 
  bestGlucometerFreqFound = 1;
    
  controlLoopMode              = 1;
  controlLoopMaxTarget         = 140;
  adjustSensitivityFlag        = 0x87 ;
  timeCounterBolusSnoozeEnable = 0;
  
  maxAbsorptionRate = 180; //4.5 U/h * 40
  maxAbsorptionRateEnable = 0;
  
  controlLoopAggressiveness = 5;

}
#endif

#ifdef _INIT_INFO_SIMON_
#define _INIT_INFO_
void initInfo (void) // Simon
{
  /*
  pumpID[0] = 0x70; // 722
  pumpID[1] = 0x98;
  pumpID[2] = 0x81;
  pumpRSSI[11] = 0x78;    
  bestPumpFreqFound = 1;
  minilinkRetransmit = 0x01;
  minilinkMinRSSI = 0xFF;
  glucoseDataSource = 0x02;
  */
  /*
  pumpID[0] = 0x63; // 554
  pumpID[1] = 0x08;
  pumpID[2] = 0x70;
  pumpRSSI[9] = 0x94;    
  bestPumpFreqFound = 1;
  minilinkRetransmit = 0x01;
  minilinkMinRSSI = 0xFF;
  glucoseDataSource = 0x00;
  */
  
  pumpID[0] = 0x95; // 754
  pumpID[1] = 0x55;
  pumpID[2] = 0x56;
  pumpRSSI[9] = 0xA3;    
  bestPumpFreqFound = 1;
  minilinkRetransmit = 0x01;
  minilinkMinRSSI = 0xFF;
  glucoseDataSource = 0x00;
  
  /*
  minilinkID[0] = 0x23;
  minilinkID[1] = 0x7A;
  minilinkID[2] = 0x7C;
  //minilinkRSSI[6] = 0x91;
  //bestMinilinkFreqFound = 1;  
  timingTable[0][0] = 0x0708;
  timingTable[0][1] = 0x0037;
  timingTable[1][0] = 0x0532;
  timingTable[1][1] = 0x003C;
  timingTable[2][0] = 0x0541;
  timingTable[2][1] = 0x0041;
  timingTable[3][0] = 0x06C7;
  timingTable[3][1] = 0x0041;
  timingTable[4][0] = 0x05FF;
  timingTable[4][1] = 0x0037;
  timingTable[5][0] = 0x06FE;
  timingTable[5][1] = 0x0050;
  timingTable[6][0] = 0x05AF;
  timingTable[6][1] = 0x0046;
  timingTable[7][0] = 0x03F2;
  timingTable[7][1] = 0x0050;
  timingTableCorrect = 1;
  timingTableFrozen = 1;
  calculateFiveMinAdjustment();
  */
  
  minilinkID[0] = 0x26;
  minilinkID[1] = 0xC7;
  minilinkID[2] = 0xF7;
  minilinkRSSI[6] = 0x91;
  bestMinilinkFreqFound = 1;
  timingTable[0][0] = 0x07AD;
  timingTable[0][1] = 0x0037;
  timingTable[1][0] = 0x05D7;
  timingTable[1][1] = 0x0041;
  timingTable[2][0] = 0x04FB;
  timingTable[2][1] = 0x041B;
  timingTable[3][0] = 0x05BE;
  timingTable[3][1] = 0x005A;
  timingTable[4][0] = 0x0631;
  timingTable[4][1] = 0x0055;
  timingTable[5][0] = 0x06A9;
  timingTable[5][1] = 0x0000;
  timingTable[6][0] = 0x049C;
  timingTable[6][1] = 0x0037;
  timingTable[7][0] = 0x052D;
  timingTable[7][1] = 0x004B;
  //timingTableCorrect = 1;
  //timingTableFrozen = 1;
  //calculateFiveMinAdjustment();
  
  /*
  minilinkID[0] = 0x28;
  minilinkID[1] = 0xEC;
  minilinkID[2] = 0x76; 
  minilinkRSSI[5] = 0xD9;
  bestMinilinkFreqFound = 1;
  timingTable[0][0] = 0x055A;
  timingTable[0][1] = 0x003C;
  timingTable[1][0] = 0x069A;
  timingTable[1][1] = 0x004B;
  timingTable[2][0] = 0x05B4;
  timingTable[2][1] = 0x004B;
  timingTable[3][0] = 0x0654;
  timingTable[3][1] = 0x0037;
  timingTable[4][0] = 0x03C5;
  timingTable[4][1] = 0x005A;
  timingTable[5][0] = 0x073F;
  timingTable[5][1] = 0x0050;
  timingTable[6][0] = 0x0497;
  timingTable[6][1] = 0x0041;
  timingTable[7][0] = 0x0749;
  timingTable[7][1] = 0x0050;
  timingTableCorrect = 1;
  timingTableFrozen = 1;
  calculateFiveMinAdjustment();
  */  
  glucometerID[0] = 0xC2;
  glucometerID[1] = 0xC6;
  glucometerID[2] = 0xE9;
  glucometerEnable = 1;
  //glucometerRSSI[11] = 0xA3; 
  bestGlucometerFreqFound = 0;
    
  controlLoopMode              = 1;
  controlLoopMaxTarget         = 140;
  adjustSensitivityFlag        = 0x87 ;
  timeCounterBolusSnoozeEnable = 0;
  
  maxAbsorptionRate = 180; //4.5 U/h * 40
  maxAbsorptionRateEnable = 0;
  
  controlLoopAggressiveness = 5;
}
#endif

/******************************************************************************
* MAIN FUNCTION
*/

 int main(void)
{

  /* Configure system */
  GREEN_LED = 0; // Turn off green led
  P1_5 = 0; // Keep Wixel with reset active (low).
  initGlobals();
  initTimingTable();
#ifdef _INIT_INFO_
  initInfo();
#endif
  configureIO();
  configureOsc();
  crc16Init();
  configureMedtronicRFMode(0);
  configureUART();
  uart0StartRxForIsr();
  enableRadioInt();
    
  // Wait for HM10 Configuration
  //sleep(4);
  //initHM10();
    
  // Initial LED Flash
  sleep(1); GREEN_LED = 1; sleep(1); GREEN_LED = 0;
    
  enableTimerInt();
    
  /* Main loop */
  while (1) {
    powerManagementCtrl();
    timeManagementTask();

    // Check BLE comms
    checkBleUartComms();    
    
    // Check for Medtronic Messages
    checkMedtronicRF();
    
    // Query pump for new configuration
    queryPumpData();
    
    // Process received information
    processInfo();
    
    // Start sending RF messages if queued
    if ((startRFTxFlag == 1) && 
        (timeCounterTxInhibit == 0)) {
      sendNextPumpCmd();
      startRFTxFlag = 0;
    }
        
    // Manage the pump frequency search process
    if (adjustPumpFreqFlag == 1) {
      if (pumpCmdQueuePendingNumber == 0) {
        if ((txRFMode & 0x3F) < 32) {
          txRFMode++;
          pumpWakeUpRFTest();
        } else {
          checkIfBestPumpFreqWasFound ();
          txRFMode = getBestPumpMode();
          adjustPumpFreqFlag = 0;
        }
      }
    }
    
    if (resetRequested == 1) {
      sleep(2);
      reset();
    }
  }    
  
}

void powerManagementCtrl (void)
{
  #ifndef _DEBUG_MODE_
  // Check if there's something waiting to be attended...
  if ((startRFTxFlag == 0) && (adjustPumpFreqFlag == 0) &&
      (sendFlag == 0) && (queryPumpFlag == 0) && 
      (rfMessage[rfMessagePointerOut] == 0x00) &&
      (timeUpdatedFlag == 0) &&
      (bleRxFlag == 0) &&
      (bleTxIndexIn == bleTxIndexOut)) {
    // If not, put CPU to sleep. (PM0)
    SLEEP &= ~0x03;
    PCON = 0x01;   
  }
  #endif
}
