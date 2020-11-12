#ifndef _TIMINGCONTROLLER_H_
#define _TIMINGCONTROLLER_H_

void initTimingTable (void);
void updateTimingTable (  char newSeqNum, unsigned char lastSeqNum, unsigned int time );
void timingSanityCheck (void);
unsigned int getTimeForNumSeq ( char seqNumQuery );
char timingCorrect(void);
void calculateFiveMinAdjustment(void);
void recalculateTiming(void);
void timeManagementTask (void);

#endif