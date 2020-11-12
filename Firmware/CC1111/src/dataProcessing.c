#include "ioCCxx10_bitdef.h"
#include "ioCC1110.h"
#include "bleComms.h"
#include "constants.h"
#include "globals.h"
#include "interrupts.h"
#include "timingController.h"
#include "dataProcessing.h"
#include "freqManagement.h"
#include "pumpCommands.h"
#include "crc_4b6b.h"
#include "medtronicRF.h"
#include "controlLoop.h"

void processMessage (char *message, unsigned int length, char dataErr)
{
  static char lastSeqNum;
  long localSgv;
  char i;
  int tempVar;
  
  // First check if the message has a correct CRC
  if (dataErr == 0) {
    
    // Second check if it's a minilink message
    if (((message[0] == 0xAA) || (message[0] == 0xAB)) && (length == 34) &&
         ((message[2] == minilinkID[0]) && (message[3] == minilinkID[1]) &&
           (message[4] == minilinkID[2])))  {
     
      // First let's see if we need to retransmit the message.
      switch(minilinkRetransmit) {
        
      // Always retransmit if RSSI is below the min. (MinRSSI = 0xFF for bridge mode)
      case 0x01:
        if (lastRSSI < minilinkMinRSSI) {
          addMsgToQueue (message, length, 1, 0x00, (0*60 + 1)*TICKS_PER_SECOND, 0);
        }
        break;
        
      // Retransmit if the pump did not send the MySentry message after the
      // second minilink message
      case 0x02:
        if ((message[8] & 0x01) && (mySentryFlag == 0)) {
           addMsgToQueue (message, length, 1, 0x00, (0*60 + 1)*TICKS_PER_SECOND, 0);
        }
      default:
        break;
      }
      
      // Update the timing table in order to get the timing for the current
      // minilink transmitter.
      updateTimingTable(message[8],lastMinilinkSeqNum,lastTimeStamp - correctionTimeStamp);
      
      // Process the message only if it's different to the previous one
      if ((lastMinilinkSeqNum & 0xF0) != (message[8] & 0xF0)) {
        lastMinilinkSeqNum = message[8];
	warmUp     = (message[0] == 0xAA) ? 1 : 0;
        adjValue   =  message[7];
        raw        = (message[ 9] & 0x0FF)*256  + (message[10] & 0x0FF);
        lastraw[0] = (message[11] & 0x0FF)*256  + (message[12] & 0x0FF);
	for (i=0; i<7; i++) {
	  lastraw[i+1] = (message[17+(i*2)] & 0x0FF)*256 + (message[17+(i*2)+1] & 0x0FF);
	}    
        if (correctionTimeStamp == 0) {
          correctionTimeStamp = lastTimeStamp;
          if (message[8] & 0x01) {
            if (getTimeForNumSeq(message[8]) == 0) {
              correctionTimeStamp -= (0*60 + 18)*TICKS_PER_SECOND;
            } else {
              correctionTimeStamp -= getTimeForNumSeq(message[8]);
            }
          }
        }
      }
      if (message[8] & 0x01) minilinkFlag |= 0x02;
      else                   minilinkFlag |= 0x01;
      
      if (message[8] & 0x01) missesTable[(message[8]>>4)&0x07] &= ~0x02;
      else                   missesTable[(message[8]>>4)&0x07] &= ~0x01;
      
      lastMinilinkSeqNum   = message[8];
      
      lookForMySentryTimer = (0*60 + 8)*TICKS_PER_SECOND;
      lookForMinilinkTimer = 0;
    }
    
    // MySentry Message
    else if (message[0] == 0xA2) {
       if ( (message[1] == pumpID[0]) &&
            (message[2] == pumpID[1]) &&
            (message[3] == pumpID[2])) {
        if (syncMode == 1) {
          respondToDeviceSearch(message,length,dataErr);         
        } else if ( (message[4] == 0x04)      &&
             (message[5] != lastSeqNum &&
              mySentryFlag == 0) ) {
          lastSeqNum = message[5];
        
          localSgv  = message[14] << 1;
          localSgv += message[29]&0x01;
        
          tempVar  = message[27] << 8;
          tempVar |= message[28];
          if (timeCounterBolusSnoozeEnable == 1) {
            pumpIOB        = (float)(tempVar) * 0.025;
          }
        
          if (localSgv >= 40) {
            sgv      = localSgv;
            lastSgv  =  message[15]<<1;
            lastSgv += (message[29]>>1)&0x01;
            mySentryFlag = 1;
          }
          
          lookForMySentryTimer = 0;
          lookForMinilinkTimer = (0*60 + 30)*TICKS_PER_SECOND;
        
          if ((correctionTimeStamp == 0) && (bestMinilinkFreqFound == 0)) {
            correctionTimeStamp = lastTimeStamp;
            lastMinilinkSeqNum = 0x80;
          }
        }
      }
    }
    
    // Pump message
    else if (message[0] == 0xA7) {
      if ( (message[1] == pumpID[0]) &&
           (message[2] == pumpID[1]) &&
           (message[3] == pumpID[2]) ) {
        // We suppose this was the answer for the last command
        if (txing == 1) {
          if (message[4] == pumpExpectedAnswer()) {
            pumpCommandsCallback(0,message,length);
            removeMsgBeingProccessed();
          }
        } else {
          // ... someone is trying to command my pump?? 
          // Bad thing.       
          if (adjustSensitivityFlag & 0x80) {
            // Counter-measure
            suspendJamming();
          }
        }
      }
    }
    
    // Glucometer Reading
    else if ((message[0] == 0xA5) && (length == 7) &&
       ((message[1] == glucometerID[0]) && (message[2] == glucometerID[1]) &&
        (message[3] == glucometerID[2]))) {
      if (glucometerTimer == 0) {
        if ((message[4] & 0xFE) == 0x00) { // Avoid ACKs
          glucometerTimer = ((0*60 + 20) * TICKS_PER_SECOND);
          bgReading = ((message[4] & 0x01)*256 + message[5]) & 0x01FF;
          composeGlucometerMessage();
        }
      }
    }
    
    if ((minilinkFlag > 0) && (mySentryFlag == 1)) {
      rfOnTimer = 0;
    }
  }
}

float getISIGfromRAW (long raw, char adjValue)
{
  float isig;
  
  // With adjValue = 0x21 -> Error +1.09239
  //                 0x17 -> Error +1.065
  
  //    coc raw
  // 22 151 148
  // 22  97  94
  // 22 134 131
  // 22  74  71
  //  
  // 20 122 124
  // 20  82  83
  // 20 108 109
  // 20 126 128
          
        
  isig = 143.2025 * (1.0 + (0.0029*(float)(adjValue)));
  isig = ((float)(raw & 0x0FFFF)) / isig; 
  isig -= 2.9476;

  return(isig);
}

void queryPumpData (void)
{
  if ((queryPumpFlag == 1) && (timeCounterHoldProcess == 0)) {
    queryPumpFlag = 2;
    
    readPumpConfig();
  }
}

void processInfo (void)
{
  if (sendFlag == 1) {
    sendFlag = 0;
    
    updateHistoryData();
    updateIOBvectors();
    composeInfoUpdateMessage();
    controlLoopDataUpdate();
  
    mySentryFlag = 0;
    minilinkFlag = 0;
  }
}

void updateHistoryData (void)
{
  int i;
  static float lastCalFactor;
  float internalCal;
  
  for (i=15; i>0; i--) {
    historySgv         [i] = historySgv         [i-1];
    historySgvValid    [i] = historySgvValid    [i-1];
    historyRawSgv      [i] = historyRawSgv      [i-1];
    historyRawSgvValid [i] = historyRawSgvValid [i-1];
    historyRaw         [i] = historyRaw         [i-1];
    historyRawValid    [i] = historyRawValid    [i-1];
  }

  // Add most recent data
  if (glucoseDataSource == DATA_SOURCE_MYSENTRY) {
    if (mySentryFlag == 1) {
      historySgv      [0] = sgv;
      historySgvValid [0] = 1;
      historySgv      [1] = lastSgv;
      historySgvValid [1] = (lastSgv > 60) ? 1 : 0;
    } else {
      historySgv      [0] = 0;
      historySgvValid [0] = 0;
    }    
  }
  
  if (minilinkFlag > 0) {
    isig = getISIGfromRAW (raw, adjValue);
    rawSgv = (unsigned long) (isig * calFactor);
    
    if (glucoseDataSource != DATA_SOURCE_MYSENTRY) {
      historySgv         [0] = rawSgv;
      historySgvValid    [0] = (calFactor > 0) ? (warmUp == 0) ? 1 : 0 : 0;
      for (i=0; i<8; i++) {
        if (calFactor > 0) {
          historySgv      [i+1] = (unsigned long) (getISIGfromRAW(lastraw[i], adjValue)*calFactor);
          historySgvValid [i+1] = (warmUp == 0) ? 1 : 0 ;
        } else {
          historySgv      [i+1] = 0 ;
          historySgvValid [i+1] = 0 ;
        }  
      }
    } 
    
    historyRawSgv      [0] = rawSgv;
    historyRawSgvValid [0] = (calFactor > 0) ? (warmUp == 0) ? 1 : 0 : 0;
    historyRaw         [0] = raw;
    historyRawValid    [0] = (warmUp == 0) ? 1 : 0;
    for (i=0; i<8; i++) {
      if (calFactor > 0) {
        historyRawSgv      [i+1] = (unsigned long) (getISIGfromRAW(lastraw[i], adjValue)*calFactor);
        historyRawSgvValid [i+1] = (warmUp == 0) ? 1 : 0 ;
      } else {
        historyRawSgv      [i+1] = 0 ;
        historyRawSgvValid [i+1] = 0 ;
      }  
      historyRaw         [i+1] = lastraw [i];
      historyRawValid    [i+1] = (warmUp == 0) ? 1 : 0;
    }
    
    // Recover past data and adjust recalibration
    if (glucoseDataSource == DATA_SOURCE_MYSENTRY) {
      if ((historySgvValid [0] == 1) && (historyRawSgvValid[0] == 1)) {
        internalCal = (float)(historySgv[0]) / (float)(historyRawSgv[0]);
        if (calFactor != lastCalFactor) {
          for (i=1; i<8; i++) {
            if ((historyRawSgvValid[i] == 1) && (historySgvValid[i] == 0)) {
              historySgv[i] = (int)((float)(historyRawSgv[i]) * internalCal);
              historySgvValid[i] = 1;
            }
          }
        }
      }
    }
    
  } else {
    historyRawSgv      [0] = 0;
    historyRaw         [0] = 0;
    historyRawSgvValid [0] = 0;
    historyRawValid    [0] = 0;
  }
  
  // Sanity check
  for (i=0; i<16; i++) {
    if (historySgvValid [ i ] == 1) {
      if ( historySgv [ i ] <= 20 ) {
        historySgv      [ i ] = 0;
        historySgvValid [ i ] = 0;
      }
    }
    if (historyRawSgvValid [ i ] == 1) {
      if ( historyRawSgv [ i ] <= 20 ) {
        historyRawSgv      [ i ] = 0;
        historyRawSgvValid [ i ] = 0;
      }
    }
  }
  
  // Update last calibration factor
  lastCalFactor = calFactor;
}

void setIOBvectors (float rate)
{  
  int i, intRate;
  
  intRate = (int)(rate*40.0); //(1.0/12.0)*480.0=40.0
  
  for (i=0;i<=(MAX_NUM_IOB_REGISTERS-1); i++) {
    basalIOBvector [i] = intRate;
  }
  
  for (i=0;i<=(MAX_NUM_FUTURE_IOBS-1); i++) {
    basalIOBfuture [i] = intRate;
  }
  
}

void updateIOBvectors (void)
{  
  static float lastPumpIOB;
  static char initFlag;
  int i;
  //int maxLocalAbsorptionRate;
    
  if (initFlag == 0) {
    initFlag = 1;
    lastPumpIOB = 255.0;
  }
  
  for (i=(MAX_NUM_IOB_REGISTERS-1); i>0; i--) {
    bolusIOBvector [i] = bolusIOBvector [i-1];
    basalIOBvector [i] = basalIOBvector [i-1];
  }
  bolusIOBvector[0] = 0;
  basalIOBvector[0] = 0;
  
  if (mySentryFlag == 1) {
    if (pumpIOB > lastPumpIOB) {
      if (timeCounterBolusSnooze == 0) {
        timeCounterBolusSnooze = (calculateBolusSnoozeTime()*60 + 0)*TICKS_PER_SECOND;
        if (timeCounterBolusSnooze > (90*60 + 0)*TICKS_PER_SECOND) {
          timeCounterBolusSnooze = (90*60 + 0)*TICKS_PER_SECOND;
        }
      }
      iobAccumBolus += (int)((pumpIOB - lastPumpIOB)*480.0); 
    }
    lastPumpIOB = pumpIOB;
  }
  
  iobAccumBasal += basalIOBfuture[MAX_NUM_FUTURE_IOBS-1];
  for (i=(MAX_NUM_FUTURE_IOBS-1); i>0; i--) {
    basalIOBfuture [i] = basalIOBfuture [i-1];
  }

  if (measuredBasalValid == 1) {
    basalIOBfuture[0] = (int)(basalUsed * 40.0);
  } else {
    basalIOBfuture[0] = (int)(getCurrentBasal() * 40.0);
  }
  
  if (maxAbsorptionRateEnable == 1) {
    /*if ( (iobAccumBasal + iobAccumBolus) <= maxAbsorptionRate) {
      maxLocalAbsorptionRate = maxAbsorptionRate;
    } else {
      
    }*/
    if (iobAccumBasal >= maxAbsorptionRate) {
      basalIOBvector[0] = maxAbsorptionRate;
    } else {
      basalIOBvector[0] = iobAccumBasal;
    }
    
    if (basalIOBvector[0] + iobAccumBolus >= maxAbsorptionRate) {
      bolusIOBvector[0] = maxAbsorptionRate - basalIOBvector[0];
    } else {
      bolusIOBvector[0] = iobAccumBolus;
    }
    
  } else {
    bolusIOBvector[0] = iobAccumBolus;
    basalIOBvector[0] = iobAccumBasal;
  }
  
  iobAccumBolus -= bolusIOBvector[0];
  iobAccumBasal -= basalIOBvector[0];
}

float getDeliveredIOBBolus (void)
{
  int i;
  float responseAccum, response;
  float iobAccum, tempIOB;
  float bolusIOBInst;
  
  responseAccum = 0;
  iobAccum      = 0.0;
  for (i=MAX_NUM_IOB_REGISTERS-1; i>=0; i--) {
    response       = (float)(responseToInsulin[i])/16384.0;
    responseAccum += response;
    bolusIOBInst   = (float)(bolusIOBvector[i])/480.0;
    tempIOB        = responseAccum * bolusIOBInst;
    iobAccum      += tempIOB;
  }
  iobAccum      += (float)iobAccumBolus/480.0;
  
  return(iobAccum);
}

float getDeliveredIOBBasal (float basal)
{
  int i;
  float responseAccum, response, basalIOBInst;
  float iobAccum, tempIOB;
  
  responseAccum = 0;
  iobAccum      = 0.0;
  for (i=MAX_NUM_IOB_REGISTERS-1; i>=0; i--) {
    response       = (float)(responseToInsulin[i])/16384.0;
    responseAccum += response;
    basalIOBInst   = (float)(basalIOBvector[i])/480.0;
    tempIOB        = responseAccum * (basalIOBInst - (basal/12.0));
    iobAccum      += tempIOB;
  }
  iobAccum      += (float)iobAccumBasal/480.0;
  
  return(iobAccum);
}
 
float getDeliveredIOB (float basal)
{
  float iob;
  
  iob  = getDeliveredIOBBasal(basal);
  iob += getDeliveredIOBBolus();
  
  return(iob);
}

float getInstantIOBBolus30min (void)
{
  int i,j;
  float response;
  float iobAccum, tempIOB;
  float bolusIOBInst;

  iobAccum      = 0.0;
  for (j=0; j<6; j++) {
    for (i=MAX_NUM_IOB_REGISTERS-1; i>=0; i--) {
      if (j > i) {
        response     = 0.0;
      } else {
        response       = ((float)(responseToInsulin[i-j]))/16384.0;
      }
      bolusIOBInst   = ((float)(bolusIOBvector[i]))/480.0;
      tempIOB        = bolusIOBInst;
      tempIOB       *= response;
      iobAccum      += tempIOB;
    }
  }
  
  return(iobAccum);
}

float getInstantIOB (void)
{
  int i;
  float response;
  float iobAccum, tempIOB;
  float bolusIOBInst,basalIOBInst;

  iobAccum      = 0.0;
  for (i=MAX_NUM_IOB_REGISTERS-1; i>=0; i--) {
    response       = ((float)(responseToInsulin[i]))/16384.0;
    bolusIOBInst   = ((float)(bolusIOBvector[i]))/480.0;
    basalIOBInst   = ((float)(basalIOBvector[i]))/480.0;
    tempIOB        = bolusIOBInst + basalIOBInst;
    tempIOB       *= response;
    iobAccum      += tempIOB;
  }
  
  return(iobAccum);
}

float getToBeDeliveredIOB (float basal)
{
  int i;
  float iobAccum, tempIOB;
  
  iobAccum = 0.0;
  for (i=0; i<MAX_NUM_FUTURE_IOBS; i++) {
    tempIOB   = ((float)(basalIOBfuture[i]))/480.0;
    tempIOB  -= (basal/12.0);
    iobAccum += tempIOB;
  }
  
  return(iobAccum);
}

float getTotalBasalIOB (float basal)
{
  float iob;
  
  iob  = getDeliveredIOBBasal(basal);
  iob += getToBeDeliveredIOB(basal);
  
  return(iob);
}

float getTotalIOB (float basal)
{
  float iob;
  
  iob  = getDeliveredIOB(basal);
  iob += getToBeDeliveredIOB(basal);
  
  return(iob);
}

void addIOBForTempBasal(float rate, unsigned int time)
{
  float insulinPerStep;
  int basalValue;
  char i;

  // Avoid negative rates
  if (rate < 0.0) rate = 0.0;
  
  // There was no room for more than 30 min temp basals....
  insulinPerStep = rate / 12.0;

  // Basal IOB value calculation
  basalValue = (int)(insulinPerStep * 480.0);
  
  for (i=0; i<6; i++) {
    basalIOBfuture [i] = basalValue;
  }
}

void cancelIOBForCurrentTempBasal(float basal)
{
  addIOBForTempBasal(basal, 30);
}

void respondToDeviceSearch (char *message, unsigned int length, char dataErr)
{
  char uartTxBuffer [15] ;
  
  // First check if the message has a correct CRC
  if (dataErr == 0) { 
    // MySentry Message
    if ( (syncMode == 1) && 
         (message[0] == 0xA2) &&
         (message[1] == pumpID[0]) &&
         (message[2] == pumpID[1]) &&
         (message[3] == pumpID[2]) ) {
           switch(message[4]) {
              
           case 0x09:
           case 0x0A:
           case 0x08:
           case 0x04:
           case 0x0B:
             uartTxBuffer[0]  = 0xA2;
             uartTxBuffer[1]  = message[1];
             uartTxBuffer[2]  = message[2];
             uartTxBuffer[3]  = message[3];
             uartTxBuffer[4]  = 0x06;
             uartTxBuffer[5]  = message[5] & 0x7F;
             uartTxBuffer[6]  = message[1];
             uartTxBuffer[7]  = message[2];
             uartTxBuffer[8]  = message[3];
             uartTxBuffer[9]  = 0x00;
             uartTxBuffer[10] = message[4];
             switch (message[4]){
               case 0x09: uartTxBuffer[11] = 0x0A; break; 
               case 0x0A: uartTxBuffer[11] = 0x08; break;
               case 0x08: uartTxBuffer[11] = 0x0B; break;
               case 0x0B: uartTxBuffer[11] = 0x04; break;
               default:   uartTxBuffer[11] = 0x00; syncMode = 0; break;
             }
            uartTxBuffer[12] = 0x00;
            uartTxBuffer[13] = 0x00;
            uartTxBuffer[14] = crc8(uartTxBuffer,14);
            addMsgToQueue(uartTxBuffer, 15, 1, 0x00, 0, 0); 
              
           default:
             break;
           }
      
    }
  }
}

float getBasalForTime (unsigned char hour, unsigned char minute)
{
  unsigned int timeToCheck, basalTime;
  char i;
  float lastValidBasal;
  
  timeToCheck = hour*60 + minute;
  lastValidBasal = 0.0;
  
  if (timeToCheck >= 24*60) return (0.0);
  if (pumpBasalsValidRanges == 0) {
    return (0.0);
  }
  
  for (i=0; i<pumpBasalsValidRanges; i++) {
    basalTime = pumpBasalsHour[i]*60 + pumpBasalsMinute[i];
    if (basalTime <= timeToCheck) {
      lastValidBasal = (float)(pumpBasalsAmount[i]) * 0.025;
    }
  }
  
  return (lastValidBasal);
}

float getCurrentBasal (void) 
{
  return(getBasalForTime(systemTimeHour,systemTimeMinute));
}

int getInsulinSensitivityForTime (unsigned char hour, unsigned char minute)
{
  unsigned int timeToCheck, rangeTime;
  char i;
  int lastSensitivity;
  
  timeToCheck = hour*60 + minute;
  lastSensitivity = 0;
  
  if (insulinSensValidRanges == 0) {
    return (0);
  }

  if (timeToCheck >= 24*60) return (0);
  
  for (i=0; i<insulinSensValidRanges; i++) {
    rangeTime = insulinSensHour[i]*60 + insulinSensMinute[i];
    if (rangeTime <= timeToCheck) lastSensitivity = (int)(insulinSensAmount[i]);
  }
  
  return (lastSensitivity);
}

int getCurrentInsulinSensitivity (void) 
{
  return(getInsulinSensitivityForTime(systemTimeHour,systemTimeMinute));
}

char getTargetsForTime (unsigned char hour, unsigned char minute,
                        int *min, int *max)
{
  unsigned int timeToCheck, targetsTime;
  char i;
  
  timeToCheck = hour*60 + minute;
  *min = 0;
  *max = 0;
  
  if (timeToCheck >= 24*60) return (0);
  if (pumpTargetsValidRanges == 0) {
    return (0);
  }
  
  for (i=0; i<pumpTargetsValidRanges; i++) {
    targetsTime = pumpTargetsHour[i]*60 + pumpTargetsMinute[i];
    if (targetsTime <= timeToCheck) {
      *min = pumpTargetsMin[i];
      *max = pumpTargetsMax[i];
    }
  }
  
  return (1);
}

char getCurrentTargets (int *min, int *max) 
{
  return(getTargetsForTime(systemTimeHour,systemTimeMinute,min,max));
}

unsigned int calculateBolusSnoozeTime (void)
{
  char i;
  int max;
  char maxIdx;
  
  max = 0;
  maxIdx = MAX_NUM_IOB_REGISTERS;
  for (i=0; i<MAX_NUM_IOB_REGISTERS; i++) {
    if (responseToInsulin[i] >= max) {
      max = responseToInsulin[i];
      maxIdx = i;
    }
  }
  
  if (maxIdx == MAX_NUM_IOB_REGISTERS) {
    return(60); // Return 1 hour of bolus snooze by default
  } else {
    return((unsigned int)(5*maxIdx));
  }
}
