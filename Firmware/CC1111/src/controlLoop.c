#include "globals.h"
#include "controlLoop.h"
#include "dataProcessing.h"
#include "pumpCommands.h"
#include "bleComms.h"
#include "init.h"
#include "crc_4b6b.h"

#define AVGDELTA_NUM_VALUES  3
#define AVGDELTA_NUM_VALUES_FACT 6.0
#define SAFE_MODE_INTERVALS 6

float avgdelta;
int targetBasal;
  
void controlLoopDataUpdate (void)
{
  int errorSgv,absErrorSgv,absSgvDif;
  float tempFloat,originalBasal;
  int   originalSensitivity;
  static char prevControlState;
  
  prevControlState = controlState;
  
  controlState = 0;
  if (basalVectorsInit == 0) {
    if ((pumpBasalsValidRanges > 0) &&
       (systemTimeSynced == 1) ){
      setIOBvectors (getCurrentBasal());
      safeMode = SAFE_MODE_INTERVALS;
      basalVectorsInit = 1;
    }
  }
  
  if (basalVectorsInit == 1) {
    
    originalBasal       = getCurrentBasal();
    originalSensitivity = getCurrentInsulinSensitivity();
    
    getCurrentTargets (&controlLoopThresLow, &controlLoopThresHigh);

    basalIOBOriginal = getDeliveredIOBBasal(originalBasal);
    bolusIOB         = getDeliveredIOBBolus();
    tbdBasalIOB      = getToBeDeliveredIOB(originalBasal);
    basalIOB         = getDeliveredIOBBasal(originalBasal);
    totalIOB         = tbdBasalIOB + basalIOB + bolusIOB;
    instantIOB       = getInstantIOB();

    basalUsed        = originalBasal;    
    sensitivityUsed  = originalSensitivity;
      
    if ((adjustSensitivityFlag & 0x03) != 0x00) {
      calculateAdjustedValues ();
    } else {
      measuredBasal = 0.0;
      measuredSensitivity = 0.0;
    }
  
    switch (controlLoopMode) {
    
    case TEMPBASAL_CONTROL_LOOP:
      controlState = 1;
    
      // Calculate current Basal
      if (((adjustSensitivityFlag & 0x01) == 0x01) &&
          (measuredBasalValid == 1) &&
          (measuredBasal == measuredBasal) &&
          (safeMode == 0)) {
        
        if (measuredBasal < -10.0) basalUsed = -10.0;
        else if (measuredBasal > 8.0*basalUsed) basalUsed *= 8.0;
        else basalUsed = measuredBasal;
        
	tbdBasalIOB      = getToBeDeliveredIOB(basalUsed);
        basalIOB         = getDeliveredIOBBasal(basalUsed);
        totalIOB         = tbdBasalIOB + basalIOB + bolusIOB;
      } 
      
      // Calculate current Sensitivity
      /*if (((adjustSensitivityFlag & 0x02) == 0x02) &&
          (measuredSensitivityValid == 1)) {
        if ((int)(measuredSensitivity) > sensitivityUsed) {
          sensitivityUsed = (int)(measuredSensitivity);
        }
      } */
      
      calculateExpectedSgv(&expectedSgv, &expectedSgvWoIOB, sensitivityUsed,
                           prevControlState);

      if ((expectedSgv > 0) && (systemTimeSynced > 0) && 
          (insulinSensValidRanges > 0) && (pumpBasalsValidRanges > 0) &&
          (pumpTargetsValidRanges > 0)) {

        // Target SGV calculation
        if (historySgv[0] > controlLoopThresHigh) {
          targetSgv = ((historySgv[0] + controlLoopThresLow)/2);
          if (targetSgv > controlLoopThresHigh) {
            targetSgv = controlLoopThresHigh;
          }
        } else {
          targetSgv = ((controlLoopThresHigh + controlLoopThresLow)/2);
        }
          
        // Target Due to higher basal
        if ((basalIOBOriginal < basalUsed) &&
            ((adjustSensitivityFlag & 0x04) != 0x00)) {
          targetBasal = (int)(basalIOBOriginal * (float)(sensitivityUsed));
          targetBasal += controlLoopThresLow;
          if (targetBasal > controlLoopMaxTarget) {
            targetBasal = controlLoopMaxTarget;
          }
          if (targetBasal < 0) targetBasal = 0;
        } else {
          targetBasal = 0;
        }
          
        // Choose the higher target (safety)
        targetSgv = (targetSgv > targetBasal) ? targetSgv : targetBasal;
        
        // Error calculation
        errorSgv   = expectedSgv - targetSgv;
        
        // Adjust sensitivity with error
        if ((adjustSensitivityFlag & 0x02) == 0x02) {
          //tempFloat = (float)sensitivityUsed;
          //if (errorSgv < 0) {
          //  tempFloat = tempFloat + (float)(errorSgv)/600.0;
          //} else {
          //  tempFloat = tempFloat - (float)(errorSgv)/600.0;
          //}
          tempFloat = (targetSgv < historySgv[0]) ? historySgv[0] - targetSgv 
                                                  : 2*(targetSgv - historySgv[0]);
          tempFloat = (float)sensitivityUsed * (1.0 - (0.5*tempFloat/200.0));
          if (tempFloat < 20.0) tempFloat = 20.0;
          sensitivityUsed = (int)tempFloat;
        }
        
        // First check if we need to suspend to prevent or correct a low
        if (((prevControlState != 2) &&
               ((expectedSgvWoIOB < controlLoopThresLow) ||
                (historySgv[0]    < controlLoopThresLow))) ||
            ((prevControlState == 2) &&
               ((expectedSgvWoIOB < ((controlLoopThresHigh - 
                             controlLoopThresLow)/4) + controlLoopThresLow) ||
                (historySgv[0]    < controlLoopThresLow)))) {    
          controlState = 2;
          setTempBasal (30,0.0);
          timeCounterBolusSnooze = 0;
          safeMode = SAFE_MODE_INTERVALS;
        } else {
          controlState = 3;

          // Update Safe Mode counter.
          if (safeMode > 0) {
            safeMode--;
            if (targetSgv < controlLoopThresHigh) {
              targetSgv = controlLoopThresHigh;
            }
          }
          
          // Cancel Bolus Snooze if you need more insulin during that time
          if (((expectedSgvWoIOB > controlLoopThresHigh) ||
              (historySgv[0]    > controlLoopThresHigh))  &&
              (errorSgv > 0)) {
            timeCounterBolusSnooze = 0;
          }

          // Only update Temp basal if the error is bigger than %10 the
          // difference between the current Sgv and the target
          absErrorSgv = (errorSgv < 0) ? -errorSgv : errorSgv;
          absSgvDif = (int)(historySgv[0]) - (int)(targetSgv);
          absSgvDif = (absSgvDif < 0) ? -absSgvDif : absSgvDif;
          if (absErrorSgv > (absSgvDif/10)) {
            controlState = 4;
          
            // Temporaty basal error calculation
            tempFloat  = (float)(sensitivityUsed);
            if ((tempFloat > 0.0) && (tempFloat < 1000.0)) { 
              tempFloat  = (float)(errorSgv) / tempFloat;
        
              // Apply new temporary basal
              tempFloat += tbdBasalIOB;
              tempFloat  = 2.0*((basalUsed/2.0) - getInstantIOBBolus30min() + tempFloat);
        
              // Apply temp basal
              if ((timeCounterBolusSnooze       == 0) || 
                  (timeCounterBolusSnoozeEnable == 0)) {
                controlState = 5;
                setTempBasal (30,tempFloat);  
              } else {
                controlState = 6;
                sendLoopReport ();
                cancelTempBasal ();
              }
            } else {
              sendLoopReport ();
            }
          } else {
            sendLoopReport ();
          }
        }
      } else {
        cancelTempBasal ();
        sendLoopReport ();
        if ((systemTimeSynced == 0) || (insulinSensValidRanges == 0) ||
            (pumpBasalsValidRanges == 0)) readPumpConfig();
      }
      break;
    
    default:
      sendLoopReport ();
      break;
    }    
  } else {
    sendLoopReport ();
  }

  
}

void calculateExpectedSgv (int *expectedSgvInt, 
                           int *expectedSgvWoIOBInt,
                           int sensitivity,
                           char prevControlState)
{
  char numValues;
  char aggressiveness;
  int steps;
  int stepsInc;
  int sgvDer;
  char i;
  float eventualBG;
  //float residualDelta;
  
  for (numValues = 0; ((historySgvValid[numValues] == 1) && 
                       (numValues < 16)); numValues ++);
  
  if (numValues > (AVGDELTA_NUM_VALUES+1)) {  
    eventualBG = (float)historySgv[0];
    
    avgdelta = 0.0;
    //residualDelta = 0.0;
    for (i=0; i<AVGDELTA_NUM_VALUES; i++) {
      avgdelta += (float)((AVGDELTA_NUM_VALUES-i)*(historySgv[i] - historySgv[i+1]));
      //residualDelta += ((float)(historySgv[i] - historySgv[i+1])) + 
      //  ((instantIOBHist[i+1] - (basalUsed/12.0))*((float)sensitivityUsed));
    }
    avgdelta = avgdelta / AVGDELTA_NUM_VALUES_FACT;
    
    if (controlLoopAggressiveness > 4) {
      // Embedded aggressiveness
      aggressiveness = controlLoopThresLow;
      while (aggressiveness > 9) aggressiveness -= 10;
      if (aggressiveness > 4) aggressiveness = 2;
    } else {
      aggressiveness = controlLoopAggressiveness;  
    }

    // Reduce aggressiveness while in Safe Mode
    if ((safeMode > 0) && (prevControlState > 2)) {
      if ((aggressiveness - safeMode) < 0) aggressiveness = 0;
      else aggressiveness = aggressiveness - safeMode;
    }
    
    steps = aggressiveness;
    
    // Higher aggressiveness when going down
    if (avgdelta < 0.0) steps++;
    
    //sgvDer = historySgv[0] - historySgv[1];
    //sgvDer = (int)(residualDelta);
    sgvDer = (int)(avgdelta);
    stepsInc = (sgvDer >= 0) ? sgvDer : -sgvDer;
    if (stepsInc >= 16) stepsInc = 16;
    steps += (stepsInc/(5-aggressiveness));
    /////if (steps <= 2) steps = 0;
    //for (i=0; i<steps; i++) if (i < 10) eventualBG += residualDelta;
    for (i=0; i<steps; i++) if (i < 10) eventualBG += avgdelta;
    
    if (eventualBG <= 0.0) *expectedSgvWoIOBInt = 1;
    else if (eventualBG > 400.0) *expectedSgvWoIOBInt = 400;
    else *expectedSgvWoIOBInt = (unsigned int)(eventualBG);
    
    eventualBG -= (totalIOB * (float)(sensitivity));
    if (eventualBG <= 0.0) *expectedSgvInt = 1;
    else if (eventualBG > 400.0) *expectedSgvInt = 400;
    else *expectedSgvInt = (unsigned int)(eventualBG);
    
  } else {
    *expectedSgvInt      = 0;
    *expectedSgvWoIOBInt = 0;
  }
}

void calculateAdjustedValues (void)
{
  int i,j,k;
  //float temp2ndDeriv;
  float tempSensFloat;
  float tempBasalFloat;
  int res;
  int currentSens;
  float eventualBasal;
    
  //if (1) {
    
   // measuredSensitivity = 0.0;
    measuredBasal       = 0.0;
    
    if ((historySgvValid[0] == 1) && (historySgvValid[1] == 1)) {
      for (i=5; i>0; i--) sgvDeriv[i]=sgvDeriv[i-1];
      for (i=5; i>0; i--) instantIOBHist[i]=instantIOBHist[i-1];
      sgvDeriv[0] = historySgv[1] - historySgv[0];
      instantIOBHist[0] = instantIOB;
      
      if (numHist < 4) numHist++;
      //else             numHist = 4;
      currentSens = getCurrentInsulinSensitivity();
    
      if (numHist >= 4) {
        for (i=3; i>0; i--) basalHist[i]=basalHist[i-1];
        for (i=3; i>0; i--) sensHist[i]=sensHist[i-1];
        if (basalHistNum < 4) basalHistNum++;
        
        //////////////////////////////////////////////////////////////////////
        // Estimated sensitivity calculation
        //////////////////////////////////////////////////////////////////////
        /*tempSensFloat = 0.0;
        temp2ndDeriv = 0.0;
        for (i=1; i<4; i++) {
          tempSensFloat += (instantIOBHist[i] - instantIOBHist[i-1]);
          temp2ndDeriv  += (sgvDeriv[i] - sgvDeriv[i-1]);
        }
        tempSensFloat = (tempSensFloat / temp2ndDeriv); 
        
        measuredSensitivity = tempSensFloat;
        if ((measuredSensitivity < (float)(currentSens)) ||
            (measuredSensitivity > (float)(currentSens)*8.0)) {
          measuredSensitivityValid = 0;
        } else {*/
          measuredSensitivityValid = 0;
        /*}*/
        
        //////////////////////////////////////////////////////////////////////
        // Estimated basal calculation
        //////////////////////////////////////////////////////////////////////
        
        // First check if there has been a change in the glucose vector
        j=0;
        for (i=1; i<3; i++) {
          k = sgvDeriv[i] - sgvDeriv[i-1];
          j = (k<0) ? j-k : j+k;
        }
        
        // If there was a change we can calculate the basal
        if (j != 0) {
          res = 1;
          tempBasalFloat = 0.0;
          for (i=0; i<3; i++) tempBasalFloat += instantIOBHist[i];
          tempBasalFloat = tempBasalFloat / 3.0;
          if (((adjustSensitivityFlag & 0x02) == 0x02) &&
              (measuredSensitivityValid == 1)) {
            tempSensFloat = measuredSensitivity;
          } else {
            tempSensFloat = (float)(currentSens);
          }
          tempBasalFloat = (tempBasalFloat+(avgdelta/tempSensFloat)); //*12.0;
        } else {
          tempSensFloat = 0.0;
          tempBasalFloat = 0.0;
          res = 0;
        }
        
        if (res == 0) {
          if (basalHist[1] != 0) {
            tempBasalFloat = getCurrentBasal();
            basalHist[0] = (int)(tempBasalFloat/0.025);
          } else {
            basalHist[0] = (int)(basalUsed/0.025);
          }
        } else {
          basalHist[0] = (int)(tempBasalFloat*12.0/0.025);
        }
        
        if (basalHistNum > 3) {
          eventualBasal = 0.0;
          for (i=3; i>0; i--) {
            eventualBasal += ((float)(basalHist[i-1])*0.025) - 
                             ((float)(basalHist[ i ])*0.025);
          }
          eventualBasal = eventualBasal/3.0;
          if (eventualBasal < 0.0) {
            measuredBasal = (float)(basalHist[0])*0.025 + 
                             3.0*eventualBasal;
          } else {
            measuredBasal = (float)(basalHist[0])*0.025;
          }
        } else {               
          measuredBasal = (float)(basalHist[0])*0.025;
        }
        
        measuredBasal = (float)(basalHist[0])*0.025;
        measuredBasalValid = 1;
      }    
    }   
  //}  
}

