#include "globals.h"
#include "freqManagement.h"
#include "crc_4b6b.h"
#include "init.h"
#include "pumpCommands.h"

void adjustFrequencies (char *message, unsigned int length, char dataErr)
{
  char channel;
  
  // First check if the message has a correct CRC
  if (dataErr == 0) {
    
    // Get the RF channel used
    channel = lastRSSIMode & 0x3F;
    
    // If RSSI is from the generic channel, divide by 4. (> BW)
    if (channel == 0x20) lastRSSI = lastRSSI >> 2;
    
    // Second check if it's a minilink message
    if (((message[0] == 0xAA) || (message[0] == 0xAB)) &&
        ((message[2] == minilinkID[0]) && 
         (message[3] == minilinkID[1]) &&
         (message[4] == minilinkID[2])) && 
         (bestMinilinkFreqFound == 0))  {
      if (minilinkRSSI[channel] < lastRSSI) {
          minilinkRSSI[channel] = lastRSSI;
      }  
    }
    
    // Pump Message
    else if (((message[0] == 0xA2) || (message[0] == 0xA7)) &&
             ((message[1] == pumpID[0]) &&
              (message[2] == pumpID[1]) &&
              (message[3] == pumpID[2])) && 
              (bestPumpFreqFound == 0) ) {
      if (pumpRSSI[channel] < lastRSSI) {
          pumpRSSI[channel] = lastRSSI;
      }
    }
    
    // Glucometer Reading
    else if ((message[0] == 0xA5) &&
       ((message[1] == glucometerID[0]) && 
        (message[2] == glucometerID[1]) &&
        (message[3] == glucometerID[2])) && 
        (bestGlucometerFreqFound == 0)) {
      if (glucometerRSSI[channel] < lastRSSI) {
          glucometerRSSI[channel] = lastRSSI;
      }
    }    
  }
}

char getBestMinilinkMode (void)
{
  char i;
  char bestRSSI;
  char bestChannel;
  
  bestChannel = 0;
  bestRSSI = minilinkRSSI[0];
  
  for (i=1; i<33; i++) {
    if (minilinkRSSI[i] >= bestRSSI) {
      bestRSSI = minilinkRSSI[i];
      bestChannel = i;
    }
  }
  
  bestChannel |= ((0x01 << 6) & 0xC0);
  
  bestMinilinkChannel[0] = bestChannel;
  bestMinilinkChannel[1] = bestRSSI;
  
  return(bestChannel);
}

char tryNextMinilinkMode (void)
{
  char channel;
  
  channel = ((0x01 << 6) & 0xC0) | lastMinilinkChannel;
  
  return(channel);
}

char getBestPumpMode (void)
{
  char i;
  char bestRSSI;
  char bestChannel;
  
  bestChannel = 0;
  bestRSSI = pumpRSSI[0];
  
  for (i=1; i<33; i++) {
    if (pumpRSSI[i] >= bestRSSI) {
      bestRSSI = pumpRSSI[i];
      bestChannel = i;
    }
  }
  
  bestChannel |= ((0x02 << 6) & 0xC0);
  
  bestPumpChannel[0] = bestChannel;
  bestPumpChannel[1] = bestRSSI;
  
  return(bestChannel);
}

char tryNextPumpMode (void)
{
  char channel;

  channel = lastPumpChannel;
  
  return(channel);
}

char getBestGlucometerMode (void)
{
  char i;
  char bestRSSI;
  char bestChannel;
  
  bestChannel = 0;
  bestRSSI = glucometerRSSI[0];
  
  for (i=1; i<33; i++) {
    if (glucometerRSSI[i] >= bestRSSI) {
      bestRSSI = glucometerRSSI[i];
      bestChannel = i;
    }
  }
  
  bestChannel |= ((0x03 << 6) & 0xC0);
  
  bestGlucometerChannel[0] = bestChannel;
  bestGlucometerChannel[1] = bestRSSI;
  
  return(bestChannel);
}

char tryNextGlucometerMode (void)
{
  char channel;
   
  channel = ((0x03 << 6) & 0xC0) | lastGlucometerChannel;
  
  return(channel);
}

void updateNextMinilinkMode (void)
{
  char bestChannel, channel;

  bestChannel = getBestMinilinkMode();
  channel = lastMinilinkChannel;
  
  do {
    channel = channel + 1;
    if (channel == 33) channel = 0;
  } while (channel == bestChannel);
  
  lastMinilinkChannel = channel;
  
  checkIfBestMinilinkFreqWasFound();
}

void updateNextPumpMode (void)
{
  char bestChannel, channel;

  bestChannel = getBestPumpMode();
  channel = lastPumpChannel;
  
  do {
    channel = channel + 1;
    if (channel == 33) channel = 0;
  } while (channel == bestChannel);
  
  if (channel < lastPumpChannel) checkIfBestPumpFreqWasFound();
  
  lastPumpChannel = channel;
}

void updateNextGlucometerMode (void)
{
  char bestChannel, channel;
  
  bestChannel = getBestGlucometerMode();
  channel = lastGlucometerChannel;
  
  do {
    channel = channel + 1;
    if (channel == 33) channel = 0;
  } while (channel == bestChannel);
  
  if (channel < lastGlucometerChannel) checkIfBestGlucometerFreqWasFound();
  
  lastGlucometerChannel = channel;
}

void checkIfBestMinilinkFreqWasFound (void)
{
  char i;
  char localBest;
  
  if (bestMinilinkFreqFound == 0) {
    if (lastMinilinkChannel > 1) {
      localBest = 40;
      for (i=0; i<lastMinilinkChannel; i++) 
        if (minilinkRSSI[i] > 10) localBest = i;
      if (localBest < 30) { 
        if (lastMinilinkChannel < (localBest + 2)) localBest = 40;
      }
      if (localBest < 32) bestMinilinkFreqFound = 1;
    }
  }
  
}

void checkIfBestPumpFreqWasFound (void)
{
  char i;
  char localBest;
  
  localBest = 40;
  for (i=0; i<32; i++) if (pumpRSSI[i] > 0) localBest = i;
  if (localBest < 32) bestPumpFreqFound = 1;
  else                bestPumpFreqFound = 0;
}

void checkIfBestGlucometerFreqWasFound (void)
{
  char i;
  char localBest;
  
  localBest = 40;
  for (i=0; i<32; i++) if (glucometerRSSI[i] > 0) localBest = i;
  if (localBest < 32) bestGlucometerFreqFound = 1;
  else                bestGlucometerFreqFound = 0;
  
}

void adjustPumpFrequency (void)
{
  unsigned char i;
  
  for (i=0; i<32; i++) {
    pumpRSSI       [ i ] = 0x00 ;
  }
  pumpRSSI       [ 32 ] = 0x01 ;
  lastPumpChannel       = 0 ;
  bestPumpFreqFound     = 0 ;
  
  txRFMode = 0x80;
  adjustPumpFreqFlag = 1;     
}