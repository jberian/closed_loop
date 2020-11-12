#include "crc_4b6b.h"
#include "globals.h"

// Globals
static char   __xdata lastValidMessage[74];
static char   __xdata recoveredMessage[74];
static unsigned int lastValidMessageLength;
static unsigned int recoveredMessageLength;

void smartRecovery (char *message, unsigned int *length, char *error)
{
  unsigned int i;
  char calcCRC;
  short calcCRC16;

  // If the message is not correct ...
  if (*error != 0) {
    // Move message to test buffer
    if (*length > 74) *length=74;
    for (i=0; i<(*length); i++) recoveredMessage[i] = message[i];
    recoveredMessageLength = *length;

    // The first nibble should be 'A'
    recoveredMessage[0] = (recoveredMessage[0] & 0x0F) | 0xA0;

    // ... we will try first adjusting the length to the closest one:

    // Pump query or glucometer reading
    if (((recoveredMessageLength) >= 7) && 
         (recoveredMessageLength < 34)) {
      recoveredMessageLength = 7;
      calcCRC = crc8(recoveredMessage,recoveredMessageLength-1);
      if (calcCRC == recoveredMessage[recoveredMessageLength-1]) {
	*error = 0;
      } 

    // Minilink message
    } else if (((recoveredMessageLength) >= 34) && 
                (recoveredMessageLength < 41)) {
      recoveredMessageLength = 34;
      calcCRC16 = crc16(recoveredMessage,recoveredMessageLength-2);
      if (((char)( calcCRC16       & 0x00FF) == 
                       recoveredMessage[recoveredMessageLength-1]) && 
	  ((char)((calcCRC16 >> 8) & 0x00FF) == 
                       recoveredMessage[recoveredMessageLength-2])) {
	*error = 0;
      } else {
	crc16Init();
	// If this didn't work, let's try reconstructing the message
        // using the previous minilink message stored.
	if (lastValidMessageLength > 0) {
	  for (i=0; i<8; i++) recoveredMessage[i] = lastValidMessage[i];
	  recoveredMessage[8] = ((lastValidMessage[8] & 0x70) + 0x10) & 0x70;
	  for (i=0; i<2; i++) recoveredMessage[11+i] = lastValidMessage[9+i];
	  recoveredMessage[13] = 0x00;
	  recoveredMessage[15] = lastValidMessage[14];
	  recoveredMessage[16] = lastValidMessage[16];
	  for (i=0; i<2; i++)  recoveredMessage[17+i] = lastValidMessage[11+i];
	  for (i=0; i<12; i++) recoveredMessage[19+i] = lastValidMessage[17+i];
	  recoveredMessage[31] = 0x00;
	  calcCRC16 = crc16(recoveredMessage,recoveredMessageLength-2);
	  if (((char)( calcCRC16       & 0x00FF) == 
                            recoveredMessage[recoveredMessageLength-1]) && 
	      ((char)((calcCRC16 >> 8) & 0x00FF) == 
                            recoveredMessage[recoveredMessageLength-2])) {
	    *error = 0;
	  }
	  if (*error != 0) {
	    // If not, check if it was the second message from the minilink
	    recoveredMessage[8] = recoveredMessage[8] | 0x01;
	    calcCRC16 = crc16(recoveredMessage,recoveredMessageLength-2);
	    if (((char)( calcCRC16       & 0x00FF) == 
                            recoveredMessage[recoveredMessageLength-1]) && 
		((char)((calcCRC16 >> 8) & 0x00FF) == 
                            recoveredMessage[recoveredMessageLength-2])) {
	      *error = 0;
	    }
	  }
	}
      }
    // MySentry  message
    } else if ((recoveredMessageLength >= 42) && 
               (recoveredMessageLength < 71)) {
      recoveredMessageLength = 42;
      calcCRC = crc8(recoveredMessage,recoveredMessageLength-1);
      if (calcCRC == recoveredMessage[recoveredMessageLength-1]) {
	*error = 0;
      }
      
    // Pump data response
    } else if ((recoveredMessageLength >= 71)) {
      recoveredMessageLength = 71;
      calcCRC = crc8(recoveredMessage,recoveredMessageLength-1);
      if (calcCRC == recoveredMessage[recoveredMessageLength-1]) {
	*error = 0;
      }
    }

    // If we recovered the message, move the contents back to
    // the original buffer
    if (*error != 1) {
      for (i=0; i<recoveredMessageLength; i++) 
                 message[i] = recoveredMessage[i];
      *length = recoveredMessageLength;
    }
  }

  // If the message is valid at this point....
  if (*error != 1) {
    // ... and it's a minilink message from our transmitter ...
    if (  ((message[0] == 0xAA) || (message[0] == 0xAB)) &&
	  ((message[2] == minilinkID[0]) &&
            (message[3] == minilinkID[1]) &&
            (message[4] == minilinkID[2])) ) {
      // ... we update the last valid message for the next time.
      lastValidMessageLength = *length;
      for (i=0; i<(*length); i++) lastValidMessage[i] = message[i];
    }
          
  }

}
