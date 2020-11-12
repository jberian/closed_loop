#include "ioCCxx10_bitdef.h"
#include "ioCC1110.h"
#include "bleComms.h"
#include "constants.h"
#include "globals.h"
#include "interrupts.h"
#include "timingController.h"
#include "freqManagement.h"
#include "init.h"
#include "medtronicRF.h"
#include "pumpCommands.h"
#include "dataProcessing.h"

/*char initHM10 (void)
{
  unsigned int idx,i,j;
  unsigned int timeout;
  char tempMsg[50];
  
  idx = 0;
  tempMsg[idx++] = 'A';
  tempMsg[idx++] = 'T';
  tempMsg[idx++] = '+';
  tempMsg[idx++] = 'N';
  tempMsg[idx++] = 'A';
  tempMsg[idx++] = 'M';
  tempMsg[idx++] = 'E';
  tempMsg[idx++] = 'm';
  tempMsg[idx++] = 'D';
  tempMsg[idx++] = 'r';
  tempMsg[idx++] = 'i';
  tempMsg[idx++] = 'p';
  tempMsg[idx++] = 0x20;
  tempMsg[idx++] = 0x0A;
  tempMsg[idx++] = 0x0D;
  
  //bleRxIndex = 0;  
  //timeout = 3;
  //while ((bleRxIndex == 0) &&
  //        (timeout != 0)) {
  //  bleTxLength = 4;
  //  tempMsg[2] = 0x0A;
  //  tempMsg[3] = 0x0D;
  //  uart0StartTxForIsr();
  //  for (j=0; j<5; j++) for (i=0; i<32567; i++);
  //  timeout--;
  //}
  //if (timeout == 0) return (0);
  
  timeout = 3;
  while ((bleRxIndex == 0) &&
         (timeout != 0)) {
    bleTxLength = idx;
    //tempMsg[2] = '+';
    //tempMsg[3] = 'N';
    if (tempMsg[0]!=0) for (i=0; i<=tempMsg[0]; i++) addCharToBleTxBuffer ( tempMsg[i] );
    uart0StartTxForIsr();
    for (j=0; j<5; j++) for (i=0; i<32567; i++);
    timeout--;
  }
  if (timeout == 0) return (0);
  
  return(1);
}
*/
void receiveBLEMessage (char *message, unsigned int messageLen)
{
  char tempMsg[50];
  unsigned char i,j,k,m,n;
  unsigned int glucometerChecksum;
  float tempFloat;
//  unsigned long tempLong;
  char *tempPointer;
  
  // Wait while BLE TX is busy
  //while (bleTxing == 1);
  
  tempMsg[0] = 0x00;

  switch(message[0]) {
  case 0x00:
    i=0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = 'm';
    tempMsg[i++] = 'D';
    tempMsg[i++] = 'r';
    tempMsg[i++] = 'i';
    tempMsg[i++] = 'p';
    tempMsg[i++] = ' ';
    tempMsg[i++] = 'v';
    tempMsg[i++] = '1';
    tempMsg[i++] = '.';
    tempMsg[i++] = '4';
    tempMsg[i++] = '2';
      
    tempMsg[0] = i-1;
    break;
  
  case 0x10:
    i = 0;
    if (messageLen == 2) {
      m=4; n=message[1];
    } else {
      m=8; n=0;
    }
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    for (j=n;j<(m+n);j++) {
      for (k=0;k<2;k++) {
          tempMsg[i++] = (timingTable[j][k] >> 8) & 0x0FF;
          tempMsg[i++] = (timingTable[j][k]     ) & 0x0FF;
      }
    }
    tempMsg[0] =i-1;
    break;
    
  /*case 0x20:
    if (messageLen == 33) {
      i = 1;
      for (j=0;j<8;j++) {
        for (k=0;k<2;k++) {
            timingTable[j][k] = (message[i] << 8) | (message[i+1]) ;
            i += 2;
        }
      }      
    }
    timingTableFrozen  = 0;
    timingTableCorrect = 0;
    if (timingCorrect()) calculateFiveMinAdjustment();
    timeCounter = _MAX_TIMECOUNTER_VAL_;
    mySentryFlag = 0;
    minilinkFlag = 0;
    sendFlag = 0 ;
    queryPumpFlag = 0;
    break;
    */
  /*case 0x11:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = (timeCounter    >> 8) & 0x0FF;
    tempMsg[i++] = (timeCounter        ) & 0x0FF;
    tempMsg[i++] = (timeCounterOn  >> 8) & 0x0FF;
    tempMsg[i++] = (timeCounterOn      ) & 0x0FF;
    tempMsg[i++] = (timeCounterOff >> 8) & 0x0FF;
    tempMsg[i++] = (timeCounterOff     ) & 0x0FF;
    tempMsg[i++] = (rfOnTimer   >> 8) & 0x0FF;
    tempMsg[i++] = (rfOnTimer       ) & 0x0FF;
    tempMsg[i++] = (reCalTimer  >> 8) & 0x0FF;
    tempMsg[i++] = (reCalTimer      ) & 0x0FF;
    temp = (int)(calFactor * 256);
    tempMsg[i++] = (temp >> 8) & 0x0FF;
    tempMsg[i++] = (temp     ) & 0x0FF;
    tempMsg[i++] = lastMinilinkSeqNum;
    tempMsg[i++] = (timingTableFrozen  << 5) |
                       (timingTableCorrect << 4) |
                       (mySentryFlag << 3) |
                       ((minilinkFlag & 0x03) << 1) |
                       (sendFlag) ;
    tempMsg[i++] = channel;
    tempMsg[i++] = rfMode;
    tempMsg[i++] = FREQ2;
    tempMsg[i++] = FREQ1;
    tempMsg[i++] = FREQ0;
    tempMsg[i++] = MDMCFG4 & 0xF0;
    tempMsg[i++] = adjValue;
    tempMsg[0] = i-1;
    break;*/
    
  case 0x12:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = rfFrequencyMode;
    tempMsg[i++] = glucoseDataSource;
    tempMsg[i++] = minilinkID[0];
    tempMsg[i++] = minilinkID[1];
    tempMsg[i++] = minilinkID[2];
    tempMsg[i++] = pumpID[0];
    tempMsg[i++] = pumpID[1];
    tempMsg[i++] = pumpID[2];
    tempMsg[i++] = glucometerID[0];
    tempMsg[i++] = glucometerID[1];
    tempMsg[i++] = glucometerID[2];
    tempMsg[0] = i-1;
    break;
  case 0x22:
    if (messageLen == 12) {
      i = 1;
      if (message[1] != rfFrequencyMode) {
        rfFrequencyMode = message[i++];
        initFreqs();
        configureMedtronicRFMode(rfMode);
      } else {
        rfFrequencyMode = message[i++];
      }    
      glucoseDataSource = message[i++];
      minilinkID[0]   = message[i++];
      minilinkID[1]   = message[i++];
      minilinkID[2]   = message[i++];
      pumpID[0]       = message[i++];
      pumpID[1]       = message[i++];
      pumpID[2]       = message[i++];
      glucometerID[0] = message[i++];
      glucometerID[1] = message[i++];
      glucometerID[2] = message[i++];
      glucometerChecksum = 0;
      for (i=0; i<3; i++) glucometerChecksum += glucometerID[i];
      if (glucometerChecksum > 0) glucometerEnable = 1;
      else glucometerEnable = 0;
    }
    break;
 
  case 0x13:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = lastMinilinkSeqNum;
    for (j=0;j<16;j++) {
      tempMsg[i++] = ((historySgv[j] >> 8) & 0x01) |
                         ((historySgvValid[j] & 0x01) << 7) ;
      tempMsg[i++] = (historySgv[j] & 0x0FF);
    }
    tempMsg[0] = i-1;
    break;
      
  case 0x14:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = lastMinilinkSeqNum;
    for (j=0;j<16;j++) {
      tempMsg[i++] = ((historyRawSgv[j] >> 8) & 0x001) |
                         ((historyRawSgvValid[j] & 0x01) << 7) ;
      tempMsg[i++] = (historyRawSgv[j] & 0x0FF);
    }
    tempMsg[0] = i-1;
    break;
      
  case 0x15:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = lastMinilinkSeqNum;
    for (j=0;j<16;j++) {
      tempMsg[i++] = ((historyRaw[j] >> 8) & 0x07F) |
                         ((historyRawValid[j] & 0x01) << 7) ;
      tempMsg[i++] = (historyRaw[j] & 0x0FF);
    }
    tempMsg[0] = i-1;
    break;
    
  /*case 0x16:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = timingTableCorrect;
    for (j=0;j<8;j++) {
      tempMsg[i++] = (fiveMinAdjTable[j] >> 8) & 0x0FF;
      tempMsg[i++] = (fiveMinAdjTable[j]     ) & 0x0FF;
    }
    tempMsg[0] = i-1;
    break;*/
      
  case 0x27:
    if (messageLen == 3) {
      syncMode = 1;
      rfOnTimer = ((message[1] & 0x0FF)*60 + (message[2] & 0x0FF))*TICKS_PER_SECOND;
    }
    break;
      
  case 0x18:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0]; 
    tempMsg[i++] = bestMinilinkChannel[0]   & 0x3F;
    tempMsg[i++] = bestMinilinkChannel[1];
    tempMsg[i++] = bestPumpChannel[0]       & 0x3F;
    tempMsg[i++] = bestPumpChannel[1];
    tempMsg[i++] = bestGlucometerChannel[0] & 0x3F;
    tempMsg[i++] = bestGlucometerChannel[1];
    tempMsg[i++] = lastMinilinkChannel      & 0x3F;
    tempMsg[i++] = lastPumpChannel          & 0x3F;
    tempMsg[i++] = lastGlucometerChannel    & 0x3F;
    
    tempMsg[0] = i-1;
    break;

 /* case 0x28:
    if (messageLen == 17) {
      i = 1;
      genericFreq[2]            = message[i++];
      genericFreq[1]            = message[i++];
      genericFreq[0]            = message[i++];
      genericBW                 = message[i++];
      minilinkBW                = message[i++];
      pumpBW                    = message[i++];
      glucometerBW              = message[i++];
      bestMinilinkChannel[0]    = message[i++] | 0x40;
      bestMinilinkChannel[1]    = message[i++];
      bestPumpChannel[0]        = message[i++] | 0x80;
      bestPumpChannel[1]        = message[i++];
      bestGlucometerChannel[0]  = message[i++] | 0xC0;
      bestGlucometerChannel[1]  = message[i++];
      lastMinilinkChannel       = message[i++];
      lastPumpChannel           = message[i++];
      lastGlucometerChannel     = message[i++];
    }
    break; */
      
  case 0x19:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0]; 
    for (j=0; j<33; j++) {
      tempMsg[i++] =  minilinkRSSI[j];
    }
    tempMsg[i++] = bestMinilinkFreqFound;
    tempMsg[0] = i-1;
    break;
    
  case 0x29:
    if (messageLen == 3) {
      for (j=0; j<32; j++) minilinkRSSI[j] = 0x00;
      minilinkRSSI[32] = 0x01;
      if (message[1] < 32) {
        minilinkRSSI[message[1]] = message[2];
        bestMinilinkFreqFound = 0x01;
      } else {
        bestMinilinkFreqFound = 0x00;
      }
   /* } else if (messageLen == 35) {
      i = 1;
      for (j=0; j<33; j++) {
        minilinkRSSI[j] = message[i++];
      }
      bestMinilinkFreqFound = message[i++];*/
    }
    break;
    
  case 0x1A:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0]; 
    for (j=0; j<33; j++) {
      tempMsg[i++] =  pumpRSSI[j];
    }
    tempMsg[i++] = bestPumpFreqFound;
    tempMsg[0] = i-1;
    break;
    
  case 0x2A:
    if (messageLen == 3) {
      for (j=0; j<32; j++) pumpRSSI[j] = 0x00;
      pumpRSSI[32] = 0x01;
      if (message[1] < 32) {
        pumpRSSI[message[1]] = message[2];
        bestPumpFreqFound = 0x01;
      } else {  
        bestPumpFreqFound = 0x00;
      }
    /*} else if (messageLen == 35) {
      i = 1;
      for (j=0; j<33; j++) {
        pumpRSSI[j] = message[i++];
      }
      bestPumpFreqFound = message[i++];*/
    }
    break;

  case 0x1B:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0]; 
    for (j=0; j<33; j++) {
      tempMsg[i++] =  glucometerRSSI[j];
    }
    tempMsg[i++] = bestGlucometerFreqFound;
    tempMsg[0] = i-1;
    break;
      
  /*case 0x2B:
    if (messageLen == 3) {
      for (j=0; j<33; j++) glucometerRSSI[j] = 0x00;
      glucometerRSSI[32] = 0x01;
      if (message[1] < 32) {
        glucometerRSSI[message[1]] = message[2];
        bestGlucometerFreqFound = 0x01;
      } else {
        bestGlucometerFreqFound = 0x00;
      }
    } else if (messageLen == 35) {
      i = 1;
      for (j=0; j<33; j++) {
        glucometerRSSI[j] = message[i++];
      }
      bestGlucometerFreqFound = message[i++];
    }
    break; */
    
  case 0x2C:
    if (messageLen == 1) {
      adjustPumpFrequency ();
    }
    break; 
    
  /*case 0x1D:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    for (j=0;j<8;j++) {
      tempMsg[i++] = missesTable[j];
    }
    tempMsg[0] =i-1;
    break;
    
  case 0x2D:
    if (messageLen == 9) {
      i = 1;
      for (j=0; j<8; j++) missesTable[j] = message[i++];
    }
    break;*/
    
  case 0x1E:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = minilinkRetransmit;
    tempMsg[i++] = minilinkMinRSSI;
    tempMsg[0] = i-1;
    break;
      
  case 0x2E:
    if (messageLen == 3) {
      i = 1;
      minilinkRetransmit = message[i++];
      minilinkMinRSSI    = message[i++];
    }
    break;
      
  /*case 0x2F:
    switch (message[1]) {
    case 0x00:
      composeActionNotificationMessage (PUMP_RESET_NOTIFICATION);
      //reset();
      break;
    case 0x01:
      if (messageLen == 2) suspendPump();
      break;
    case 0x02:
      if (messageLen == 2) resumePump();
      break;
    case 0x03:
      tempFloat = (float)(message[3]) + (float)(message[4])/256.0;
      if (messageLen == 5) setTempBasal (30*message[2],tempFloat);
      break;
    case 0x04:
      if (messageLen == 2) cancelTempBasal();
      break;
    case 0x05:
      tempFloat = (float)(message[2]) + (float)(message[3])/256.0;
      if (messageLen == 4) bolus(tempFloat);
      break;
    case 0x06:
      if (messageLen == 2) readPumpConfig();
      break;
    case 0x07:
      if (messageLen == 2) readSensorCalibrationFactor ();
      break;
    case 0x08:
      resetRFBuffers ();
      break;
    case 0x09:
      initIOBRegisters();
      break;
    default:
      break; 
    } 
    break;
    */
  /*case 0x30:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    temp = (int)(calFactor * 256.0);
    tempMsg[i++] = (temp >> 8) & 0x0FF;
    tempMsg[i++] = (temp     ) & 0x0FF;
    tempMsg[0] = i-1;
    break;*/
    
  /*case 0x40:
    if (messageLen == 4) {
      i = 1;
      tempFloat = 0.0;
      if (message[i++] == 0x00) {
        tempFloat += (float)(message[i++]) * 256.0;
        tempFloat += (float)(message[i++]);
        calFactor = tempFloat;
      } else {
        if (historyRawValid[0] == 1) {
          tempLong  = (message[i++] << 8) & 0x0FF00;
          tempLong |=  message[i++] & 0x00FF;
          tempFloat = getISIGfromRAW(historyRaw[0], adjValue);
          tempFloat = (float)(tempLong) / tempFloat;
          calFactor = tempFloat;
        }
      }
    } 
    break;*/
  
  /*case 0x31:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = systemTimeHour;
    tempMsg[i++] = systemTimeMinute;
    tempMsg[i++] = systemTimeSecond;
    tempMsg[i++] = systemTimeSecondTimer;
    tempMsg[i++] = systemTimeSynced;
    tempMsg[0] = i-1;
    break;*/
  
  /*case 0x41:
    if (messageLen == 6) {
      i=1;
      systemTimeHour         = message[i++];
      systemTimeMinute       = message[i++];
      systemTimeSecond       = message[i++];
      systemTimeSecondTimer  = message[i++];
      systemTimeSynced       = message[i++];
    }
    break;*/
    
  case 0x32:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = controlLoopMode;
    tempMsg[i++] = (controlLoopThresHigh   >> 8) & 0x0FF;
    tempMsg[i++] = (controlLoopThresHigh       ) & 0x0FF;
    tempMsg[i++] = (controlLoopThresLow    >> 8) & 0x0FF;
    tempMsg[i++] = (controlLoopThresLow        ) & 0x0FF;
    tempMsg[i++] = (controlLoopMaxTarget   >> 8) & 0x0FF;
    tempMsg[i++] = (controlLoopMaxTarget       ) & 0x0FF;
    tempMsg[i++] = timeCounterBolusSnoozeEnable;
    tempMsg[i++] = durationIOB;
    tempMsg[i++] = adjustSensitivityFlag;
    tempMsg[i++] = controlLoopAggressiveness;
    //tempMsg[i++] = maxAbsorptionRateEnable;
    //tempMsg[i++] = (maxAbsorptionRate   >> 8) & 0x0FF;
    //tempMsg[i++] = (maxAbsorptionRate       ) & 0x0FF;
    tempMsg[0] = i-1;
    break;
    
  case 0x42:
    if (messageLen == 12) {
      i = 1;
      controlLoopMode               =  message[i++];
      controlLoopThresHigh          = (message[i++] << 8) & 0x0FF00;
      controlLoopThresHigh         |=  message[i++] & 0x0FF;
      controlLoopThresLow           = (message[i++] << 8) & 0x0FF00;
      controlLoopThresLow          |=  message[i++] & 0x0FF;
      controlLoopMaxTarget          = (message[i++] << 8) & 0x0FF00;
      controlLoopMaxTarget         |=  message[i++] & 0x0FF;
      timeCounterBolusSnoozeEnable  =  message[i++];
      durationIOB                   =  message[i++];
      adjustSensitivityFlag         =  message[i++];   
      controlLoopAggressiveness     =  message[i++];
      //maxAbsorptionRateEnable       =  message[i++];
      //maxAbsorptionRate             = (message[i++] << 8) & 0x0FF00;
      //maxAbsorptionRate            |=  message[i++] & 0x0FF;
    }
    break;
    
  /*case 0x33:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = pumpBasalsValidRanges;
    for (j=0; j<pumpBasalsValidRanges; j++) {
      tempMsg[i++] = pumpBasalsHour   [j];
      tempMsg[i++] = pumpBasalsMinute [j];
      tempMsg[i++] = pumpBasalsAmount [j];
    }
    tempMsg[0] = i-1;
    break;*/
    
  /*case 0x43:
    if (messageLen > 2) { 
      if (messageLen == (message[1]*3 + 2)) {
        i = 1;
        pumpBasalsValidRanges = message[i++];
        for (j=0; j<pumpBasalsValidRanges; j++) {
          pumpBasalsHour    [j] = message[i++];
          pumpBasalsMinute  [j] = message[i++];
          pumpBasalsAmount  [j] = message[i++];
        }
      }
    }
    break;*/
    
 /* case 0x34:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = insulinSensValidRanges;
    for (j=0; j<insulinSensValidRanges; j++) {
      tempMsg[i++] = insulinSensHour   [j];
      tempMsg[i++] = insulinSensMinute [j];
      tempMsg[i++] = insulinSensAmount [j];
    }
    tempMsg[0] = i-1;
    break;*/
    
  /*case 0x44:
    if (messageLen > 2) { 
      if (messageLen == (message[1]*3 + 2)) {
        i = 1;
        insulinSensValidRanges = message[i++];
        for (j=0; j<insulinSensValidRanges; j++) {
          insulinSensHour    [j] = message[i++];
          insulinSensMinute  [j] = message[i++];
          insulinSensAmount  [j] = message[i++];
        }
      }
    }
    break;*/

  case 0x35:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    
    tempMsg[i++] = (expectedSgv >> 8) & 0x0FF;
    tempMsg[i++] =  expectedSgv & 0x0FF;
    tempMsg[i++] = (expectedSgvWoIOB >> 8) & 0x0FF;
    tempMsg[i++] =  expectedSgvWoIOB & 0x0FF;
    tempMsg[i++] = controlState;
    tempPointer = (char *)(&bolusIOB);
    tempMsg[i++] = tempPointer[0];
    tempMsg[i++] = tempPointer[1];
    tempMsg[i++] = tempPointer[2];
    tempMsg[i++] = tempPointer[3];
    tempFloat = basalIOB + tbdBasalIOB;
    tempPointer = (char *)(&tempFloat);
    tempMsg[i++] = tempPointer[0];
    tempMsg[i++] = tempPointer[1];
    tempMsg[i++] = tempPointer[2];
    tempMsg[i++] = tempPointer[3];
    tempPointer = (char *)(&tbdBasalIOB);
    tempMsg[i++] = tempPointer[0];
    tempMsg[i++] = tempPointer[1];
    tempMsg[i++] = tempPointer[2];
    tempMsg[i++] = tempPointer[3];
    tempPointer = (char *)(&basalIOB);
    tempMsg[i++] = tempPointer[0];
    tempMsg[i++] = tempPointer[1];
    tempMsg[i++] = tempPointer[2];
    tempMsg[i++] = tempPointer[3];
    tempPointer = (char *)(&instantIOB);
    tempMsg[i++] = tempPointer[0];
    tempMsg[i++] = tempPointer[1];
    tempMsg[i++] = tempPointer[2];
    tempMsg[i++] = tempPointer[3];
    tempMsg[i++] = (timeCounterBolusSnooze >> 8) & 0x0FF;
    tempMsg[i++] = (timeCounterBolusSnooze     ) & 0x0FF;
    tempPointer = (char *)(&measuredBasal);
    tempMsg[i++] = tempPointer[0];
    tempMsg[i++] = tempPointer[1];
    tempMsg[i++] = tempPointer[2];
    tempMsg[i++] = tempPointer[3];
    tempPointer = (char *)(&measuredSensitivity);
    tempMsg[i++] = tempPointer[0];
    tempMsg[i++] = tempPointer[1];
    tempMsg[i++] = tempPointer[2];
    tempMsg[i++] = tempPointer[3];   
    tempPointer = (char *)(&basalUsed);
    tempMsg[i++] = tempPointer[0];
    tempMsg[i++] = tempPointer[1];
    tempMsg[i++] = tempPointer[2];
    tempMsg[i++] = tempPointer[3];  
    tempMsg[i++] = (sensitivityUsed >> 8) & 0x0FF;
    tempMsg[i++] = (sensitivityUsed     ) & 0x0FF; 
    tempPointer = (char *)(&tempBasalRate);
    tempMsg[i++] = tempPointer[0];
    tempMsg[i++] = tempPointer[1];
    tempMsg[i++] = tempPointer[2];
    tempMsg[i++] = tempPointer[3];  
    tempMsg[i++] = (targetSgv >> 8) & 0x0FF;
    tempMsg[i++] = (targetSgv     ) & 0x0FF;   
    
    tempMsg[0] = i-1;
    break;
      
  /*case 0x36:
    if (messageLen == 2) {
      i = 0;
      tempMsg[i++] = 0x00;
      tempMsg[i++] = message[0];
      k = message[1];
      if (k > MAX_NUM_IOB_REGISTERS-12) k = MAX_NUM_IOB_REGISTERS-12;
      tempMsg[i++] = k;
      for (j=k;j<k+11;j++) {
        tempMsg[i++] = (responseToInsulin[j] >> 8) & 0x0FF;
        tempMsg[i++] = (responseToInsulin[j]     ) & 0x0FF;
      }
      tempMsg[0] =i-1;
    }
    break;*/
    
  /*case 0x46:
    if (messageLen == 26) {
      i = 2;
      k = message[1];
      if (k <= MAX_NUM_IOB_REGISTERS-12) {
        for (j=k;j<k+11;j++) {
          responseToInsulin[j]  = (message[i] << 8);
          responseToInsulin[j] |=  message[i+1];
          i += 2;
        }
      }      
    }
    break;*/
    
  /*case 0x37:
    if (messageLen == 2) {
      i = 0;
      tempMsg[i++] = 0x00;
      tempMsg[i++] = message[0];
      k = message[1];
      if (k > MAX_NUM_IOB_REGISTERS-12) k = MAX_NUM_IOB_REGISTERS-12;
      tempMsg[i++] = k;
      for (j=k;j<k+11;j++) {
        tempMsg[i++] = (basalIOBvector[j] >> 8) & 0x0FF;
        tempMsg[i++] = (basalIOBvector[j]     ) & 0x0FF;
      }
      tempMsg[0] =i-1;
    }
    break;*/
    
  /*case 0x47:
    if (messageLen == 26) {
      i = 2;
      k = message[1];
      if (k <= MAX_NUM_IOB_REGISTERS-12) {
        for (j=k;j<k+11;j++) {
          basalIOBvector[j]  = (message[i] << 8);
          basalIOBvector[j] |=  message[i+1];
          i += 2;
        }
      }      
    }
    break;*/
      
  /*case 0x38:
    if (messageLen == 2) {
      i = 0;
      tempMsg[i++] = 0x00;
      tempMsg[i++] = message[0];
      k = message[1];
      if (k > MAX_NUM_IOB_REGISTERS-12) k = MAX_NUM_IOB_REGISTERS-12;
      tempMsg[i++] = k;
      for (j=k;j<k+11;j++) {
        tempMsg[i++] = (bolusIOBvector[j] >> 8) & 0x0FF;
        tempMsg[i++] = (bolusIOBvector[j]     ) & 0x0FF;
      }
      tempMsg[0] =i-1;
    }
    break;*/
    
  /*case 0x48:
    if (messageLen == 26) {
      i = 2;
      k = message[1];
      if (k <= MAX_NUM_IOB_REGISTERS-12) {
        for (j=k;j<k+11;j++) {
          bolusIOBvector[j]  = (message[i] << 8);
          bolusIOBvector[j] |=  message[i+1];
          i += 2;
        }
      }      
    }
    break;*/
        
  /*case 0x39:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    for (j=0;j<MAX_NUM_FUTURE_IOBS;j++) {
      tempMsg[i++] = (basalIOBfuture[j] >> 8) & 0x0FF;
      tempMsg[i++] = (basalIOBfuture[j]     ) & 0x0FF;
    }
    tempMsg[0] =i-1;
    break;*/
    
  /*case 0x49:
    if (messageLen == 1) {
      for (j=0;j<MAX_NUM_FUTURE_IOBS*2;j++) basalIOBfuture[j]  = 0x00;
      for (j=0;j<MAX_NUM_IOB_REGISTERS*2;j++) {
        bolusIOBvector[j]  = 0x00;
        basalIOBvector[j]  = 0x00;
      }
      for (j=0; j<6; j++) {
        instantIOBHist[j] = 0.0;
        sgvDeriv[j] = 0.0;
        sensHist[j] = 0.0;
        basalHist[j] = 0.0;
      }
      numHist = 0;
      numRes = 0;
    }
    break;*/
    
  /*case 0x49:
    if (messageLen == 13) {
      i = 1;
      for (j=0;j<MAX_NUM_FUTURE_IOBS;j++) {
        basalIOBfuture[j]  = (message[i] << 8);
        basalIOBfuture[j] |=  message[i+1];
        i += 2;
      }      
    }
    break;  */
    
  /*case 0x50:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = numHist;
    for (j=0;j<6;j++) {
      tempMsg[i++] = (sgvDeriv[j] >> 8) & 0x0FF;
      tempMsg[i++] = (sgvDeriv[j]     ) & 0x0FF;
    }
    tempMsg[0] =i-1;
    break;
    
  case 0x51:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = numHist;
    for (j=0;j<6;j++) {
      temp = (int)(instantIOBHist[j] * 256.0);
      tempMsg[i++] = (temp >> 8) & 0x0FF;
      tempMsg[i++] = (temp     ) & 0x0FF;
    }
    tempMsg[0] =i-1;
    break;
    
  case 0x52:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = numRes;
    for (j=0;j<6;j++) {
      tempMsg[i++] = (sensHist[j] >> 8) & 0x0FF;
      tempMsg[i++] = (sensHist[j]     ) & 0x0FF;
    }
    tempMsg[0] =i-1;
    break;
    
  case 0x53:
    i = 0;
    tempMsg[i++] = 0x00;
    tempMsg[i++] = message[0];
    tempMsg[i++] = numRes;
    for (j=0;j<6;j++) {
      tempMsg[i++] = (basalHist[j] >> 8) & 0x0FF;
      tempMsg[i++] = (basalHist[j]     ) & 0x0FF;
    }
    tempMsg[0] =i-1;
    break; */
    
 /* case 0x54:
    if (messageLen == 4) {
      i = 0;
      tempMsg[i++] = 0x00;
      tempMsg[i++] = message[0];
      tempPointer = (char *)(((message[2]<<8) & 0x0FF00) |
                             ( message[3]     & 0x000FF));
      for (j=0; j<(message[1]); j++) {
        tempMsg[i++] = (char)(*tempPointer);
        tempPointer++;
      }
      tempMsg[0] =i-1;
    }
    break;*/
    
  /*case 0x55:
    if (messageLen == 2) {
      breakPointId = message[2];
    }
    break;*/
  
  default:
    break;
  }
  
  if (tempMsg[0]!=0) for (i=0; i<=tempMsg[0]; i++) addCharToBleTxBuffer ( tempMsg[i] );
}

void composeInfoUpdateMessage (void)
{
  char tempMsg[30];
  unsigned int i;
  unsigned int timeOffset;
  int tempInt;
  
  // Wait while BLE TX is busy
  //while (bleTxing == 1);

  i=0;
  
  timeOffset = 0;
  if (fiveMinAdjTable[(lastMinilinkSeqNum & 0x0F0)>>4] > 0) {
    timeOffset = timeCounter + fiveMinAdjTable[(lastMinilinkSeqNum & 0x0F0)>>4];
  } 
  
  if ((mySentryFlag == 1) || (minilinkFlag > 0)) {

    tempMsg[i++] = 0x00; // Message length
    tempMsg[i++] = 0x01; // Message type -> 0x01 = Info update
    tempMsg[i]   = (minilinkFlag << 0x02);
    if (glucoseDataSource == DATA_SOURCE_MYSENTRY) {
      tempMsg[i]   |= (mySentryFlag<<1);
    } else {
      tempMsg[i]   |= (minilinkFlag > 0) ? 0x02 : 0x00;
    }
    tempMsg[i++] |= (minilinkFlag == 0 ? 0 : 1); // Flags
    tempMsg[i++] = (timeOffset >> 8) & 0x00FF; // Time offset to be applied
    tempMsg[i++] = (timeOffset     ) & 0x00FF; // Time offset to be applied
    if (glucoseDataSource == DATA_SOURCE_MYSENTRY) {
      if (mySentryFlag == 1) {
        tempMsg[i++] = (historySgv [0] >> 8) & 0x00FF;
        tempMsg[i++] =  historySgv [0]       & 0x00FF;
      } else {
        tempMsg[i++] = 0x00;
        tempMsg[i++] = 0x00;
      }
    } else {
      if (minilinkFlag > 0) {
        tempMsg[i++] = (historyRawSgv [0] >> 8) & 0x00FF;
        tempMsg[i++] =  historyRawSgv [0]       & 0x00FF;
      } else {
        tempMsg[i++] = 0x00;
        tempMsg[i++] = 0x00;
      }
    }
    
    if (minilinkFlag > 0) {
      tempMsg[i++] = (historyRawSgv [0] >> 8) & 0x00FF;
      tempMsg[i++] =  historyRawSgv [0]       & 0x00FF;
      tempMsg[i++] = (historyRaw    [0] >> 8) & 0x00FF;
      tempMsg[i++] =  historyRaw    [0]       & 0x00FF;
    } else {
      tempMsg[i++] = 0x00;
      tempMsg[i++] = 0x00;
      tempMsg[i++] = 0x00;
      tempMsg[i++] = 0x00;
    }
  
    tempMsg[i++] = 0xFF;
    
    tempInt = (int)(pumpIOB / 0.025);
    tempMsg[i++]  = (tempInt   >> 8) & 0x00FF;
    tempMsg[i++]  = (tempInt       ) & 0x00FF;
    tempInt = (int)(pumpBattery * 1000.0);
    tempMsg[i++]  = (tempInt   >> 8) & 0x00FF;
    tempMsg[i++]  = (tempInt       ) & 0x00FF;
    tempMsg[i++]  = sensorAge        & 0x00FF;    
    tempInt = (int)(pumpReservoir * 10.0);
    tempMsg[i++]  = (tempInt   >> 8) & 0x00FF;
    tempMsg[i++]  = (tempInt       ) & 0x00FF;
          
    tempMsg[0] = i-1;
        
    if (tempMsg[0]!=0) for (i=0; i<=tempMsg[0]; i++) addCharToBleTxBuffer ( tempMsg[i] );
  }
}

void composeGlucometerMessage (void)
{
  char tempMsg[6];
  unsigned int i;

  // Wait while BLE TX is busy
  //while (bleTxing == 1);
  
  i=0;
   
  tempMsg[i++] = 0x00; // Message length
  tempMsg[i++] = 0x02; // Message type -> 0x02 = Glucometer reading
  
  tempMsg[i++] = (bgReading >> 8) & 0x00FF;
  tempMsg[i++] =  bgReading       & 0x00FF;
  
  tempMsg[0] = i-1;
  
  if (tempMsg[0]!=0) for (i=0; i<=tempMsg[0]; i++) addCharToBleTxBuffer ( tempMsg[i] );
}

void composeActionNotificationMessage (char action)
{
  unsigned int i;
  unsigned int tempVar;
  char tempMsg[30];
  
  // Wait while BLE TX is busy
  //while (bleTxing == 1);

  i=0;
   
  tempMsg[i++] = 0x00; // Message length
  tempMsg[i++] = 0x03; // Message type -> 0x03 = Action
  
  tempMsg[i++] = action;
  
  switch(action){
  case PUMP_TIMEOUT:
    tempMsg[i++] = pumpCommandsFlags;
    tempMsg[i++] = lastPumpCommandSent;
    tempMsg[i++] = lastPumpCommandLengthSent;
    break;
  case PUMP_SUSPEND_CMD_FLAG:
  case PUMP_RESUME_CMD_FLAG:
  case PUMP_CANCEL_TEMP_BASAL_CMD_FLAG:
    break;
  case PUMP_SET_TEMP_BASAL_CMD_FLAG:
    tempVar = (unsigned int)(tempBasalRate / 0.025);
    tempMsg[i++] = (tempVar >> 8) & 0x0FF;
    tempMsg[i++] =  tempVar       & 0x0FF;
    tempVar = tempBasalMinutes / 30;
    tempMsg[i++] =  tempVar       & 0x0FF;
    break;
    
  case PUMP_SET_BOLUS_CMD_FLAG:
    tempVar = (unsigned int)(bolusInsulinAmount / 0.025);
    tempMsg[i++] = (tempVar >> 8) & 0x0FF;
    tempMsg[i++] =  tempVar       & 0x0FF;
    break;
    
  case PUMP_RESET_NOTIFICATION:
    resetRequested = 1;
    break;
    
  default:
    break;
  }
  
  tempMsg[0] = i-1;
  
  if (tempMsg[0]!=0) for (i=0; i<=tempMsg[0]; i++) addCharToBleTxBuffer ( tempMsg[i] );
}

void sendLoopReport (void)
{
  char infoUpdate[2];
  
  infoUpdate[0] = 0x35;
  infoUpdate[1] = 0x35;
 
  receiveBLEMessage (infoUpdate, 1);  
}