#include "ioCCxx10_bitdef.h"
#include "ioCC1110.h"
#include "medtronicRF.h"
#include "crc_4b6b.h"
#include "constants.h"
#include "globals.h"
#include "interrupts.h"
#include "bleComms.h"
#include "dataProcessing.h"
#include "pumpCommands.h"
#include "freqManagement.h"
#include "smartRecovery.h"

// Globals
char          __xdata rfMessage[SIZE_OF_RF_BUFFER];
char          __xdata rfMessageTx[SIZE_OF_RF_BUFFER];
int           __xdata rfMessageTxLength;
int           __xdata rfMessagePointerIn;
int           __xdata rfMessagePointerOut;
int           __xdata rfMessagePointerLen;
char          __xdata rfMessageRXInProgress;
char          __xdata rfLastData;
char          __xdata rfTXMode;
int           __xdata rfMessagePointerTx;

int    __xdata txCalcCRC;
int    __xdata txCalcCRC16;
char   __xdata txLength;
int    __xdata txTimes;


void configureMedtronicRFMode (char mode)
{
  //char channel;
  char bwReg,i;
  //unsigned long freqSelected;
  char freqReg[3];
  char shift;
  
  RFST = RFST_SIDLE;
  
  SYNC1 = 0xFF; SYNC0 = 0x00;
  PKTLEN = 0xFF;
  PKTCTRL1 = 0x00; PKTCTRL0 = 0x00;
  ADDR = 0x00;
  CHANNR = 0x00;
  FSCTRL1 = 0x06; FSCTRL0 = 0x00;
  
  channel = mode & 0x3F;
  bwReg = _WIDE_BANDWIDTH_;
  
  switch((mode>>6) & 0x03){
  case 0x01:
  case 0x02:
  case 0x03:
    shift = ((mode>>6) & 0x03)-1;
    if ((rfFrequencyMode>>shift) & 0x01)  freqSelected = _INIT_FREQ_USA_;
    else                                  freqSelected = _INIT_FREQ_EUR_;
    if (channel >= 32) {
      for (i=0; i<16; i++) freqSelected += (long)(_RF_CHANNEL_SPACING_);
      bwReg  = _WIDE_BANDWIDTH_;
    } else {
      for (i=0; i<channel; i++) freqSelected += (long)(_RF_CHANNEL_SPACING_);
      bwReg  = _NARROW_BANDWIDTH_;
    }
    freqReg[2] = (freqSelected >> 16) & 0x0FF;
    freqReg[1] = (freqSelected >>  8) & 0x0FF;
    freqReg[0] = (freqSelected      ) & 0x0FF;
    break;
    
  default:
    freqReg[2] = genericFreq[2];
    freqReg[1] = genericFreq[1];
    freqReg[0] = genericFreq[0];
    bwReg      = genericBW;
    break;
  }
  
  MDMCFG4 = bwReg | 0x09;
  FREQ2   = freqReg[2];
  FREQ1   = freqReg[1];
  FREQ0   = freqReg[0];
    
  MDMCFG3 = 0x66; 
  MDMCFG2 = 0x33; 
  MDMCFG1 = 0x62; 
  MDMCFG0 = 0x1A; 
  DEVIATN = 0x13;
  MCSM2 = 0x07; MCSM1 = 0x30; MCSM0 = 0x18;
  FOCCFG = 0x17;
  BSCFG = 0x6C;
  AGCCTRL2 = 0x03; AGCCTRL1 = 0x40; AGCCTRL0 = 0x91;
  FREND1 = 0x56; FREND0 = 0x12;
  FSCAL3 = 0xE9; FSCAL2 = 0x2A; FSCAL1 = 0x00; FSCAL0 = 0x1F;
  TEST2 = 0x88; TEST1 = 0x31; TEST0 = 0x09;
  PA_TABLE7 = 0x00; PA_TABLE6 = 0x00; PA_TABLE5 = 0x00; PA_TABLE4 = 0x00;
  PA_TABLE3 = 0x00; PA_TABLE2 = 0x52; PA_TABLE1 = 0x00; PA_TABLE0 = 0x00;

  RFTXRXIE = 0;
  
  if (rfState[0] == 1) {
    RFST = RFST_SRX;
  } else {
    RFST = RFST_SIDLE;

    // Clear interrupt flags when idle
    RFIF     =  0x00;
    S1CON   &= ~0x03;
    IEN0    &= ~0x01;
    RFTXRXIF =  0;
  }
 
}

void sendMedtronicMessage (char *message, unsigned int length, int times)
{

  if (times > 0) {
    encode4b6b (message, length, rfMessageTx, (unsigned int *)&rfMessageTxLength);
    PKTLEN = rfMessageTxLength;
    txTimes = times;
  
    rfMessagePointerTx=0;
    IEN0 |= 0x01;  // Enable RF TXRX Interrupt
    TCON &= ~0x02;
    RFIM |=  0x90;  // Unmask RF Interrupts
    lastPumpCommandSent = message[4];
    lastPumpCommandLengthSent = length;
    rfTXMode = 1;
    RFST = RFST_SIDLE;
    RFST = RFST_STX;
  }
}

char receiveMedtronicMessage (char *message, unsigned int *length)
{
  char calcCRC;
  short calcCRC16;
  char tempMsg[120];
  int  tempLen;
  int  i,j;
    
  tempLen = (int)((rfMessage[rfMessagePointerOut]) & 0x000FF);
    
  if (tempLen > 118) {
    //while ((rfMessageRXInProgress == 1) ||
    //       (rfMessagePointerTx != 0));
    //resetRFBuffers();
    *length = 0;
    discardRFMessage();
    return(1);
  }
  
  
  j= ((rfMessagePointerOut+1) >= SIZE_OF_RF_BUFFER) ? 0 
                                                    : rfMessagePointerOut + 1 ;
  for (i=0; i<tempLen; i++) {
    tempMsg[i] = rfMessage[j];
    j = ((j+1) >= SIZE_OF_RF_BUFFER) ? 0 : j+1;
  }
  discardRFMessage();
  
  decode4b6b(tempMsg,tempLen,message,length);
  calcCRC = crc8(message,(*length)-1);
  
  if (calcCRC == message[(*length)-1]) {
    return (0);
  } 
  
  calcCRC16 = crc16(message,(*length)-2);
  if (((char)( calcCRC16       & 0x00FF) == message[(*length)-1]) && 
      ((char)((calcCRC16 >> 8) & 0x00FF) == message[(*length)-2])) {
    return (0);
  }
  
  calcCRC = crc8(message,(*length)-2);
  
  if (calcCRC == message[(*length)-2]) {
    (*length) = (*length)-1;
    return (0);
  } 
 
  calcCRC16 = crc16(message,(*length)-3);
  if (((char)( calcCRC16       & 0x00FF) == message[(*length)-2]) &&
      ((char)((calcCRC16 >> 8) & 0x00FF) == message[(*length)-3])) {
    (*length) = (*length)-1;
    return (0);
  }
 
  crc16Init();
  return(1);
}

void discardRFMessage (void)
{
  int tempPointer;
  tempPointer = rfMessagePointerOut + rfMessage[rfMessagePointerOut] + 1;
  if (tempPointer >= SIZE_OF_RF_BUFFER) tempPointer -= SIZE_OF_RF_BUFFER;
  rfMessagePointerOut = tempPointer;
}

void resetRFBuffers (void)
{
  int i;
  rfMessagePointerOut = 0;
  rfMessagePointerIn  = 0;
  rfMessagePointerLen = 0;
  rfMessagePointerTx  = 0;
  rfMessageTxLength   = 0;
  for (i=0;i<SIZE_OF_RF_BUFFER;i++) {
    rfMessage[i]   = 0x00;
    rfMessageTx[i] = 0x00;
  }
}

void checkMedtronicRF (void)
{
  if (rfMessage[rfMessagePointerOut] != 0x00) {
    dataErr = receiveMedtronicMessage(dataPacket, &dataLength);
    if (dataLength > 0) {
      smartRecovery(dataPacket,&dataLength,&dataErr);
      adjustFrequencies(dataPacket,dataLength,dataErr);
      processMessage(dataPacket,dataLength,dataErr);        
    }
  }
}