#ifndef _BLECOMMS_H_
#define _BLECOMMS_H_


char initHM10 (void);
void checkBLERx (void);
void receiveBLEMessage (char *message, unsigned int messageLen);
void composeInfoUpdateMessage (void);
void composeGlucometerMessage (void);
void composeActionNotificationMessage (char action);
void sendLoopReport (void);

#endif