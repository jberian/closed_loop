from simglucose.simulation.sim_engine import SimObj, batch_sim
from simglucose.controller.base import Controller, Action
from simglucose.simulation.env import T1DSimEnv
from simglucose.sensor.cgm import CGMSensor
from simglucose.actuator.pump import InsulinPump
from simglucose.patient.t1dpatient import T1DPatient
from simglucose.simulation.scenario import CustomScenario
from simglucose.analysis.report import report
from datetime import timedelta
from datetime import datetime
import pkg_resources
import numpy as np
import pandas as pd
import copy
import matplotlib.pyplot as plt

CONTROL_QUEST = pkg_resources.resource_filename(
    'simglucose', 'params/Quest.csv')
PATIENT_PARA_FILE = pkg_resources.resource_filename(
    'simglucose', 'params/vpatient_params.csv')
AP_PARA_FILE = 'AP_params.csv'

class MyController(Controller):
    def calculateInsulinResponse(self, DIA, insulinType):
        self.DIA = DIA
        self.insulinResponse = np.zeros(12*24,np.float64,'C')
        self.timeOffset = 0
        if (insulinType == 'humalog'):
            idxInit = 0
            if (idxInit >= 12*24):
                idxInit = 12*24 - 1
            idx = np.int(np.round((0.0*self.DIA/3.0)/5.0 + self.timeOffset))
            idxEnd  = idx
            if (idxEnd >= 12*24):
                idxEnd = 12*24 - 1
            for i in range(idxInit,idxEnd):
                self.insulinResponse[i] = 0.0*(i/idxEnd)

            idxInit = idxEnd
            idx = np.int(np.round((25.0*self.DIA/3.0)/5.0 + self.timeOffset))
            idxEnd  = idx
            if (idxEnd >= 12*24):
                idxEnd = 12*24 - 1
            for i in range(idxInit,idxEnd):
                self.insulinResponse[i] = 0.477*(i/idxEnd)

            idxInit = idxEnd
            idx = np.int(np.round((45.0*self.DIA/3.0)/5.0 + self.timeOffset))
            idxEnd  = idx
            if (idxEnd >= 12*24):

                idxEnd = 12*24 - 1
            for i in range(idxInit,idxEnd):
                self.insulinResponse[i] = 0.477 + (0.9-0.477)*((i-idxInit)/(idxEnd-idxInit))

            idxInit = idxEnd
            idx = np.int(np.round((60.0*self.DIA/3.0)/5.0 + self.timeOffset))
            idxEnd  = idx
            if (idxEnd >= 12*24):
                idxEnd = 12*24 - 1
            for i in range(idxInit,idxEnd):
                self.insulinResponse[i] = 0.9 + (0.99-0.9)*((i-idxInit)/(idxEnd-idxInit))

            idxInit = idxEnd
            idx = np.int(np.round((150.0*self.DIA/3.0)/5.0 + self.timeOffset))
            idxEnd  = idx
            if (idxEnd >= 12*24):
                idxEnd = 12*24 - 1
            for i in range(idxInit,idxEnd):
                self.insulinResponse[i] = 0.99 - (0.99-0.5)*((i-idxInit)/(idxEnd-idxInit))

            idxInit = idxEnd
            idx = np.int(np.round((210.0*self.DIA/3.0)/5.0 + self.timeOffset))
            idxEnd  = idx
            if (idxEnd >= 12*24):
                idxEnd = 12*24 - 1
            for i in range(idxInit,idxEnd):
                self.insulinResponse[i] = 0.5 - (0.5-0.2)*((i-idxInit)/(idxEnd-idxInit))

            idxInit = idxEnd
            idx = np.int(np.round((420.0*self.DIA/3.0)/5.0 + self.timeOffset))
            idxEnd  = idx
            if (idxEnd >= 12*24):
                idxEnd = 12*24 - 1
            for i in range(idxInit,idxEnd):
                self.insulinResponse[i] = 0.2 - (0.2-0.05)*((i-idxInit)/(idxEnd-idxInit))

            idxInit = idxEnd
            idx = np.int(np.round((440.0*self.DIA/3.0)/5.0 + self.timeOffset))
            idxEnd  = idx
            if (idxEnd >= 12*24):
                idxEnd = 12*24 - 1
            for i in range(idxInit,idxEnd):
                self.insulinResponse[i] = 0.05 - (0.05-0.01)*((i-idxInit)/(idxEnd-idxInit))

            idxInit = idxEnd
            idx = np.int(np.round((480.0*self.DIA/3.0)/5.0 + self.timeOffset))
            idxEnd  = idx
            if (idxEnd >= 12*24):
                idxEnd = 12*24 - 1
            for i in range(idxInit,idxEnd):
                self.insulinResponse[i] = 0.01 - 0.01*((i-idxInit)/(idxEnd-idxInit))

            normSum = np.sum(self.insulinResponse)
            for i in range(0,(12*24)):
                self.insulinResponse[i] = self.insulinResponse[i]/normSum

        else:
            for i in range(0,(12*24)):
                self.insulinResponse[i] = 1.0/(self.DIA*12)
            normSum = np.sum(self.insulinResponse)
            for i in range(0,(12*24)):
                self.insulinResponse[i] = self.insulinResponse[i]/normSum

    def __init__(self, init_state, batch_sim):
        self.init_state = init_state
        self.state = init_state
        self.quest = pd.read_csv(CONTROL_QUEST)
        self.patient_params = pd.read_csv(
            PATIENT_PARA_FILE)
        self.AP_params = pd.read_csv(AP_PARA_FILE)

        self.prevMeal = 0
        self.bolusSnooze = 0
        self.simulationTime = 0
        self.cgmHistory = np.zeros(12*24,np.int16,'C')
        self.basalHistory = np.ones(12*24+6,np.float64,'C')
        self.basalNeedsHistory = np.zeros(12*24+6,np.float64,'C')
        self.cgmDeriv = np.zeros(12*48,np.float64,'C')
        self.bolusHistory = np.zeros(12*24,np.float64,'C')
        self.insulinResponse = np.zeros(12*24,np.float64,'C')
        self.insulinOnBoardCurve = np.zeros(12*24,np.float64,'C')
        self.DIA = 6
        self.insulinType = 'humalog'
        self.calculateInsulinResponse(self.DIA,self.insulinType)
        self.correctionBolusAfterTwoHours = False
        if (batch_sim == 2) or (batch_sim == 4):
            self.mealSchedule = dict([(7*60, 56), (13*60, 37), (17*60, 9), (20*60, 38), (23*60, 0)])
        else:
            self.mealSchedule = dict([(7*60,  0), (13*60,  0), (17*60, 0), (20*60,  0), (23*60, 0)])
        if (batch_sim == 3) or (batch_sim == 4):
            self.simBlockage = True
        else:
            self.simBlockage = False

    def policy(self, observation, reward, done, **info):
        '''
        Every controller must have this implementation!
        ----
        Inputs:
        observation - a namedtuple defined in simglucose.simulation.env. For
                      now, it only has one entry: blood glucose level measured
                      by CGM sensor.
        reward      - current reward returned by environment
        done        - True, game over. False, game continues
        info        - additional information as key word arguments,
                      simglucose.simulation.env.T1DSimEnv returns patient_name
                      and sample_time
        ----
        Output:
        action - a namedtuple defined at the beginning of this file. The
                 controller action contains two entries: basal, bolus
        '''

        pname = info.get('patient_name')
        meal = info.get('meal')

        ap = self.AP_params[self.AP_params.Name.str.match(pname)]
        insulinType = ap.InsulinType.values[0]
        dia = ap.DIA.values[0]
        maxBasal = ap.maxBasal.values[0]
        ISFtarget = ap.ISFtarget.values[0]
        ISFratio = ap.ISFratio.values[0]
        ISFmin = ap.ISFmin.values[0]
        targetMax = ap.TargetMax.values[0]
        targetMin = ap.TargetMin.values[0]
        carbRatio = ap.CR.values[0]
        preBolusingMinutes = ap.PreBolusing.values[0]
        preBolusingEnable = ap.PreBolusEnable.values[0]
        autoBasalEnable = ap.AutoBasalEnable.values[0]
        APEnable = ap.Enable.values[0]
        scen_dict = self.mealSchedule

        if (self.simulationTime == 0):
            self.basalHistory = np.ones(12*24+6,np.float64,'C')*(ap.Basal.values[0]+0.2)/12
            self.basalNeedsHistory = np.ones(12*24+6,np.float64,'C')*ap.Basal.values[0]

        self.state = observation
        self.simulationTime += info.get('sample_time')

        ''' CGM data derivative '''
        self.cgmDeriv = np.roll(self.cgmDeriv,-1)
        self.cgmDeriv[-1] = observation.CGM - self.cgmHistory[-1]

        ''' IOB calculation '''
        if (self.DIA != dia) or (self.insulinType != insulinType):
            self.calculateInsulinResponse(dia,insulinType)
        basaliIOB = np.convolve(self.insulinResponse,self.basalHistory,'full')
        bolusiIOB = np.convolve(self.insulinResponse,self.bolusHistory,'full')
        futureBasalIOB = basaliIOB[24*12:24*12+6] - np.ones(6,np.float64,'C')*(self.basalNeedsHistory[-1])/12
        futureBolusIOB = bolusiIOB[24*12:-1]
        basalIOB = np.sum(futureBasalIOB)
        bolusIOB = np.sum(futureBolusIOB)
        totalIOB = basalIOB + bolusIOB

        ''' Update CGM history '''
        self.cgmHistory = np.roll(self.cgmHistory,-1)
        self.cgmHistory[-1] = observation.CGM
        deviation = sum(self.cgmHistory[-3:-1]-self.cgmHistory[-4:-2])
        eventualCGMWoIOB = observation.CGM + deviation*3

        ''' Target calculation '''
        targetDesired = (targetMax + targetMin)/2.0
        targetDynamic = (observation.CGM + targetMin)/2.0
        targetFinal = targetMax
        if (observation.CGM >= targetMin):
            if (targetDynamic <= targetDesired):
                targetFinal = targetDesired
            else:
                if (targetDynamic >= targetMax):
                    targetFinal = targetMax
                else:
                    targetFinal = targetDynamic

        ''' ISF dynamic adjustment '''
        ISFfinal = ISFtarget - ISFratio*(np.abs(eventualCGMWoIOB - targetDesired))
        if (ISFfinal < ISFmin):
            ISFfinal = ISFmin

        ''' Eventual CGM value '''
        eventualCGM = eventualCGMWoIOB - totalIOB*ISFfinal

        ''' Error calculation with adjusted ISF and dynamic target '''
        error = eventualCGM - targetFinal

        ''' Basal needs estimation '''
        if (autoBasalEnable == True):
            lastCGMDeriv = self.cgmDeriv[-6:-1]
            lastiIOB = basaliIOB[24*12-5:24*12]

            thereWasAChange = sum(abs(lastCGMDeriv[0:-2] - lastCGMDeriv[1:-1]))
            if (self.simulationTime < info.get('sample_time')*7):
                thereWasAChange = 0
            if (thereWasAChange > 0):
                basalNeeds = (np.mean(lastiIOB)+(np.mean(lastCGMDeriv)/ISFfinal))*12
            else:
                basalNeeds = self.basalNeedsHistory[-1]

            self.basalNeedsHistory = np.roll(self.basalNeedsHistory,-1)
            self.basalNeedsHistory[-1] = basalNeeds

            if (thereWasAChange > 0):
                needsDeriv = np.mean(self.basalNeedsHistory[-3:-1] - self.basalNeedsHistory[-4:-2])
                if (needsDeriv < 0):
                    basalNeeds = basalNeeds + 3*needsDeriv
        else:
            basalNeeds = ap.Basal.values[0]

        ''' Correction action through temporary basal '''
        actionBasal = (error/ISFfinal)
        if (actionBasal > (maxBasal - basalNeeds)):
            actionBasal = (maxBasal - basalNeeds)
        if (actionBasal < -basalNeeds):
            actionBasal = -basalNeeds

        ''' Meal detection and Meal bolusing '''
        if (preBolusingEnable == True):
            if ((self.simulationTime + preBolusingMinutes) in scen_dict):
                internalMeal = scen_dict[(self.simulationTime + preBolusingMinutes)]
            else:
                internalMeal = 0
        else:
            internalMeal = meal

        eventualError = eventualCGMWoIOB - targetFinal
        if (internalMeal > self.prevMeal):
            if (eventualError < 0):
                bolusCompensation = (1 + eventualError/80)
                if (bolusCompensation < 0):
                    bolusCompensation = 0
            else:
                bolusCompensation = 1

            actionBolus = bolusCompensation*internalMeal/carbRatio

        else:
            actionBolus = 0.0
            if self.correctionBolusAfterTwoHours:
                if (self.simulationTime - 2*60) in scen_dict:
                    if eventualError > 0:
                        actionBolus = eventualError / ISFfinal

        ''' Update history vectors '''
        self.prevMeal = internalMeal
        self.basalHistory = np.roll(self.basalHistory,-1)
        self.bolusHistory = np.roll(self.bolusHistory,-1)
        self.bolusHistory[len(self.bolusHistory)-1] = actionBolus

        ''' Artificial Pancreas action '''
        if (APEnable == True):

            ''' Low Threshold Suspend '''
            if ((observation.CGM < targetMin) or
                (eventualCGMWoIOB < targetMin) or
                (eventualCGM < targetMin)) :
                self.basalHistory[24*12:24*12+6] = np.ones(6)*(basalNeeds/12)*(-1)
            else:
                self.basalHistory[24*12:24*12+6] = np.ones(6)*(actionBasal/6 + basalNeeds/12)

            basalNeedsDetected = self.basalHistory[24*12]/5 # From U/sim_step to U/min

        else:
            basalNeedsDetected = basalNeeds/60  # From U/h to U/min

        ''' Cannula partial blockage simulation '''
        blockage = 0.0
        if ((self.simulationTime >= (12*60)) and (self.simBlockage == True)):
            blockage = (ap.Basal.values[0]/2)/60

        action = Action(basalNeedsDetected, actionBolus, blockage)

        return action

    def reset(self):
        '''
        Reset the controller state to inital state, must be implemented
        '''
        self.__init__()


# specify start_time as the beginning of today
now = datetime.now()
start_time = datetime.combine(now.date(), datetime.min.time())
sim_time=timedelta(days=1)

# --------- Specify the simulation batch --------
#  Batch 1 -> Auto basal, no blockage and no meals
#  Batch 2 -> Auto basal, no blockage and meals
#  Batch 3 -> Auto basal, blockage and no meals
#  Batch 4 -> Auto basal, blockage and meals
batch_number = 4

# --------- Patients list --------------
'''
patient_names=['adolescent#010']

'''
patient_names=['adolescent#001','adolescent#002','adolescent#003','adolescent#004',
               'adolescent#005','adolescent#006','adolescent#007','adolescent#008',
               'adolescent#009','adolescent#010',
               'adult#001','adult#002','adult#003','adult#004','adult#005',
               'adult#006','adult#007','adult#008','adult#009','adult#010',
               'child#001','child#002','child#003','child#004','child#005',
               'child#006','child#007','child#008','child#009','child#010']

# Create meal scenario based on the simulation batch
if (batch_number == 2) or (batch_number == 4):
    scen = [(7, 56), (13,37), (17, 9), (20, 38), (23, 0)]
else:
    scen = [(7, 00), (12,00), (17, 0), (20, 00), (23, 0)]

scenario = CustomScenario(start_time=start_time, scenario=scen)

# Run simulation
def local_build_env(pname):
    patient = T1DPatient.withName(pname)
    cgm_sensor = CGMSensor.withName('GuardianRT', seed=1)
    insulin_pump = InsulinPump.withName('Insulet')
    scen = copy.deepcopy(scenario)
    env = T1DSimEnv(patient, cgm_sensor, insulin_pump, scen)
    return env

envs = [local_build_env(p) for p in patient_names]

# Create a controller
controller = MyController(0,batch_number)
ctrllers = [copy.deepcopy(controller) for _ in range(len(envs))]
path = './results_batch' + str(batch_number)
sim_instances = [SimObj(e,
                        c,
                        sim_time,
                        animate=False,
                        path=path) for (e, c) in zip(envs, ctrllers)]
results = batch_sim(sim_instances, parallel=False)
df = pd.concat(results, keys=[s.env.patient.name for s in sim_instances])
results, ri_per_hour, zone_stats, figs, axes = report(df, path)
