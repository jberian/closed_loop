#include "hal_types.h"
#include "constants.h"

extern char           __xdata dataPacket           [ 90 ]                   ;
extern char           __xdata dataErr                                       ;
extern unsigned int   __xdata dataLength                                    ;
extern char           __xdata rfMessage            [SIZE_OF_RF_BUFFER]      ;
extern int            __xdata rfMessageTxLength                             ;
extern int            __xdata rfMessagePointerIn                            ;
extern int            __xdata rfMessagePointerOut                           ;
extern int            __xdata rfMessagePointerLen                           ;
extern char           __xdata rfMessageRXInProgress                         ;
extern char           __xdata rfLastData                                    ;
extern char           __xdata rfTXMode                                      ;
extern char           __xdata rfMessageTx          [SIZE_OF_RF_BUFFER]      ;
extern int            __xdata rfMessagePointerTx                            ;
extern char           __xdata bleRxBuffer          [SIZE_OF_UART_RX_BUFFER] ;
extern char           __xdata bleTxBuffer          [SIZE_OF_UART_TX_BUFFER] ;
extern char           __xdata bleTxing                                      ;
extern char           __xdata bleRxFlag                                     ;
extern int            __xdata bleTxIndexIn                                  ;
extern int            __xdata bleTxIndexOut                                 ;
extern int            __xdata bleRxIndexIn                                  ;
extern int            __xdata bleRxIndexOut                                 ;
extern int            __xdata txCalcCRC                                     ;
extern int            __xdata txCalcCRC16                                   ;
extern char           __xdata txLength                                      ;
extern int            __xdata txTimes                                       ;
extern char           __xdata lastMinilinkSeqNum                            ;
extern char           __xdata expectedSeqNum                                ;
extern unsigned int   __xdata lastTimeStamp                                 ;
extern unsigned int   __xdata correctionTimeStamp                           ;
extern char           __xdata lastRSSI                                      ;
extern char           __xdata lastRSSIMode                                  ;
extern char           __xdata rfFrequencyMode                               ;
extern char           __xdata syncMode                                      ;
extern char           __xdata minilinkID           [ 3 ]                    ;
extern char           __xdata pumpID               [ 3 ]                    ;
extern char           __xdata glucometerID         [ 3 ]                    ;
extern char           __xdata glucometerEnable                              ;
extern char           __xdata minilinkBW                                    ;
extern char           __xdata pumpBW                                        ;
extern char           __xdata glucometerBW                                  ;
extern char           __xdata genericFreq          [ 3 ]                    ;
extern char           __xdata genericBW                                     ;
extern char           __xdata txRFMode                                      ;
extern unsigned char  __xdata systemTimeHour                                ;
extern unsigned char  __xdata systemTimeMinute                              ;
extern unsigned char  __xdata systemTimeSecond                              ;
extern unsigned char  __xdata systemTimeSecondTimer                         ;
extern unsigned char  __xdata systemTimeSynced                              ;
extern unsigned int   __xdata timeCounter                                   ;
extern unsigned int   __xdata timeCounterOnGap                              ;
extern unsigned int   __xdata timeCounterOn                                 ;
extern unsigned int   __xdata timeCounterOff                                ;
extern unsigned int   __xdata timeCounterOffGap                             ;
extern unsigned char  __xdata timeCounterUndetermined                       ;
extern unsigned int   __xdata timeCounterHoldProcess                        ;
extern unsigned int   __xdata timeCounterBolusSnooze                        ;
extern unsigned char  __xdata timeCounterBolusSnoozeEnable                  ;
extern unsigned int   __xdata timeCounterPumpAwake                          ;
extern unsigned int   __xdata timeCounterTxInhibit                          ;
extern unsigned int   __xdata lookForMinilinkTimer                          ;
extern unsigned int   __xdata lookForMySentryTimer                          ;
extern unsigned int   __xdata rfOnTimer                                     ;
extern unsigned int   __xdata glucometerTimer                               ;
extern unsigned int   __xdata reCalTimer                                    ;
extern unsigned int   __xdata bleCommsWatchdogTimer                         ;
extern char           __xdata rfState              [ 2 ]                    ;
extern unsigned int   __xdata txTimer                                       ;
extern unsigned char  __xdata txing                                         ;
extern          char  __xdata lastPumpCommandSent                           ;
extern unsigned int   __xdata lastPumpCommandLengthSent                     ;
extern          char  __xdata pumpCommandsFlags                             ;
extern         float  __xdata bolusInsulinAmount                            ;
extern         float  __xdata tempBasalRate                                 ;
extern unsigned int   __xdata tempBasalMinutes                              ;
extern          float __xdata tempBasalActiveRate                           ;
extern unsigned int   __xdata tempBasalTimeLeft                             ;
extern unsigned int   __xdata tempBasalTimeTotal                            ;
extern char           __xdata mySentryFlag                                  ;
extern char           __xdata minilinkFlag                                  ;
extern char           __xdata bleSentFlag                                   ;
extern char           __xdata sendFlag                                      ;
extern char           __xdata queryPumpFlag                                 ;
extern char           __xdata timeUpdatedFlag                               ;
extern char           __xdata reconfigFlag                                  ;
extern char           __xdata resetRequested                                ;
extern char           __xdata adjustSensitivityFlag                         ;
extern char           __xdata glucoseDataSource                             ;
extern          long  __xdata sgv, lastSgv                                  ;
extern          long  __xdata raw                                           ;
extern          long  __xdata rawSgv                                        ;
extern          int   __xdata bgReading                                     ;
extern          char  __xdata adjValue                                      ;
extern unsigned char  __xdata warmUp                                        ;
extern          long  __xdata lastraw              [ 9 ]                    ;
extern float          __xdata isig                                          ;
extern float          __xdata pumpIOB                                       ;
extern         float  __xdata pumpBattery                                   ;
extern unsigned char  __xdata sensorAge                                     ;
extern         float  __xdata pumpReservoir                                 ;

extern float          __xdata calFactor                                     ;
extern char           __xdata lastCalMoment        [ 2 ]                    ;
extern char           __xdata nextCalMoment        [ 2 ]                    ;
extern unsigned int   __xdata timingTable          [ 8 ][ 2 ]               ;
extern          char  __xdata missesTable          [ 8 ]                    ;
extern          int   __xdata fiveMinAdjTable      [ 8 ]                    ;
extern          char  __xdata timingTableCorrect                            ;
extern          char  __xdata timingTableFrozen                             ;
extern          int   __xdata historySgv           [ 16 ]                   ;
extern          int   __xdata historyRawSgv        [ 16 ]                   ;
extern          int   __xdata historyRaw           [ 16 ]                   ;
extern          char  __xdata historySgvValid      [ 16 ]                   ;
extern          char  __xdata historyRawSgvValid   [ 16 ]                   ;
extern          char  __xdata historyRawValid      [ 16 ]                   ;
extern          char  __xdata rfMode                                        ;
extern          char  __xdata rfRXOverflows                                 ;
extern          char  __xdata rfTXOverflows                                 ; 
extern          char  __xdata minilinkRSSI         [ 33 ]                   ;
extern          char  __xdata pumpRSSI             [ 33 ]                   ;
extern          char  __xdata glucometerRSSI       [ 33 ]                   ;
extern          char  __xdata lastMinilinkChannel                           ;
extern          char  __xdata lastPumpChannel                               ;
extern          char  __xdata lastGlucometerChannel                         ;
extern          char  __xdata bestMinilinkChannel   [2]                     ;
extern          char  __xdata bestPumpChannel       [2]                     ;
extern          char  __xdata bestGlucometerChannel [2]                     ;
extern          char  __xdata bestMinilinkFreqFound                         ;
extern          char  __xdata bestPumpFreqFound                             ;
extern          char  __xdata bestGlucometerFreqFound                       ;

extern unsigned char  __xdata pumpCmdQueuePendingNumber                     ;
extern          char  __xdata adjustPumpFreqFlag                            ;
extern          char  __xdata startRFTxFlag                                 ;
extern          int   __xdata txTimes                                       ;

extern unsigned char  __xdata pumpTargetsHour          [ MAX_NUM_TARGETS ]  ;
extern unsigned char  __xdata pumpTargetsMinute        [ MAX_NUM_TARGETS ]  ;
extern unsigned char  __xdata pumpTargetsMin           [ MAX_NUM_TARGETS ]  ;
extern unsigned char  __xdata pumpTargetsMax           [ MAX_NUM_TARGETS ]  ;
extern unsigned char  __xdata pumpTargetsValidRanges                        ;

extern unsigned char  __xdata pumpBasalsHour        [ MAX_NUM_BASALS ]      ;
extern unsigned char  __xdata pumpBasalsMinute      [ MAX_NUM_BASALS ]      ;
extern unsigned char  __xdata pumpBasalsAmount      [ MAX_NUM_BASALS ]      ;
extern unsigned char  __xdata pumpBasalsValidRanges                         ;

/*extern unsigned char  __xdata carbRatioHour         [ MAX_NUM_CARB_RATIOS ] ;
extern unsigned char  __xdata carbRatioMinute       [ MAX_NUM_CARB_RATIOS ] ;
extern unsigned char  __xdata carbRatioAmount       [ MAX_NUM_CARB_RATIOS ] ;
extern unsigned char  __xdata carbRatioValidRanges                          ;
extern char           __xdata carbUnits                                     ;*/

extern unsigned char  __xdata insulinSensHour       [ MAX_NUM_INSULIN_SENS ];
extern unsigned char  __xdata insulinSensMinute     [ MAX_NUM_INSULIN_SENS ];
extern unsigned char  __xdata insulinSensAmount     [ MAX_NUM_INSULIN_SENS ];
extern unsigned char  __xdata insulinSensValidRanges                        ;
extern char           __xdata bgUnits                                       ;

extern unsigned char  __xdata minilinkRetransmit                            ;
extern unsigned char  __xdata minilinkMinRSSI                               ;

extern unsigned char  __xdata controlLoopMode                               ;
extern          int   __xdata controlLoopThresHigh                          ;
extern          int   __xdata controlLoopThresLow                           ;
extern          int   __xdata controlLoopMaxTarget                          ;

//extern          int   __xdata controlLoopSuspendLow                       ;
extern          char  __xdata controlLoopAggressiveness                     ;

extern          char  __xdata basalVectorsInit                              ;
extern          int   __xdata basalIOBfuture       [  MAX_NUM_FUTURE_IOBS  ];
extern          int   __xdata basalIOBvector       [ MAX_NUM_IOB_REGISTERS ];
extern          int   __xdata bolusIOBvector       [ MAX_NUM_IOB_REGISTERS ];
extern unsigned int   __xdata responseToInsulin    [ MAX_NUM_IOB_REGISTERS ];
extern          int   __xdata iobAccumBasal                                 ;
extern          int   __xdata iobAccumBolus                                 ;

extern unsigned char  __xdata durationIOB                                   ;
extern unsigned int   __xdata maxBolus                                      ;
extern unsigned int   __xdata maxBasal                                      ;

extern          int   __xdata maxAbsorptionRate                             ;
extern          char  __xdata maxAbsorptionRateEnable                       ;

extern          int   __xdata sensitivityUsed                               ;
extern          float __xdata basalUsed                                     ;

extern          float __xdata measuredSensitivity                           ;
extern          float __xdata measuredBasal                                 ;
extern          char  __xdata measuredSensitivityValid                      ;
extern          char  __xdata measuredBasalValid                            ;

extern          float __xdata tbdBasalIOB                                   ;
extern          float __xdata instantIOB                                    ;
extern          float __xdata basalIOB                                      ;
extern          float __xdata basalIOBOriginal                              ;
extern          float __xdata bolusIOB                                      ;
extern          float __xdata totalIOB                                      ;

extern          char  __xdata breakPointId                                  ;

//Remove this after the test
extern char channel;
extern unsigned long freqSelected;
extern int expectedSgv;
extern int expectedSgvWoIOB;
extern int targetSgv;
extern unsigned char controlState;
extern float instantIOBHist[6];
extern int   sgvDeriv[6];
extern char  numHist;
extern int   sensHist[6];
extern int   basalHist[6];
extern int   basalHistNum;
extern char  numRes;
extern char  safeMode;
