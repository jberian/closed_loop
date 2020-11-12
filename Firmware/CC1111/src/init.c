#include "hal_types.h"
#include "hal_defs.h"
#include "hal_cc8051.h"
#include "ioCCxx10_bitdef.h"
#include "ioCC1110.h"
#include "constants.h"
#include "globals.h"
#include "init.h"
#include "freqManagement.h"

/***********************************************************************************
* LOCAL FUNCTIONS
*/
void configureIO (void)
{
    /* Set function of pin to general purpose I/O */
    P1SEL &= BIT0;
    P1SEL &= BIT1;
    P1SEL &= BIT5;
    P2SEL &= ~BIT0;

    /* Write value to pin. Note that this is a direct bit access.
     * Also note that the value is set before the pin is configured
     * as an output, as in some cases the value of the output needs
     * to be defined before the output is driven (e.g. to avoid
     * conflicts if the signal is shared with an other device)
     */
    P1_0 = 0;
    P1_1 = 0;
    GREEN_LED = 0;
    P1_5 = 0;

    /* Change direction to output */
    P2DIR |= BIT0;
    P1DIR |= BIT0;
    P1DIR |= BIT1;
    P1DIR |= BIT5;
}

void configureOsc (void)
{
  SLEEP &= ~OSC_PD_BIT;     // powering down all oscillators
  while(!XOSC_STABLE);      // waiting until the oscillator is stable
  asm("NOP");
  CLKCON &= 0xC0;
  CLKCON &= ~MAIN_OSC_BITS; // starting the Crystal Oscillator
  CLKCON |= 0x18;           // Tick speed = fref/8
  SLEEP |= OSC_PD_BIT;      // powering down the unused oscillator
  SLEEP &= ~0x80;           // Disable USB interface
}

void configureUART (void)
{

  /***************************************************************************
   * Setup I/O ports
   *
   * Port and pins used by USART0 operating in UART-mode are
   * RX     : P0_2
   * TX     : P0_3
   * CT/CTS : P0_4
   * RT/RTS : P0_5
   *
   * These pins can be set to function as peripheral I/O to be be used by UART0.
   * The TX pin on the transmitter must be connected to the RX pin on the receiver.
   * If enabling hardware flow control (U0UCR.FLOW = 1) the CT/CTS (Clear-To-Send)
   * on the transmitter must be connected to the RS/RTS (Ready-To-Send) pin on the
   * receiver.
   */

  // Configure USART0 for Alternative 1 => Port P0 (PERCFG.U0CFG = 0)
  // To avoid potential I/O conflict with USART1:
  // configure USART1 for Alternative 2 => Port P1 (PERCFG.U1CFG = 1)
  PERCFG = (PERCFG & ~PERCFG_U0CFG) | PERCFG_U1CFG;

  // Configure relevant Port P0 pins for peripheral function:
  // P0SEL.SELP0_2/3/4/5 = 1 => RX = P0_2, TX = P0_3, CT = P0_4, RT = P0_5
  P0SEL |= BIT5 | BIT4 | BIT3 | BIT2;

  /***************************************************************************
   * Configure UART
   *
   * The system clock source used is the HS XOSC at 48 MHz speed.
   */

  // Initialise bitrate = 9600 bbps (U0BAUD.BAUD_M = 163, U0GCR.BAUD_E = 7)
  U0BAUD = UART_BAUD_M;
  U0GCR = (U0GCR&~U0GCR_BAUD_E) | UART_BAUD_E;

  // Initialise UART protocol (start/stop bit, data bits, parity, etc.):

  // USART mode = UART (U0CSR.MODE = 1)
  U0CSR |= U0CSR_MODE;

  // Start bit level = low => Idle level = high  (U0UCR.START = 0)
  U0UCR &= ~U0UCR_START;

  // Stop bit level = high (U0UCR.STOP = 1)
  U0UCR |= U0UCR_STOP;

  // Number of stop bits = 1 (U0UCR.SPB = 0)
  U0UCR &= ~U0UCR_SPB;

  // Parity = disabled (U0UCR.PARITY = 0)
  U0UCR &= ~U0UCR_PARITY;

  // 9-bit data enable = 8 bits transfer (U0UCR.BIT9 = 0)
  U0UCR &= ~U0UCR_BIT9;

  // Level of bit 9 = 0 (U0UCR.D9 = 0), used when U0UCR.BIT9 = 1
  // Level of bit 9 = 1 (U0UCR.D9 = 1), used when U0UCR.BIT9 = 1
  // Parity = Even (U0UCR.D9 = 0), used when U0UCR.PARITY = 1
  // Parity = Odd (U0UCR.D9 = 1), used when U0UCR.PARITY = 1
  U0UCR &= ~U0UCR_D9;

  // Flow control = disabled (U0UCR.FLOW = 0)
  U0UCR &= ~U0UCR_FLOW;

  // Bit order = LSB first (U0GCR.ORDER = 0)
  U0GCR &= ~U0GCR_ORDER;
}

void initGlobals (void)
{
  int i,j;
  
  resetRequested = 0;
  bleTxIndexIn   = 0;
  bleTxIndexOut  = 0;
  bleRxIndexIn   = 0;
  bleRxIndexOut  = 0;
  bleTxing       = 0;
  bleRxFlag      = 0;
  timeCounter  = _MAX_TIMECOUNTER_VAL_;
  timeCounterOn  = 0;
  timeCounterOff = 0;
  timeCounterHoldProcess = 0;
  timeCounterBolusSnooze = 0;
  timeCounterBolusSnoozeEnable = 0;
  timeCounterPumpAwake = 0;
  timeCounterTxInhibit = 0;
  bleCommsWatchdogTimer = 0;
  mySentryFlag = 0;
  rfFrequencyMode = 0;
  timeUpdatedFlag = 0;
  sendFlag = 0;
  queryPumpFlag = 0;
  rfState[0] = 0;
  rfState[1] = 0;
  syncMode = 0;
  txTimer = 0;
  txing = 0;
  rfTXMode = 0;
  startRFTxFlag = 0;
  adjustSensitivityFlag = 0;
  lookForMySentryTimer = 0;
  lookForMinilinkTimer = 0;
  expectedSeqNum = 0;
  correctionTimeStamp = 0;
  glucoseDataSource = 0;
  tempBasalActiveRate = 0.0;
  tempBasalTimeLeft = 0;
  //bgUnits = BG_UNITS_NONE;
  //carbUnits = CARB_UNITS_NONE;

  lastMinilinkSeqNum = 0x80;
  for (i=0; i<3; i++) {
   minilinkID[i]     = 0x00;
   pumpID[i]         = 0x00;
   glucometerID[i]   = 0x00;
  }
  
  timingTableFrozen  = 0;
  timingTableCorrect = 0;
  for (i=0; i<8; i++) {
    missesTable[i] = 0x03;
    for (j=0; j<2; j++) {
      timingTable[i][j] = 0;
    }
  }
  
  /*for (i=0; i<MAX_NUM_BASALS; i++) {     
    pumpBasalsHour   [ i ] = 0;
    pumpBasalsMinute [ i ] = 0;
    pumpBasalsAmount [ i ] = 0;
  }*/
  pumpBasalsValidRanges = 0;
   
  /*for (i=0; i<MAX_NUM_INSULIN_SENS; i++) {     
    insulinSensHour   [ i ] = 0;
    insulinSensMinute [ i ] = 0;
    insulinSensAmount [ i ] = 0;
  }*/
  insulinSensValidRanges = 0;

  pumpTargetsValidRanges = 0;
  
  resetHistoryLogs();
  
  initFreqs();
  
  systemTimeHour         = 0;
  systemTimeMinute       = 0;
  systemTimeSecond       = 0;
  systemTimeSecondTimer  = 0;
  systemTimeSynced       = 0;
  
  //controlLoopMode = NO_CONTROL_LOOP;
  controlLoopAggressiveness = 2;
  controlLoopMaxTarget = 130;

  durationIOB = 3;
  //maxBolus    = 0;
  maxBasal    = 0;

  maxAbsorptionRate = 0.0;
  maxAbsorptionRateEnable = 0;

  initIOBRegisters();
  initResponseToInsulin((float)(durationIOB),30/5);
  //initResponseToInsulin(3.0,15/5);
  
  for (i=0; i<6; i++) {
    instantIOBHist[i] = 0.0;
    sgvDeriv[i] = 0;
    sensHist[i] = 0;
    basalHist[i] = 0;
  }
  numRes = 0;
  numHist = 0;
  basalHistNum = 0;
  
  measuredSensitivityValid  = 0;
  measuredBasalValid        = 0;
  
}

void initIOBRegisters (void)
{
  int i;
  
  basalVectorsInit = 0;
  for (i=0; i<MAX_NUM_IOB_REGISTERS; i++) {
    basalIOBvector    [i] = 0;
    bolusIOBvector    [i] = 0;
    responseToInsulin [i] = 0;
  }
  for (i=0; i<MAX_NUM_FUTURE_IOBS; i++) {
    basalIOBfuture    [i] = 0;
  }
  
  iobAccumBasal = 0.0;
  iobAccumBolus = 0.0;
}

void initResponseToInsulin (float hours, char offset)
{
  float idx;
  float tempValue;
  int idxInit, idxEnd;
  int i;
  int32 normSum;
  //char offset;
  
  //offset = 2; // 10 min / 5 min = 2
  
  if (hours > 8.0) hours = 8.0;
  else if (hours < 0.0) hours = 0.0;
  
  // Curve calculation
  // 
  //    Humalog Response
  //    https://dailymed.nlm.nih.gov/dailymed/archives/image.cfm?archiveid=41365&type=img&name=humalog-f001-v1.jpg
  //
  idxInit = 0;
  idx = (0.0*hours/3.0)/5.0 + offset;
  idxEnd  = (int)idx;
  for (i=idxInit;i<=idxEnd;i++) {
    if ((i>=0) && (i<MAX_NUM_IOB_REGISTERS)) {
      tempValue = 0.0*((float)(i) / (float)(idxEnd));
      responseToInsulin[i] = (unsigned int) (tempValue*16380.0);
    }
  }
  idxInit = idxEnd;
  idx = (25.0*hours/3.0)/5.0  + offset;
  idxEnd  = (int)idx;
  for (i=idxInit;i<=idxEnd;i++) {
    if ((i>=0) && (i<MAX_NUM_IOB_REGISTERS)) {
      tempValue = 0.477*((float)(i) / (float)(idxEnd));
      responseToInsulin[i] = (unsigned int) (tempValue*16380.0);
    }
  }
  idxInit = idxEnd;
  idx = (45.0*hours/3.0)/5.0  + offset;
  idxEnd  = (int)idx;
  for (i=idxInit;i<=idxEnd;i++) {
    if ((i>=0) && (i<MAX_NUM_IOB_REGISTERS)) {
      tempValue = 0.477 + (0.9-0.477)*((float)(i-idxInit)/(float)(idxEnd-idxInit));
      responseToInsulin[i] = (unsigned int) (tempValue*16380.0);
    }
  }
  idxInit = idxEnd;
  idx = (60.0*hours/3.0)/5.0  + offset;
  idxEnd  = (int)idx;
  for (i=idxInit;i<=idxEnd;i++) {
    if ((i>=0) && (i<MAX_NUM_IOB_REGISTERS)) {
      tempValue = 0.9 + (0.99-0.9)*((float)(i-idxInit)/(float)(idxEnd-idxInit));
      responseToInsulin[i] = (unsigned int) (tempValue*16380.0);
    }
  }  
  idxInit = idxEnd;
  idx = (150.0*hours/3.0)/5.0 + offset;
  idxEnd  = (int)idx;
  for (i=idxInit;i<=idxEnd;i++) {
    if ((i>=0) && (i<MAX_NUM_IOB_REGISTERS)) {
      tempValue = 0.99 - (0.99-0.5)*((float)(i-idxInit)/(float)(idxEnd-idxInit));
      responseToInsulin[i] = (unsigned int) (tempValue*16380.0);
    }
  }  
  idxInit = idxEnd;
  idx = (210.0*hours/3.0)/5.0 + offset;
  idxEnd  = (int)idx;
  for (i=idxInit;i<=idxEnd;i++) {
    if ((i>=0) && (i<MAX_NUM_IOB_REGISTERS)) {
      tempValue = 0.5 - (0.5-0.2)*((float)(i-idxInit)/(float)(idxEnd-idxInit));
      responseToInsulin[i] = (unsigned int) (tempValue*16380.0);
    }
  } 
  idxInit = idxEnd;
  idx = (420.0*hours/3.0)/5.0 + offset;
  idxEnd  = (int)idx;
  for (i=idxInit;i<=idxEnd;i++) {
    if ((i>=0) && (i<MAX_NUM_IOB_REGISTERS)) {
      tempValue = 0.2 - (0.2-0.05)*((float)(i-idxInit)/(float)(idxEnd-idxInit));
      responseToInsulin[i] = (unsigned int) (tempValue*16380.0);
    }
  } 
  idxInit = idxEnd;
  idx = (440.0*hours/3.0)/5.0 + offset;
  idxEnd  = (int)idx;
  for (i=idxInit;i<=idxEnd;i++) {
    if ((i>=0) && (i<MAX_NUM_IOB_REGISTERS)) {
      tempValue = 0.05 - (0.05-0.01)*((float)(i-idxInit)/(float)(idxEnd-idxInit));
      responseToInsulin[i] = (unsigned int) (tempValue*16380.0);
    }
  }   
  idxInit = idxEnd;
  idx = (480.0*hours/3.0)/5.0 + offset;
  idxEnd  = (int)idx;
  for (i=idxInit;i<=idxEnd;i++) {
    if ((i>=0) && (i<MAX_NUM_IOB_REGISTERS)) {
      tempValue = 0.01 - 0.01*((float)(i-idxInit)/(float)(idxEnd-idxInit));
      responseToInsulin[i] = (unsigned int) (tempValue*16380.0);
    }
  }   
  
  // Normalization
  normSum = 0;
  for (i=0; i<MAX_NUM_IOB_REGISTERS; i++) normSum += responseToInsulin[i];
  tempValue = (float)(normSum) / 16382.0;
  for (i=0; i<MAX_NUM_IOB_REGISTERS; i++) {
    responseToInsulin[i] = (unsigned int)((float)(responseToInsulin[i])/tempValue); 
  }
}

void resetHistoryLogs (void)
{
  char i;
  
  for (i=0; i<16; i++) {
    historySgv         [i] = 0;
    historyRawSgv      [i] = 0;
    historyRaw         [i] = 0;
    historySgvValid    [i] = 0;
    historyRawSgvValid [i] = 0;
    historyRawValid    [i] = 0;
  }
}

void initFreqs (void)
{
  int i = 0;
  if (rfFrequencyMode == 0 ) {
    genericFreq[2]    = 0x24;
    genericFreq[1]    = 0x2E;             // f = 868.4 MHz
    genericFreq[0]    = 0xC0; //0x38
    genericBW         = _WIDE_BANDWIDTH_;
  } else {
    genericFreq[2]    = 0x26;
    genericFreq[1]    = 0x30;             // f = 916.5 MHz
    genericFreq[0]    = 0x70;
    genericBW         = _WIDE_BANDWIDTH_; 
  }
  
  minilinkBW          = genericBW;
  pumpBW              = genericBW;
  glucometerBW        = genericBW;
  
  for (i=0; i<32; i++) {
    minilinkRSSI   [ i ] = 0x00 ;
    pumpRSSI       [ i ] = 0x00 ;
    glucometerRSSI [ i ] = 0x00 ;
  }
  
  minilinkRSSI   [ 32 ] = 0x01 ;
  pumpRSSI       [ 32 ] = 0x01 ;
  glucometerRSSI [ 32 ] = 0x01 ;
  
  lastMinilinkChannel   = 0 ;
  lastPumpChannel       = 0 ;
  lastGlucometerChannel = 0 ;
  
  bestMinilinkFreqFound   = 0 ;
  bestPumpFreqFound       = 0 ;
  bestGlucometerFreqFound = 0 ;
}

void reset (void)
{
 // (*((void (* __code *)(void))0))(); // Reset
 EA = 0;  // Disable global interrupt 
 WDCTL = 0xA0;
 WDCTL = 0x50; // Clear timer
 WDCTL = 0x8B; // Enable watchdog to force reset
}
