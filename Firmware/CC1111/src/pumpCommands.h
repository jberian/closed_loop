#ifndef _PUMPCOMMANDS_H_
#define _PUMPCOMMANDS_H_

unsigned char numberOfPendingMsgs (void);
char addMsgToQueue (char *msg, unsigned int length, int times, char expectedAnswer,
                    unsigned long timeout, unsigned char retries);
void removeMsgFromQueue (char slot);
void removeMsgBeingProccessed (void);
void sendNextPumpCmd (void);
char pumpExpectedAnswer (void);
void pumpCmdTimeout (void);
void pumpCommandsCallback (  char timeout, char *message, unsigned int length );
void pumpWakeUp (void);
void pumpWakeUpRFTest (void);

void suspendResumeRequestCommand (void);
void suspendCommand (void);
void resumeCommand (void);
void setTempBasalRequestCommand (void);
void setTempBasalCommand (unsigned int minutes, float rate);
void cancelTempBasalCommand (void);
void bolusRequestCommand (void);
void bolusCommand (float insulinAmount);

void suspendPump (void);
void resumePump (void);
void setTempBasal (unsigned int minutes, float rate);
void cancelTempBasal (void);
void bolus (float insulinAmount);

void readPumpConfig (void);
void readStdBasalProfileRequestCommand (void);
void readSensorCalibrationFactor (void);
void readSensorCalibrationFactorRequestCommand (void);
void readCarbUnitsRequestCommand (void);
void readBGUnitsRequestCommand (void);
void readCarbRatiosRequestCommand (void);
void readInsulinSensitivitiesRequestCommand (void);
void readBGTargetsRequestCommand (void);
void readRTCRequestCommand (void);
void readPumpStateCommand (void);
void readPumpParamsRequestCommand (void);
void readReservoirRequestCommand (void);
void readBatteryRequestCommand (void);

void suspendJamming (void);

#endif