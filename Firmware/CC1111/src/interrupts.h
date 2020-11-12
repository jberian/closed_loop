#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

void enablePushButtonInt(void);
void enableTimerInt (void);
void resetTimerCounter (void);
void stopTimerInt (void);
void uart0StartTxForIsr(void);
void uart0StartRxForIsr(void);
void checkBleUartComms(void);
void addCharToBleTxBuffer (char byteToTransmit);
void enableRadioInt (void);

#endif
