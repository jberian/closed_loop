#ifndef _FREQMANAGEMENT_H_
#define _FREQMANAGEMENT_H_

void adjustFrequencies (char *message, unsigned int length, char dataErr);
char getBestMinilinkMode (void);
char getBestPumpMode (void);
char getBestGlucometerMode (void);
char tryNextMinilinkMode (void);
char tryNextPumpMode (void);
char tryNextGlucometerMode (void);
void updateNextMinilinkMode (void);
void updateNextPumpMode (void);
void updateNextGlucometerMode (void);
void checkIfBestMinilinkFreqWasFound (void);
void checkIfBestPumpFreqWasFound (void);
void checkIfBestGlucometerFreqWasFound (void);
void adjustPumpFrequency (void);

#endif