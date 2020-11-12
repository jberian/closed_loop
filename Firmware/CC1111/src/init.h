#ifndef _INIT_H_
#define _INIT_H_

void configureIO (void);
void configureOsc (void);
void configureUART (void);
void initGlobals (void);
void resetHistoryLogs (void);
void configureTimeOutTimer (void);
void initFreqs (void);
void initResponseToInsulin (float hours, char offset);
void reset (void);
void initIOBRegisters (void);

#endif