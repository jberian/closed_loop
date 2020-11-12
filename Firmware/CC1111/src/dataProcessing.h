#ifndef _DATAPROCESSING_H_
#define _DATAPROCESSING_H_

void processMessage (char *message, unsigned int length, char dataErr);
void queryPumpData (void);
void processInfo (void);
void updateHistoryData (void);
void updateIOBvectors (void);
void setIOBvectors (float rate);
float getDeliveredIOBBasal (float basal);
float getDeliveredIOBBolus (void);
float getDeliveredIOB (float basal);
float getTotalBasalIOB (float basal);
float getToBeDeliveredIOB (float basal);
float getTotalIOB (float basal);
float getInstantIOB (void);
float getInstantIOBBolus30min (void);
void addIOBForTempBasal(float rate, unsigned int Time);
void cancelIOBForCurrentTempBasal(float basal);
float getISIGfromRAW (long raw, char adjValue);
void respondToDeviceSearch (char *message, unsigned int length, char dataErr);
float getBasalForTime (unsigned char hour, unsigned char minute);
float getCurrentBasal (void);
int getInsulinSensitivityForTime (unsigned char hour, unsigned char minute);
int getCurrentInsulinSensitivity (void);
char getTargetsForTime (unsigned char hour, unsigned char minute,
                        int *min, int *max);
char getCurrentTargets (int *min, int *max);
unsigned int calculateBolusSnoozeTime (void);

#endif
