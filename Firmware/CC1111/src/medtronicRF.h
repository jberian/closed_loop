#ifndef _MEDTRONICRF_H_
#define _MEDTRONICRF_H_

void configureMedtronicRFMode (char mode);
void sendMedtronicMessage (char *message, unsigned int length, int times);
char receiveMedtronicMessage (char *message, unsigned int *length);
void discardRFMessage (void);
void checkMedtronicRF (void);
void resetRFBuffers (void);

#endif