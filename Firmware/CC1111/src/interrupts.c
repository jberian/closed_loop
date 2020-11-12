#include "globals.h"
#include "ioCCxx10_bitdef.h"
#include "ioCC1110.h"
#include "crc_4b6b.h"
#include "interrupts.h"
#include "medtronicRF.h"
#include "bleComms.h"
#include "timingController.h"
#include "dataProcessing.h"
#include "init.h"
#include "freqManagement.h"
#include "pumpCommands.h"

#pragma vector = T1_VECTOR
__interrupt void TIMER1_ISR(void)
{
  // Clear Timer 1 Interrupt Flag
  IRCON &= ~0x02;
  // Time counter update
  if (timeCounter < _MAX_TIMECOUNTER_VAL_ ) timeCounter++;
  // Ask for timing and RF reconfiguration
  timeUpdatedFlag = 1;
}

void stopTimerInt (void)
{
  // Stop Timer 1
  T1CTL &= 0xFC;
  // Reset Timer 1 Counter
  T1CNTL = 0x00;
  // Disable Timer 1 interrupt
  IEN1 &= ~0x02;
}

void resetTimerCounter (void)
{
  timeCounter = 0;
  T1CNTL = 0x00;
}

void enableTimerInt (void)
{
  // Stop Timer 1
  T1CTL = 0x08;

  // Set Timer 1 timeout value = Every 200 ms
  T1CC0H = 0x49;
  T1CC0L = 0x3D;
    
  // Reset Timer 1 Counter
  T1CNTL = 0x00;
 
  // Set Timer 1 mode 
  T1CCTL0 = 0x44; 
 
  // Clear any pending Timer 1 Interrupt Flag
  IRCON &= ~0x02;
  
  // Enable global interrupt (IEN0.EA = 1) and Timer 1 Interrupt (IEN1.1 = 1)
  EA = 1; IEN1 |= 0x02;  
  
  // Start Timer 1
  T1CTL = 0x0A; // Tick frequency /32
}

#pragma vector = URX0_VECTOR
__interrupt void UART0_RX_ISR(void)
{ 
  // Clear UART0 RX Interrupt Flag (TCON.URX0IF = 0)
  URX0IF = 0;
 
  if (bleCommsWatchdogTimer >= (0*60 + 2)*TICKS_PER_SECOND) {
    bleRxBuffer[bleRxIndexOut] = U0DBUF;
    bleRxIndexIn = bleRxIndexOut + 1;
  } else {
    bleRxBuffer[bleRxIndexIn] = U0DBUF;
    bleRxIndexIn = ((bleRxIndexIn+1) >= SIZE_OF_UART_RX_BUFFER) ? 0 : bleRxIndexIn + 1;
  }

  bleCommsWatchdogTimer = 0;
  bleRxFlag = 1;
}

void checkBleUartComms(void)
{ 
  char notFinishedYet, i, j;
  char tmpMsg[64];
  char tmpLen;
  
  if ((bleTxIndexIn != bleTxIndexOut) && (bleTxing == 0)) {
    uart0StartTxForIsr();
  }
    
  bleRxFlag = 0;
  notFinishedYet = 1;
  while ((notFinishedYet == 1) && (bleRxIndexIn != bleRxIndexOut)) {
    if (bleRxIndexIn < bleRxIndexOut) {
      tmpLen  = SIZE_OF_UART_RX_BUFFER - bleRxIndexOut;
      tmpLen += bleRxIndexIn;
    } else {
      tmpLen  = bleRxIndexIn - bleRxIndexOut;
    }
    if (tmpLen > (bleRxBuffer[bleRxIndexOut] & 0x00FF)) {
      i = bleRxIndexOut+1;
      for (j=0; j<(bleRxBuffer[bleRxIndexOut] & 0x00FF); j++) {
        tmpMsg[j] = bleRxBuffer[i++];
        if (i >= SIZE_OF_UART_RX_BUFFER) i = 0;
      }
      
      tmpLen = ((unsigned int)bleRxBuffer[bleRxIndexOut] & 0x00FF);
      
      bleRxIndexOut += (bleRxBuffer[bleRxIndexOut] & 0x00FF) + 1;
      if (bleRxIndexOut >= SIZE_OF_UART_RX_BUFFER) {
        bleRxIndexOut -= SIZE_OF_UART_RX_BUFFER;
      }
      
      receiveBLEMessage (tmpMsg, tmpLen);
    } else {
      notFinishedYet = 0;
    }
  }
 }

#pragma vector = UTX0_VECTOR
__interrupt void UART0_TX_ISR(void)
{
  // Clear UART0 TX Interrupt Flag (IRCON2.UTX0IF = 0)
  UTX0IF = 0;

  // If no UART byte left to transmit, stop this UART TX session
  if (bleTxIndexIn == bleTxIndexOut)
  //if (bleTxIndex >= bleTxLength)
  {
    // Note:
    // In order to start another UART TX session the application just needs
    // to prepare the source buffer, and simply send the very first byte.
    //bleTxIndex = 0; 
    bleTxing = 0;
    IEN2 &= ~IEN2_UTX0IE; return;
  } else {
    // Send next UART byte
    bleTxing = 1;
    U0DBUF = bleTxBuffer[bleTxIndexOut];
    bleTxIndexOut = (bleTxIndexOut+1 >= SIZE_OF_UART_TX_BUFFER) ? 0 : bleTxIndexOut+1;
  }
}

void uart0StartRxForIsr(void)
{

  // Initialize the UART RX buffer index
  bleRxIndexIn  = 0;
  bleRxIndexOut = 0;

  // Clear any pending UART RX Interrupt Flag (TCON.URXxIF = 0, UxCSR.RX_BYTE = 0)
  URX0IF = 0; U0CSR &= ~U0CSR_RX_BYTE;

  // Enable UART RX (UxCSR.RE = 1)
  U0CSR |= U0CSR_RE;

  // Enable global Interrupt (IEN0.EA = 1) and UART RX Interrupt (IEN0.URXxIE = 1)
  EA = 1; URX0IE = 1;
}

void uart0StartTxForIsr(void)
{

  // Initialize the UART TX buffer indexes.
  //bleTxIndex = 0;

  if ((bleTxIndexIn != bleTxIndexOut) && (bleTxing == 0)){
    // Clear any pending UART TX Interrupt Flag (IRCON2.UTXxIF = 0, UxCSR.TX_BYTE = 0)
    UTX0IF = 0; U0CSR &= ~U0CSR_TX_BYTE;
    
    bleTxing = 1;
    
    // Send very first UART byte
    U0DBUF = bleTxBuffer[bleTxIndexOut];
    bleTxIndexOut = ((bleTxIndexOut+1) >= SIZE_OF_UART_TX_BUFFER) ? 0 : bleTxIndexOut + 1;

    // Enable global interrupt (IEN0.EA = 1) and UART TX Interrupt (IEN2.UTXxIE = 1)
    EA = 1; IEN2 |= IEN2_UTX0IE;

  }
}

void addCharToBleTxBuffer (char byteToTransmit)
{
  int limitIndex;
  
  limitIndex = (bleTxIndexOut == 0) ? SIZE_OF_UART_TX_BUFFER - 1 : bleTxIndexOut - 1;
  while (bleTxIndexIn == limitIndex);
  
  bleTxBuffer[bleTxIndexIn] = byteToTransmit;
  bleTxIndexIn = ((bleTxIndexIn+1)>= SIZE_OF_UART_TX_BUFFER) ? 0 : bleTxIndexIn + 1;
  //if (bleTxing == 0) uart0StartTxForIsr();
}

void enableRadioInt (void)
{
  S1CON &= ~0x03; // Clear CPU interrupt flag
  IEN0 &= ~0x01;  // Disable RF TXRX Interrupt
  IEN2 |=  0x01;  // Enable RF Interrupt
  RFIM |=  0xD1;  // Unmask RF Interrupts
  EA = 1;  // Enable global interrupt 
}

#pragma vector = RF_VECTOR
__interrupt void RF_ISR(void)
{  
  char intRFIM;
  //unsigned int i;
  int bufferFreeSpace;
  
  S1CON &= ~0x03; // Clear CPU interrupt flag
    
  intRFIM = RFIM;
  RFIF &= intRFIM;
  
  // If SFD detected ...
  while (RFIF & 0xD1) {
    if ( RFIF & 0x01 ) {
      RFIF &= ~0x01;
      timeCounterTxInhibit = 1; // Only 1 tick
      if (rfMessagePointerOut > rfMessagePointerIn) {
        bufferFreeSpace = rfMessagePointerOut - rfMessagePointerIn;
      } else {
        bufferFreeSpace = SIZE_OF_RF_BUFFER - rfMessagePointerIn +
                          rfMessagePointerOut - 1;
      }
      if (bufferFreeSpace >= 11) { // Check space for the smallest valid message
        rfMessageRXInProgress = 1;
        lastRSSI     = RSSI ^ 0x80;
        lastRSSIMode = rfMode;
        lastTimeStamp = timeCounter;
        rfOnTimer = (0*60 + 2) * TICKS_PER_SECOND;
        rfMessagePointerLen=rfMessagePointerIn;
        rfMessage[rfMessagePointerLen] = 0x00;
        rfMessagePointerIn = ((rfMessagePointerIn + 1) >= SIZE_OF_RF_BUFFER ) ?
                                                 0 : rfMessagePointerIn + 1 ;
        IEN0 |= 0x01;  // Enable RF TXRX Interrupt
      } else {
        // If we have no space, drop message
        RFST = RFST_SIDLE;
        //for (i=0; i<20000; i++) asm("nop");
        RFST = RFST_SRX;
      }
    } 
    
    // If TX underflow or RX overflow ...
    if ( RFIF & 0xC0 ) {
      /*if (RFIF & 0x80) rfRXOverflows++;
      else             rfTXOverflows++;*/
      RFST = RFST_SIDLE;
      //for (i=0; i<20000; i++) asm("nop");
      RFIF &= ~0xC0;
      rfLastData = 0xFF;
      rfMessage[rfMessagePointerLen] = 0x00;
      rfMessagePointerIn = rfMessagePointerLen;
      IEN0 &= ~0x01;  // Disable RF TXRX Interrupt
      RFTXRXIF = 0;
      TCON &= ~0x02;
      rfMessageRXInProgress = 0;
      resetRFBuffers ();
      RFST = RFST_SRX;
    } 
    
    // If packet was transmitted
    if ( RFIF & 0x10 ) {
      RFIF &= ~0x10;
      if (rfTXMode == 1) {
        rfMessagePointerTx=0;
        txTimes--;
        if (txTimes > 0) {
          RFST = RFST_SIDLE;
          TCON &= ~0x02;
          IEN0 |= 0x01;  // Enable RF TXRX Interrupt
          RFST = RFST_STX;
        } else {          
          IEN0 &= ~0x01;  // Disable RF TXRX Interrupt
          TCON &= ~0x02;
          PKTLEN = 0xFF;
          rfTXMode = 0;
          RFST = RFST_SIDLE;
          RFST = RFST_SRX;
          RFIM |=  0x51;  // Unmask RF Interrupt          
        }
      }
    }
  }
}

#pragma vector = RFTXRX_VECTOR
__interrupt void RFTXRX_ISR(void)
{
  int msgLen;
  //unsigned int i;
  int boundaryIndex;
  
  // Clear Interrupt Flags
  RFTXRXIF = 0;
  TCON &= ~0x02;
  
  if (rfTXMode == 0) {
    boundaryIndex = (rfMessagePointerOut == 0) ? SIZE_OF_RF_BUFFER   - 1
                                               : rfMessagePointerOut - 1;
    if (rfMessagePointerIn != boundaryIndex) {
      rfMessage[rfMessagePointerIn] = RFD;
      rfLastData = rfMessage[rfMessagePointerIn];
      rfMessagePointerIn = ((rfMessagePointerIn + 1) >= SIZE_OF_RF_BUFFER ) ?
                                                 0 : rfMessagePointerIn + 1 ;
    } else {
      // Abort message reception. Buffer overflow
      IEN0 &= ~0x01;  // Disable RF TXRX Interrupt
      rfMessage[rfMessagePointerLen]  = 0x00;
      rfMessagePointerIn = rfMessagePointerLen;
      RFST = RFST_SIDLE;
      //for (i=0; i<20000; i++) asm("nop");
      RFST = RFST_SRX;   
      rfMessageRXInProgress = 0;
    }
    
    if (rfMessagePointerIn < rfMessagePointerLen) {
      msgLen = SIZE_OF_RF_BUFFER - rfMessagePointerLen + rfMessagePointerIn - 1;
    } else {
      msgLen = rfMessagePointerIn - rfMessagePointerLen - 1;
    }
    
    if ((((rfLastData == 0) && (msgLen > 0)) || (msgLen >= 118) ||
         (rfMessagePointerIn == boundaryIndex)) &&
        (rfMessageRXInProgress == 1)){
      // End of the message
      IEN0 &= ~0x01;  // Disable RF TXRX Interrupt
      rfMessagePointerIn = ((rfMessagePointerIn-1) < 0) ? SIZE_OF_RF_BUFFER - 1 
                                                        : rfMessagePointerIn - 1;
      rfMessage[rfMessagePointerIn]  = 0x00;
      msgLen--;
      if (msgLen < 0) msgLen = 0;
      rfMessage[rfMessagePointerLen] = (msgLen & 0x0FF);
      rfMessageRXInProgress = 0;
      RFST = RFST_SIDLE;
      RFST = RFST_SRX;   
    }
    
  } else {
    RFD = rfMessageTx[rfMessagePointerTx];
    rfMessagePointerTx++;
  }
}

