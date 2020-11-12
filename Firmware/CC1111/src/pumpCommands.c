#include "constants.h"
#include "globals.h"
#include "crc_4b6b.h"
#include "medtronicRF.h"
#include "init.h"
#include "pumpCommands.h"
#include "freqManagement.h"
#include "bleComms.h"
#include "dataProcessing.h"

#define QUEUE_SLOTS           2
#define QUEUE_MAX_MSG_SIZE   80
#define DEFAULT_MSG_TIMEOUT  ((0*60 + 8)*TICKS_PER_SECOND)
#define DEFAULT_MSG_RETRIES   3

unsigned char __xdata pumpCmdQueuePendingNumber                                    ;
unsigned char __xdata pumpCmdQueuePending   [ QUEUE_SLOTS ]                        ;
         char __xdata pumpCmdQueueMsg       [ QUEUE_SLOTS ] [ QUEUE_MAX_MSG_SIZE ] ;
         char __xdata pumpCmdQueueExpectAns [ QUEUE_SLOTS ]                        ;
unsigned int  __xdata pumpCmdQueueLen       [ QUEUE_SLOTS ]                        ;
         int  __xdata pumpCmdQueueTimes     [ QUEUE_SLOTS ]                        ;
unsigned long __xdata pumpCmdQueueTimeout   [ QUEUE_SLOTS ]                        ;
unsigned char __xdata pumpCmdQueueRetries   [ QUEUE_SLOTS ]                        ;

unsigned char numberOfPendingMsgs (void)
{
  char i;
  char pending = 0;
  
  for (i=0; i<QUEUE_SLOTS; i++) if (pumpCmdQueuePending[i] == 1) pending++;
  
  return(pending);
}

char addMsgToQueue (char *msg, unsigned int length, int times, char expectedAnswer, 
                    unsigned long timeout, unsigned char retries)
{
  int i;
  char slot;
  
  // Look for empty slot
  slot = QUEUE_SLOTS;
  for (i=QUEUE_SLOTS-1; i>=0; i--) 
    if (pumpCmdQueuePending[i] == 0) slot = i;
  
  // Check if the message fits
  if (length > QUEUE_MAX_MSG_SIZE) return(-1);
  
  // If a slot is available, add the message to the queue
  if (slot < QUEUE_SLOTS) {
    for (i=0; i<length; i++) pumpCmdQueueMsg[slot][i] = msg[i];
    pumpCmdQueueTimes[slot]     = times;
    pumpCmdQueueLen[slot]       = length;
    pumpCmdQueueTimeout[slot]   = timeout;
    pumpCmdQueueRetries[slot]   = retries;
    pumpCmdQueueExpectAns[slot] = expectedAnswer;
    pumpCmdQueuePending[slot]   = 1;
  } else {
    // Return error if no slot was found
    return(-1);
  }
  
  pumpCmdQueuePendingNumber = numberOfPendingMsgs ();
  
  return(slot);
}

void removeMsgFromQueue (char slot)
{
  char i,j;
  
  for (i=slot; i<QUEUE_SLOTS-1; i++) {
    if (pumpCmdQueuePending[i+1] == 1) {
      pumpCmdQueueTimeout[i]   = pumpCmdQueueTimeout[i+1];
      pumpCmdQueueRetries[i]   = pumpCmdQueueRetries[i+1];
      pumpCmdQueueTimes[i]     = pumpCmdQueueTimes[i+1];
      pumpCmdQueueLen[i]       = pumpCmdQueueLen[i+1];
      for (j=0; j<pumpCmdQueueLen[i+1] && j<QUEUE_MAX_MSG_SIZE; j++) 
        pumpCmdQueueMsg[i][j]  = pumpCmdQueueMsg[i+1][j];
      pumpCmdQueueExpectAns[i] = pumpCmdQueueExpectAns[i+1];
      pumpCmdQueuePending[i]   = 1;
    } else {
      pumpCmdQueueTimeout[i]   = 0;
      pumpCmdQueueRetries[i]   = 0;
      pumpCmdQueueTimes[i]     = 0;
      pumpCmdQueueLen[i]       = 0;
      pumpCmdQueuePending[i]   = 0;
      pumpCmdQueueExpectAns[i] = 0;
    }
  }
  
  pumpCmdQueueTimeout  [QUEUE_SLOTS-1] = 0;
  pumpCmdQueueRetries  [QUEUE_SLOTS-1] = 0;
  pumpCmdQueueTimes    [QUEUE_SLOTS-1] = 0;
  pumpCmdQueueLen      [QUEUE_SLOTS-1] = 0;
  pumpCmdQueuePending  [QUEUE_SLOTS-1] = 0;
  pumpCmdQueueExpectAns[QUEUE_SLOTS-1] = 0;
  
  pumpCmdQueuePendingNumber = numberOfPendingMsgs ();
  
}

void removeMsgBeingProccessed (void)
{
  char i = 0;
  
  // Look for the first pending slot
  while ((pumpCmdQueuePending[i] == 0) && (i<QUEUE_SLOTS)) i++;
  
  // Remove it from the queue
  removeMsgFromQueue (i);
  
  txTimer = 0;
  txing = 0;
}

void sendNextPumpCmd (void)
{
  char i = 0;
  
  // Look for the first pending slot
  while ((pumpCmdQueuePending[i] == 0) && (i<QUEUE_SLOTS)) i++;
  
  // If there's one slot waiting to be transmitted...
  if (i<QUEUE_SLOTS) {
    txTimer = pumpCmdQueueTimeout[i];
    txing = 1;
    if (adjustPumpFreqFlag == 0) {
      txRFMode = getBestPumpMode() | ((0x02 << 6) & 0xC0);
    }
    configureMedtronicRFMode (txRFMode);
    sendMedtronicMessage(pumpCmdQueueMsg[i],pumpCmdQueueLen[i],
                         pumpCmdQueueTimes[i]);
  }
  
}

char pumpExpectedAnswer (void)
{
  char i = 0;
  
  // Look for the first pending slot
  while ((pumpCmdQueuePending[i] == 0) && (i<QUEUE_SLOTS)) i++;
  
  if (i<QUEUE_SLOTS) return(pumpCmdQueueExpectAns[i]);
  else return(0x00);
}

void pumpCmdTimeout (void)
{
  char i = 0;
   
  // Look for the first pending slot
  while ((pumpCmdQueuePending[i] == 0) && (i<QUEUE_SLOTS)) i++;
  
  // If there's one slot waiting to be transmitted...
  if (i<QUEUE_SLOTS) {
    if (pumpCmdQueueRetries[i] > 0) {
      pumpCmdQueueRetries[i]--;
      txTimer = pumpCmdQueueTimeout[i];
      configureMedtronicRFMode (txRFMode);
      sendMedtronicMessage(pumpCmdQueueMsg[i],pumpCmdQueueLen[i],
                           pumpCmdQueueTimes[i]);
    } else {
      // Send the timeout signal to the callback
      pumpCommandsCallback(1,0,0);
      removeMsgFromQueue (i);
      txing = 0;
    }
  }
}

void pumpCommandsCallback ( char timeout, char *message, unsigned int length )
{
  unsigned char i;
  unsigned int tempInt;
  
  if (timeout == 1) {
    if (pumpCommandsFlags != 0x00) {
      composeActionNotificationMessage (PUMP_TIMEOUT);
      if (pumpCommandsFlags & PUMP_READ_PUMP_CONFIG_CMD_FLAG) {
        if (queryPumpFlag == 2) {
          queryPumpFlag = 0;
          sendFlag = 1;
        }
      }
      if (pumpCommandsFlags & PUMP_SET_TEMP_BASAL_CMD_FLAG) {
       sendLoopReport();
      }
      
      pumpCommandsFlags = 0x00;
    }

  } else {
    if (message == 0) return;
    if (length > 90) return;
    
    switch(pumpCommandsFlags) {
 /*   case PUMP_SUSPEND_CMD_FLAG:
      switch (lastPumpCommandSent) {
        case CMD_SET_RF_STATUS:
          timeCounterPumpAwake = _PUMP_AWAKE_TIME_ ;
          suspendResumeRequestCommand();
          break;
        case CMD_PUMP_SUSPEND:
          if (lastPumpCommandLengthSent == 7) suspendCommand();
          else {
            composeActionNotificationMessage (PUMP_SUSPEND_CMD_FLAG);
            pumpCommandsFlags = 0x00;
          }
          break;
        default:
          pumpCommandsFlags = 0x00;
          break;
        }
      break;
    
    case PUMP_RESUME_CMD_FLAG:
      switch (lastPumpCommandSent) {
      case CMD_SET_RF_STATUS:
        timeCounterPumpAwake = _PUMP_AWAKE_TIME_ ;
        suspendResumeRequestCommand();
        break;
      case CMD_PUMP_SUSPEND:
        if (lastPumpCommandLengthSent == 7) resumeCommand();
        else {
          composeActionNotificationMessage (PUMP_RESUME_CMD_FLAG);
          pumpCommandsFlags = 0x00;
        }
        break;
      default:
        pumpCommandsFlags = 0x00;
        break;
      }
      break; */
    
    case PUMP_SET_TEMP_BASAL_CMD_FLAG:
      switch (lastPumpCommandSent) {
      case CMD_SET_RF_STATUS:
        timeCounterPumpAwake = _PUMP_AWAKE_TIME_ ;
        setTempBasalRequestCommand();
        break;
      case CMD_SET_TEMP_BASAL:
        if (lastPumpCommandLengthSent == 7) 
          setTempBasalCommand(tempBasalMinutes,tempBasalRate);
        else {
          tempBasalActiveRate = tempBasalRate;
          tempBasalTimeLeft = tempBasalMinutes*60*TICKS_PER_SECOND;
          tempBasalTimeTotal = tempBasalMinutes*60*TICKS_PER_SECOND;
          addIOBForTempBasal(tempBasalActiveRate, tempBasalTimeTotal);
          tbdBasalIOB      = getToBeDeliveredIOB(basalUsed);
          totalIOB         = tbdBasalIOB + basalIOB + bolusIOB;
          composeActionNotificationMessage (PUMP_SET_TEMP_BASAL_CMD_FLAG);
          sendLoopReport();
          pumpCommandsFlags = 0x00;
        }
        break;
      default:
        pumpCommandsFlags = 0x00;
        break;
      }
      break;
      
    case PUMP_CANCEL_TEMP_BASAL_CMD_FLAG:
      switch (lastPumpCommandSent) {
      case CMD_SET_RF_STATUS:
        timeCounterPumpAwake = _PUMP_AWAKE_TIME_ ;
        setTempBasalRequestCommand();
        break;
      case CMD_SET_TEMP_BASAL:
        if (lastPumpCommandLengthSent == 7) cancelTempBasalCommand();
        else {
          tempBasalActiveRate = 0.0;
          tempBasalTimeLeft = 0;
          tempBasalTimeTotal = 0;
          cancelIOBForCurrentTempBasal(basalUsed);
          tbdBasalIOB      = getToBeDeliveredIOB(basalUsed);
          totalIOB         = tbdBasalIOB + basalIOB + bolusIOB;
          composeActionNotificationMessage (PUMP_CANCEL_TEMP_BASAL_CMD_FLAG);
          pumpCommandsFlags = 0x00;
        }
        break;
      default:
        pumpCommandsFlags = 0x00;
        break;
      }
      break; 
  
  /*  case PUMP_SET_BOLUS_CMD_FLAG:
      switch (lastPumpCommandSent) {
      case CMD_SET_RF_STATUS:
        timeCounterPumpAwake = _PUMP_AWAKE_TIME_ ;
        bolusRequestCommand();
        break;
      case CMD_BOLUS:
        if (lastPumpCommandLengthSent == 7) bolusCommand (bolusInsulinAmount);
        else {
          composeActionNotificationMessage (PUMP_SET_BOLUS_CMD_FLAG);
          pumpCommandsFlags = 0x00;
        }
        break;
      default:
        pumpCommandsFlags = 0x00;
        break;
      }
      break;*/
      
    case PUMP_READ_PUMP_CONFIG_CMD_FLAG:
      switch (lastPumpCommandSent) {
      case CMD_SET_RF_STATUS:
        timeCounterPumpAwake = _PUMP_AWAKE_TIME_ ;
        readRTCRequestCommand();
        break;
      case CMD_READ_PUMP_RTC:
        if (message[5] == 0x07) {
          systemTimeHour   = message[6] & 0x1F;
          systemTimeMinute = message[7] & 0x3F;
          systemTimeSecond = message[8] & 0x3F;
          systemTimeSynced = 1;
        }
        //readBGUnitsRequestCommand();
        readInsulinSensitivitiesRequestCommand();
        break;
      /*case CMD_READ_BG_UNITS:
        if (message[5] == 0x01) bgUnits = message[6];
        else bgUnits = 0x00;
        readCarbUnitsRequestCommand();
        break;
      case CMD_READ_CARB_UNITS:
        if (message[5] == 0x01) carbUnits = message[6];
        else carbUnits = 0x00;
        readCarbRatiosRequestCommand();
        break;
      case CMD_READ_CARB_RATIOS:
        for (i=0; (i<MAX_NUM_CARB_RATIOS) && 
                ((message[7+(i*2)  ] != 0x00) ||
                (message[7+(i*2)+1] != 0x00)); i++) {
          carbRatioAmount[i] = (message[7+(i*2)+1]); // *0.1
          carbRatioHour[i]   = (message[7+(i*2)] >> 1) & 0x7F;
          carbRatioMinute[i] = (message[7+(i*2)] & 0x01) ? 30 : 0 ;
        }
        carbRatioValidRanges = i;
        readInsulinSensitivitiesRequestCommand();
        break;*/
      case CMD_READ_INSULIN_SENSITIVITIES:
        for (i=0; (i<MAX_NUM_INSULIN_SENS) && 
                ((message[7+(i*2)  ] != 0x00) ||
                (message[7+(i*2)+1] != 0x00)); i++) {
          insulinSensAmount[i] = message[7+(i*2)+1];
          insulinSensHour[i]   = (message[7+(i*2)] >> 1) & 0x7F;
          insulinSensMinute[i] = (message[7+(i*2)] & 0x01) ? 30 : 0 ;
        }
        insulinSensValidRanges = i;
        readBGTargetsRequestCommand();
        break;
      case CMD_READ_BG_TARGETS:
        if (message[6] == 0x01) {
          //controlLoopThresHigh = message[9] & 0x0FF;
          //controlLoopThresLow  = message[8] & 0x0FF;
          for (i=0; (i<MAX_NUM_TARGETS) && 
                 ((message[7+(i*3)+1] != 0x00) ||
                  (message[7+(i*3)+2] != 0x00)); i++) {
            pumpTargetsMin    [ i ]  = (message[7+(i*3)+1]) & 0x0FF;
            pumpTargetsMax    [ i ]  = (message[7+(i*3)+2]) & 0x0FF;
            pumpTargetsHour   [ i ]  = (message[7+(i*3)] >> 1) & 0x7F;
            pumpTargetsMinute [ i ]  = (message[7+(i*3)] & 0x01) ? 30 : 0 ;
          }
          pumpTargetsValidRanges = i;
        }
        readPumpParamsRequestCommand();
        break;
      case CMD_READ_PUMP_PARAMS:
        if (message[5] == 0x19) {
          maxBasal = (((unsigned int)(message[13]) << 8) & 0x0FF00) |
                      ((unsigned int)(message[14]) & 0x00FF);
          maxBolus = (((unsigned int)(message[11]) << 8) & 0x0FF00) |
                      ((unsigned int)(message[12]) & 0x00FF);
          maxBolus = (maxBolus << 2) & 0xFFFC;
          durationIOB = message[23] & 0x07;
          initResponseToInsulin((float)(durationIOB),30/5);
        } else if (message[5] == 0x15) {
          maxBasal = (((unsigned int)(message[12]) << 8) & 0x0FF00) |
                      ((unsigned int)(message[13]) & 0x00FF);
          maxBolus = (((unsigned int)(message[11]) << 2) & 0x003FC);
          durationIOB = message[23] & 0x07;
          initResponseToInsulin((float)(durationIOB),30/5);
        }
        
        readStdBasalProfileRequestCommand();
        break;
      case CMD_READ_STD_BASAL_PROFILE:
        for (i=0; (i<MAX_NUM_BASALS) && 
                ((message[6+(i*3)  ] != 0x00) ||
                (message[6+(i*3)+1] != 0x00) ||
                (message[6+(i*3)+2] != 0x00)); i++) {
          pumpBasalsAmount[i] = (message[6+(i*3)]); // *0.025
          pumpBasalsHour[i]   = ((message[6+(i*3)+2])>>1) & 0x7F;
          pumpBasalsMinute[i] = (message[6+(i*3)+2] & 0x01) ? 30 : 0 ;
        }
        pumpBasalsValidRanges = i;
        readSensorCalibrationFactorRequestCommand();
        break;
      case CMD_READ_CALIBRATION_FACTOR:
        tempInt  = (message[6] << 8) & 0xFF00;
        tempInt |= (message[7] & 0x00FF);
        calFactor  = (float)(tempInt) / 1000.0;
        readReservoirRequestCommand();
        break;
      
      case CMD_READ_PUMP_REMAINING_INSULIN:
        if (message[5] == 0x04) {
          tempInt  = (message[8] << 8) & 0xFF00;
          tempInt |= (message[9] & 0x00FF);
          pumpReservoir = (float)(tempInt) / 40.0;
        } else if (message[5] == 0x02) {
          tempInt  = (message[6] << 8) & 0xFF00;
          tempInt |= (message[7] & 0x00FF);
          pumpReservoir = (float)(tempInt) / 10.0;
        }
        readBatteryRequestCommand();
        break;
        
      case CMD_READ_PUMP_BATTERY_STATUS:
        if (message[5] == 0x03) {
          tempInt = (unsigned char)(message[8]);
          pumpBattery = (float)(tempInt) / 100.0;
        }
        pumpCommandsFlags = 0x00;
        if (queryPumpFlag == 2) {
          queryPumpFlag = 0;
          sendFlag = 1;
        }
        break;
        
      default:
        pumpCommandsFlags = 0x00;
        if (queryPumpFlag == 2) {
          queryPumpFlag = 0;
          sendFlag = 1;
        }
        break;
      }
    break;
  
/*    case PUMP_READ_SENSOR_CAL_FACTOR_CMD_FLAG:
      switch (lastPumpCommandSent) {
      case CMD_SET_RF_STATUS:
        timeCounterPumpAwake = _PUMP_AWAKE_TIME_ ;
        readSensorCalibrationFactorRequestCommand();
        break;
      case CMD_READ_CALIBRATION_FACTOR:
        tempInt  = (message[6] << 8) & 0xFF00;
        tempInt |= (message[7] & 0x00FF);
        calFactor  = (float)(tempInt) / 1000.0;
        pumpCommandsFlags = 0x00;
        break;
      default:
        pumpCommandsFlags = 0x00;
        break;
      }
      break;*/
    
    default:
      pumpCommandsFlags = 0x00;
      break;
    }
  }
}

void pumpWakeUp (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_SET_RF_STATUS;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 512, CMD_PUMP_ACK,
                3*DEFAULT_MSG_TIMEOUT,DEFAULT_MSG_RETRIES);        
}

void pumpWakeUpRFTest (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_SET_RF_STATUS;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 256, CMD_PUMP_ACK,
                (0*60+8)*TICKS_PER_SECOND, 0);        
}

/*void suspendPump (void)
{
  if (pumpCommandsFlags == 0) {
    pumpCommandsFlags = PUMP_SUSPEND_CMD_FLAG;
    if (timeCounterPumpAwake == 0) pumpWakeUp();
    else suspendResumeRequestCommand();
  }
}

void resumePump (void)
{
  if (pumpCommandsFlags == 0) {
    pumpCommandsFlags = PUMP_RESUME_CMD_FLAG;
    if (timeCounterPumpAwake == 0) pumpWakeUp();
    else suspendResumeRequestCommand();
  }
}*/

void setTempBasal (unsigned int minutes, float rate)
{
  if (pumpCommandsFlags == 0) {
    pumpCommandsFlags = PUMP_SET_TEMP_BASAL_CMD_FLAG;
    if (rate < 0.0) {
      tempBasalRate = 0.0;
    } else if (rate > ((float)(maxBasal)*0.025)) {
      tempBasalRate = ((float)(maxBasal)*0.025);
    } else {
      tempBasalRate = rate;
    }
    tempBasalMinutes = minutes;
    if (timeCounterPumpAwake == 0) pumpWakeUp();
    else setTempBasalRequestCommand();
  }
}

void cancelTempBasal (void)
{
  if (pumpCommandsFlags == 0) {
    pumpCommandsFlags = PUMP_CANCEL_TEMP_BASAL_CMD_FLAG;
    if (timeCounterPumpAwake == 0) pumpWakeUp();
    else setTempBasalRequestCommand();
  }
}

/*void bolus (float insulinAmount)
{
  if (pumpCommandsFlags == 0) {
    pumpCommandsFlags = PUMP_SET_BOLUS_CMD_FLAG;
    bolusInsulinAmount = insulinAmount;
    if (timeCounterPumpAwake == 0) pumpWakeUp();
    else bolusRequestCommand();
  }
}*/

/*void suspendResumeRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_PUMP_SUSPEND;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_PUMP_ACK,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}

void suspendCommand (void)
{
  char msg[71];
  unsigned int i;
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_PUMP_SUSPEND;
  msg[5] = 0x01;
  msg[6] = 0x01;
  for (i=7; i<70; i++) msg[i] = 0x00;;
  msg[70] = crc8(msg,70);
  
  addMsgToQueue(msg, 71, 1, CMD_PUMP_ACK,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}

void resumeCommand (void)
{
  char msg[71];
  unsigned int i;
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_PUMP_SUSPEND;
  msg[5] = 0x01;
  msg[6] = 0x00;
  for (i=7; i<70; i++) msg[i] = 0x00;
  msg[70] = crc8(msg,70);
  
  addMsgToQueue(msg, 71, 1, CMD_PUMP_ACK,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);
}*/

void setTempBasalRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_SET_TEMP_BASAL;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_PUMP_ACK,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}

void setTempBasalCommand (unsigned int minutes, float rate)
{
  char msg[71];
  unsigned int i;
  int temp;
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_SET_TEMP_BASAL;
  msg[5] = 0x03;
  
  temp = (int)(rate / 0.025);
  temp = (temp < 0) ? 0 : temp;
  temp = (temp > (int)(maxBasal)) ? (int)(maxBasal) : temp ;
  msg[6] = (temp >> 8) & 0x0FF;
  msg[7] = temp & 0x0FF; // Rate in 0.025 steps
  
  temp = minutes / 30;
  temp = (temp < 0) ? 0 : temp;
  temp = (temp > (int)(MAX_TEMP_BASAL_DURATION*2.0)) ? 
                 (int)(MAX_TEMP_BASAL_DURATION*2.0) : temp ;
  msg[8] = temp & 0x0FF; // Duration in 30 minute periods
  
  for (i=9; i<70; i++) msg[i] = 0x00;
  msg[70] = crc8(msg,70);
  
  addMsgToQueue(msg, 71, 1, CMD_PUMP_ACK,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);
}

void cancelTempBasalCommand (void)
{
  setTempBasalCommand(0,0.0);
}

/*void bolusRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_BOLUS;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_PUMP_ACK,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}

void bolusCommand (float insulinAmount)
{
  char msg[71];
  unsigned int i;
  int temp;
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_BOLUS;
  
  temp = (int)(insulinAmount / 0.025);
  //temp = (int)(insulinAmount / 0.1);
  temp = (temp < 0) ? 0 : temp;
  temp = (temp > (int)(maxBolus)) ? (int)(maxBolus) : temp ;
  
  if (temp < 255) {
    msg[5] = 0x01;
    msg[6] =   temp        & 0x0FF;
    msg[7] = 0x00;
  } else {
    msg[5] = 0x02;
    msg[6] = ( temp >> 8 ) & 0x0FF; // Rate in 0.025 steps
    msg[7] =   temp        & 0x0FF;
  }
  
  for (i=8; i<70; i++) msg[i] = 0x00;
  msg[70] = crc8(msg,70);
  
  addMsgToQueue(msg, 71, 1, CMD_PUMP_ACK,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);
}*/

void readPumpConfig (void)
{
  if (pumpCommandsFlags == 0) {
    pumpCommandsFlags = PUMP_READ_PUMP_CONFIG_CMD_FLAG;
    pumpWakeUp();
  } else {
    if (queryPumpFlag == 2) {
      queryPumpFlag = 0;
      sendFlag = 1;
    }
  }
}

void readStdBasalProfileRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_READ_STD_BASAL_PROFILE;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_READ_STD_BASAL_PROFILE,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}

/*void readCarbUnitsRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_READ_CARB_UNITS;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_READ_CARB_UNITS,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}*/

/*void readBGUnitsRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_READ_BG_UNITS;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_READ_BG_UNITS,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}*/

/*void readCarbRatiosRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_READ_CARB_RATIOS;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_READ_CARB_RATIOS,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}*/

void readInsulinSensitivitiesRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_READ_INSULIN_SENSITIVITIES;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_READ_INSULIN_SENSITIVITIES,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}

void readBGTargetsRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_READ_BG_TARGETS;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_READ_BG_TARGETS,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}

void readRTCRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_READ_PUMP_RTC;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_READ_PUMP_RTC,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}

/*void readPumpStateCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_READ_PUMP_STATE;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_READ_PUMP_STATE,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}*/



/*void readBolusHistory (void)
{
  if (pumpCommandsFlags == 0) {
    pumpCommandsFlags = PUMP_READ_BOLUS_HISTORY_CMD_FLAG;
    pumpWakeUp();
  }
}

void readBolusHistoryRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = 0x27;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg,7,1,0x06,DEFAULT_MSG_TIMEOUT,DEFAULT_MSG_RETRIES);  
}*/

void readSensorCalibrationFactor (void)
{
  if (pumpCommandsFlags == 0) {
    pumpCommandsFlags = PUMP_READ_SENSOR_CAL_FACTOR_CMD_FLAG;
    pumpWakeUp();
  }
}

void readSensorCalibrationFactorRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_READ_CALIBRATION_FACTOR;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_READ_CALIBRATION_FACTOR,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}

void readPumpParamsRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_READ_PUMP_PARAMS;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_READ_PUMP_PARAMS,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}

void readReservoirRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_READ_PUMP_REMAINING_INSULIN;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_READ_PUMP_REMAINING_INSULIN,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}

void readBatteryRequestCommand (void)
{
  char msg[7];
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_READ_PUMP_BATTERY_STATUS;
  msg[5] = 0x00;
  msg[6] = crc8(msg,6);
  
  addMsgToQueue(msg, 7, 1, CMD_READ_PUMP_BATTERY_STATUS,
                DEFAULT_MSG_TIMEOUT, DEFAULT_MSG_RETRIES);  
}

void suspendJamming (void)
{
  char msg[71];
  unsigned int i;
  
  msg[0] = 0xA7;
  msg[1] = pumpID[0];
  msg[2] = pumpID[1];
  msg[3] = pumpID[2];
  msg[4] = CMD_PUMP_SUSPEND;
  msg[5] = 0x01;
  msg[6] = 0x01;
  for (i=7; i<70; i++) msg[i] = 0x00;;
  msg[70] = crc8(msg,70);
 
  //configureMedtronicRFMode (txRFMode);
  //sendMedtronicMessage(msg,71,255);
  
  addMsgToQueue(msg, 71, 512, CMD_PUMP_ACK,
                DEFAULT_MSG_TIMEOUT,3*DEFAULT_MSG_RETRIES);       
}
