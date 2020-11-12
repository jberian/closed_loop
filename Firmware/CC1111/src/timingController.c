#include "ioCC1110.h"
#include "globals.h"
#include "timingController.h"
#include "medtronicRF.h"
#include "init.h"
#include "freqManagement.h"
#include "pumpCommands.h"

void initTimingTable (void)
{
  char j,k;
 
  for (j=0;j<8;j++) {
    for (k=0;k<2;k++) {
      timingTable     [j][k] = 0;
      fiveMinAdjTable [j] = 0;
    }
  }
}

void timingSanityCheck (void)
{
  char i,j;
  long correctionFactor;
  
  // Round timings to the closest "second"
  for (i=0;i<8;i++) {
    for (j=0; j<2; j++) {
      correctionFactor = timingTable[i][j] % TICKS_PER_SECOND;
      if (correctionFactor > (TICKS_PER_SECOND >> 1)) {
        timingTable[i][j] += (TICKS_PER_SECOND - correctionFactor);
      } else {
        timingTable[i][j] -= correctionFactor;
      }  
    }
  }
  
  // Remove non-valid timings
  for (i=0;i<8;i++) {
    if ( (timingTable[i][0] < ((3*60 + 0) * TICKS_PER_SECOND)) ||
        (timingTable[i][0] > ((7*60 +  0) * TICKS_PER_SECOND)) ) { 
      timingTable[i][0] = 0;
    }
    if ( (timingTable[i][1] < ((0*60 + 8) * TICKS_PER_SECOND)) ||
        (timingTable[i][1] > ((0*60 + 25) * TICKS_PER_SECOND)) ) { 
      timingTable[i][1] = 0;
    }
  }
}

unsigned int getTimeForNumSeq ( char seqNumQuery )
{
  char seqNum, subSeqNum;
  
  seqNum           = (seqNumQuery >> 4) & 0x07;
  subSeqNum        =  seqNumQuery & 0x01;
  
  return (timingTable[seqNum][subSeqNum]);
  
}

void updateTimingTable ( char newSeqNum, unsigned char lastSeqNum, unsigned int time )
{
  char seqNum, subSeqNum, seqNumCurrent, subSeqNumCurrent;
 
  // If we don't have the exact timing yet...
  if ((timingTableFrozen == 0) && (lastSeqNum <= 0x71)) {
  
    seqNum           = (lastSeqNum >> 4) & 0x0F;
    subSeqNum        =  lastSeqNum & 0x01;
    seqNumCurrent    = ( newSeqNum >> 4) & 0x0F;
    subSeqNumCurrent =   newSeqNum & 0x01;
  
    // If we received two following sequence numbers...
    if ((seqNumCurrent == ((seqNum + 1) % 8)) && (seqNum < 0x08) &&
        (time < ((8*60 + 0) * TICKS_PER_SECOND)) ) {
          
      // ... and both are the first tranmissions ...
      if ( (subSeqNum == 0) && (subSeqNumCurrent == 0) ) {
        if (timingTable[seqNum][0] == 0) {
          timingTable[seqNum][0] = time;
        } else {
          timingTable[seqNum][0] += time;
          timingTable[seqNum][0] /= 2;
        }
      }
    
      // ... and we have the first transmission for the previous
      //   sequence number and the second for the current one ...
      else if ( (subSeqNum == 0) && (subSeqNumCurrent == 1) ) {
        
        if ((timingTable[seqNumCurrent][1]) != 0 ) {
          if (timingTable[seqNum][0] == 0) {
            timingTable[seqNum][0]  = (time - timingTable[seqNumCurrent][1]);
          } else {
            timingTable[seqNum][0] += (time - timingTable[seqNumCurrent][1]);
            timingTable[seqNum][0] /= 2;
          }
        }
      
        if ((timingTable[seqNum][0]) != 0 ) {
          if (timingTable[seqNumCurrent][1] == 0) {
            timingTable[seqNumCurrent][1]  = (time - timingTable[seqNum][0]);
          } else {
            timingTable[seqNumCurrent][1] += (time - timingTable[seqNum][0]);
            timingTable[seqNumCurrent][1] /= 2;
          }
        }
          
      }
      
      else if ( (subSeqNum == 1) && (subSeqNumCurrent == 0) ) {
        if (timingTable[seqNum][0] == 0) {
          timingTable[seqNum][0]  = time;
        } else {
          timingTable[seqNum][0] += time;
          timingTable[seqNum][0] /= 2;
        }
      }   
    } else if ((seqNumCurrent == seqNum) && (time < ((1*60 + 0) * TICKS_PER_SECOND))) {
      if (timingTable[seqNum][1] == 0) {
        timingTable[seqNum][1]  = time;
      } else {
        timingTable[seqNum][1] += time;
        timingTable[seqNum][1] /= 2;
      }
    }
    
    timingSanityCheck();
  
    if (timingCorrect()) calculateFiveMinAdjustment();
  }
    
}

char timingCorrect (void)
{
  unsigned long timeSum;
  unsigned char i;

  if (timingTableFrozen == 1) {
    timingTableCorrect = 1;
  } else {
    timingTableCorrect = 0;
    timingTableFrozen  = 0;
  
    timeSum = 0;
    for (i=0;i<8;i++) timeSum += timingTable[i][0];
  
    if ((timeSum > ((8*5*60 + 3) * TICKS_PER_SECOND)) ||
        (timeSum < ((8*5*60 - 3) * TICKS_PER_SECOND))) return(0);
  
    for (i=0;i<8;i++) 
      if ((timingTable[i][1] < ((0*60 +  5) * TICKS_PER_SECOND)) ||
          (timingTable[i][1] > ((0*60 + 25) * TICKS_PER_SECOND))) return(0);
    
    // If the sum is exactly 5*8 minutes we freeze the timingTable.
    // We should probably need to check that using this table as it is
    // you are able to receive all the sequence numbers... but I'll leave
    // it like this by now.
    if (timeSum == (8*5*60) * TICKS_PER_SECOND) timingTableFrozen = 1;
  
    timingTableCorrect = 1;
  }
  
  return(1);
}

void calculateFiveMinAdjustment(void)
{
  unsigned int tempValue;
  int maxTime, timePeriod, timeAccum;
  char i;
  
  timePeriod = 0;
  timeAccum = 0;
  maxTime = -((20*60 + 0) * TICKS_PER_SECOND);
  for (i=0; i<8; i++) {
    timePeriod = ((5*60 + 0) * TICKS_PER_SECOND)*(i+1);
    timeAccum += timingTable[i][0];
    fiveMinAdjTable [i] = timePeriod - timeAccum;
    if (fiveMinAdjTable [i] > maxTime) 
      maxTime = fiveMinAdjTable [i];
  }
  
  tempValue = fiveMinAdjTable [7];
  for (i=7; i>0; i--) {
    fiveMinAdjTable [i] = maxTime - fiveMinAdjTable [i-1];
  }
  fiveMinAdjTable[0] = maxTime - tempValue;
  
}

void recalculateTiming(void)
{
  unsigned int nextActivation, timeIncrease;
  unsigned int i;
  unsigned char nextSeqNum, undetFlag, undetFlag01;
  
  timeCounterOn  = 0;
  timeCounterOff = 0;
  undetFlag      = 0;
  undetFlag01    = 0;
  timeIncrease   = 0;
  
  for (i=0; timeCounter > timeCounterOff; i++) {
    nextSeqNum = ((((((lastMinilinkSeqNum >> 4) & 0x07) + i) % 8) << 4) & 0xF0);
    nextActivation = getTimeForNumSeq (nextSeqNum);
    
    if ((nextActivation == 0) || (undetFlag == 1)) {
      undetFlag = 1;
      timeIncrease     += (5*60 + 30)*TICKS_PER_SECOND;
      timeCounterOnGap  = (2*60 + 30)*TICKS_PER_SECOND;
      timeCounterOffGap = (2*60 + 30)*TICKS_PER_SECOND;
    } else {
      timeCounterOnGap  = (0*60 + (1+i))*TICKS_PER_SECOND;
      if (missesTable[(nextSeqNum>>4)&0x07] & 0x01) 
                timeCounterOnGap += (0*60 + 10)*TICKS_PER_SECOND;
      timeIncrease  += nextActivation;
      nextActivation = getTimeForNumSeq (nextSeqNum | 0x01);
      if (nextActivation == 0) {
        timeCounterOffGap = (0*60 + (90+i))*TICKS_PER_SECOND;
        undetFlag01       = 1;
      } else {
        timeCounterOffGap = (0*60 + (30+i))*TICKS_PER_SECOND;
        undetFlag01       = 0;
      }
      if (missesTable[(nextSeqNum>>4)&0x07] & 0x02) 
              timeCounterOffGap += (0*60 + 10)*TICKS_PER_SECOND;
    }
    
    timeCounterOn  = timeIncrease                  - timeCounterOnGap;
    timeCounterOff = timeIncrease + nextActivation + timeCounterOffGap;       
  }
  
  expectedSeqNum = nextSeqNum;
  
  missesTable[(nextSeqNum>>4)&0x07] = 0x03;
  
  timeCounterUndetermined = undetFlag | undetFlag01;
}


void timeManagementTask (void)
{
  char periodicRx;
  static char prevMode = 0;
  static int ledBlinkCounter = 0;

  if (timeUpdatedFlag == 1) {
    timeUpdatedFlag = 0;
    
    // Update registers
    rfState[1] = rfState[0];  
    prevMode = rfMode;
    
    // Update auxiliary timers
    if (rfOnTimer > 0) rfOnTimer--;
    if ((txTimer > 0) && (rfTXMode == 0)) txTimer--;
    if (glucometerTimer > 0) glucometerTimer--;
    if ((rfState[0] == 1) && (rfTXMode == 0)) reCalTimer++;
    else                                      reCalTimer=0;
    if (bleCommsWatchdogTimer <= ((1*60 + 0) * TICKS_PER_SECOND)) bleCommsWatchdogTimer++;
    if (lookForMinilinkTimer > 0) lookForMinilinkTimer--;
    if (lookForMySentryTimer > 0) lookForMySentryTimer--;
    if (tempBasalTimeLeft > 0) tempBasalTimeLeft--;
    if (timeCounterHoldProcess > 0) timeCounterHoldProcess--;
    if (timeCounterBolusSnooze > 0) timeCounterBolusSnooze--;
    if (timeCounterPumpAwake > 0) timeCounterPumpAwake--;
    if (timeCounterTxInhibit > 0) timeCounterTxInhibit--;
    
    // Update system time
    systemTimeSecondTimer++;
    if (systemTimeSecondTimer == TICKS_PER_SECOND) {
      systemTimeSecondTimer = 0;
      systemTimeSecond++;
      if (systemTimeSecond == 60) {
        systemTimeSecond = 0;
        systemTimeMinute++;
        if (systemTimeMinute == 60) {
          systemTimeMinute = 0;
          systemTimeHour++;
          if (systemTimeHour == 24) {
            systemTimeHour = 0;
          }
        }
      }
    }
    
    // Recalculate Timing if needed
    if (timeCounterOff < timeCounter) recalculateTiming();
    
    // Sync Mode management
    if (rfOnTimer == 0) syncMode = 0;
  
    // Check if RF should be recalibrated due to excessive On-Time
    if (reCalTimer >= ((1*60 + 0)*TICKS_PER_SECOND - 1)) {
      reCalTimer = 0;
      configureMedtronicRFMode (rfMode);
    }
  
    // Check if RF should be on or off and in which mode
    if (glucometerEnable == 1) {
      if ((timeCounter & 0x07) == 0) periodicRx = 1;
      else periodicRx = 0;
    } else periodicRx = 0;
  
    // Check if there's a message waiting to be transmitted.
    if ((txing == 0) && (pumpCmdQueuePendingNumber > 0)) {
      startRFTxFlag = 1;
    } else if (( txing == 1 ) && (txTimer == 0)) {
      pumpCmdTimeout();
    }
    
    // Choose the right RF mode to be used
    if ( txing == 1 ) {
      rfMode = txRFMode;
      rfState[0] = 1;
      ledBlinkCounter = (ledBlinkCounter + 1) % 4;
      GREEN_LED = (ledBlinkCounter & 0x02) ? 1 : 0;
      
    } else if (rfMessageRXInProgress == 1) {
      rfState[0] = 1;
      GREEN_LED = 1;
      
    } else if (timeCounter >= (20*60 + 0)*TICKS_PER_SECOND) {
      if (glucoseDataSource == DATA_SOURCE_MYSENTRY) {
        if (bestMinilinkFreqFound == 1) {
          if   (lookForMySentryTimer > 0) rfMode = getBestPumpMode();
          else                            rfMode = getBestMinilinkMode();
        } else {
          if (lookForMinilinkTimer > 0)   rfMode = tryNextMinilinkMode();
          else                            rfMode = getBestPumpMode();
        }
      } else {
        if (bestMinilinkFreqFound == 1) {
          rfMode = getBestMinilinkMode ();
        } else {
          if (minilinkFlag > 0)   rfMode = tryNextMinilinkMode();
          else                    rfMode = getBestMinilinkMode();
        }
      }
      
      rfState[0] = 1;
      GREEN_LED = 1;
      if ((bestMinilinkFreqFound == 1) || 
          (glucoseDataSource != DATA_SOURCE_MYSENTRY)) {
        if (minilinkFlag > 0) {
          timeCounter   -= correctionTimeStamp;
          if ((mySentryFlag == 1) || 
              (glucoseDataSource != DATA_SOURCE_MYSENTRY)) {
            timeCounterOff = 0; // Force timing recalculation
          } else {
            timeCounterOn  = timeCounter + 1;
            timeCounterOff = timeCounter + (1*60 + 00)*TICKS_PER_SECOND; 
          }
          correctionTimeStamp = 0;
          resetHistoryLogs();
        }
      } else {
        if (mySentryFlag == 1) {
          timeCounter   -= correctionTimeStamp;
          timeCounterOn  = timeCounter + 1;
          timeCounterOff = timeCounter + (1*60 + 00)*TICKS_PER_SECOND; 
          resetHistoryLogs();
        }
      }
    } else if ((timeCounter >= timeCounterOn) && 
               (timeCounter <= timeCounterOff)) {
        
      if (timeCounter == timeCounterOn) {
        resetRFBuffers (); // Remove after the test
        correctionTimeStamp = 0;
        mySentryFlag = 0;
        minilinkFlag = 0;
        updateNextMinilinkMode();
        updateNextPumpMode();
        timeCounterHoldProcess = timeCounterOff - timeCounterOn + 
                                 (0*60 + 3)*TICKS_PER_SECOND;
      }
    
      if ((timeCounterUndetermined == 0) && (bestMinilinkFreqFound == 1)) {
        if ((timeCounter <= (timeCounterOn + timeCounterOnGap + 
                            (0*60 + 2)*TICKS_PER_SECOND)) && (minilinkFlag == 0)) {
          rfMode = getBestMinilinkMode();
          rfState[0] = 1;
          GREEN_LED = 1;
        } else if (timeCounter >= (timeCounterOff - timeCounterOffGap 
                                  - (0*60 + 2)*TICKS_PER_SECOND)) {
          if ((minilinkFlag & 0x02) == 0x00) {
            if ((minilinkFlag == 0) || (getTimeForNumSeq (expectedSeqNum | 0x01) == 0)
                || (bestMinilinkFreqFound == 1)) 
                                   rfMode = getBestMinilinkMode();
            else                   rfMode = tryNextMinilinkMode();
            rfState[0] = 1;
            GREEN_LED = 1;
          } else {
            if (mySentryFlag == 0) rfMode = getBestPumpMode();
            else                   rfMode = tryNextPumpMode();
            if (glucoseDataSource == DATA_SOURCE_MYSENTRY) {
              rfState[0] = 1;
              GREEN_LED = 1;
            } else {
              rfState[0] = 0;
              GREEN_LED = 0;
            }
          }
        } else {
          if (mySentryFlag == 0) rfMode = getBestPumpMode();
          else                   rfMode = tryNextPumpMode();
          if (glucoseDataSource == DATA_SOURCE_MYSENTRY) {
            rfState[0] = 1;
            GREEN_LED = 1;
          } else {
            rfState[0] = 0;
            GREEN_LED = 0;
          }
        }     
      } else {
        if (glucoseDataSource == DATA_SOURCE_MYSENTRY) {
          if (bestMinilinkFreqFound == 1) {
            if   (lookForMySentryTimer > 0) rfMode = getBestPumpMode();
            else                            rfMode = getBestMinilinkMode();
          } else {
            if (lookForMinilinkTimer > 0) rfMode = tryNextMinilinkMode();
            else                          rfMode = getBestPumpMode();
          }
        } else {
           if (minilinkFlag != 0)  rfMode = tryNextMinilinkMode ();
           else                    rfMode = getBestMinilinkMode ();
        }
        rfState[0] = 1;
        GREEN_LED = 1;
      }
    
      // If we have everything, don't waste more time (and battery)
      if (glucoseDataSource == DATA_SOURCE_MYSENTRY) {
        if ((bestMinilinkFreqFound == 1) && (bestPumpFreqFound == 1) &&
            (timingTableFrozen == 1) && (minilinkFlag > 0) && 
            (mySentryFlag == 1)) {
          timeCounterOff = timeCounter;
        } else if (bestMinilinkFreqFound == 1)  {
          if (((minilinkFlag & 0x02) == 0x02) && (mySentryFlag == 1) &&
              (timeCounterOff > timeCounter + (0*60 + 1)*TICKS_PER_SECOND)) {
            timeCounterOff = timeCounter;
          }
        } else {
          if ((mySentryFlag == 1) && (lookForMySentryTimer == 0) &&
              (lookForMinilinkTimer == 0)  &&
              (timeCounterOff > timeCounter + (0*60 + 1)*TICKS_PER_SECOND)) {
            timeCounterOff = timeCounter;
          }
        }     
      } else {
        if ((bestMinilinkFreqFound == 1) && (timingTableFrozen == 1) && 
            (minilinkFlag > 0)) {
          timeCounterOff = timeCounter;
        } else if (minilinkFlag & 0x02) {
          timeCounterOff = timeCounter;
        }
      }
    
      if ((timeCounter == timeCounterOff) &&
          (timeCounterOff > 0)) {
        queryPumpFlag = 1;
        if (((minilinkFlag > 0) && ((bestMinilinkFreqFound == 1) || 
                   (glucoseDataSource != DATA_SOURCE_MYSENTRY))) ||
            ((mySentryFlag > 0) && (bestMinilinkFreqFound == 0)) ){
          timeCounter   -= correctionTimeStamp;
          timeCounterOff = 0; // Force timing recalculation   
        }
        correctionTimeStamp = 0;
      }
    
      
    } else if (rfOnTimer > 0) {
      if (glucometerTimer > 0) {
        updateNextGlucometerMode();
        rfMode = tryNextGlucometerMode();
      }
      rfState[0] = 1;
      GREEN_LED = 1;
    } else if (periodicRx == 1) {
      rfMode = getBestGlucometerMode();
      rfState[0] = 1;   
    } else {
      #ifndef _DEBUG_MODE_
        rfState[0] = 0;
        GREEN_LED = 0;
      #endif
      #ifdef _DEBUG_MODE_
        rfState[0] = 1;
        GREEN_LED = 1;
      #endif
    }
    
    // Switch RF mode is necessary
    if (txing == 0) {
      if (((rfMode != prevMode) && (rfState[0] == 1)) ||
          ((rfState[0] == 1) && (rfState[1] == 0))){
        resetRFBuffers ();
        configureMedtronicRFMode (rfMode);
      }
    
      // Switch RF state
      if ((rfState[0] == 1) && (rfState[1] == 0)) {
        RFST = RFST_SRX;
      } else if ((rfState[0] == 0) && (rfState[1] == 1)) {
        RFST = RFST_SIDLE; 
      }
    }
    
  }
}