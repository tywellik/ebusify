import UtilityManager
import BusManager
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import datetime
import shutil
import math
import os

##################################################
#      Parsing Utility Manager Information       #
##################################################
df_solarWind = pd.read_csv('../resrc/other/solar_wind_basecase.csv', index_col='Time of Day', parse_dates=True)
solar = df_solarWind['Solar_Production_MW'].astype('float64')
wind  = df_solarWind['Wind_Production_MW'].astype('float64')

df_utilSources = pd.read_csv('../resrc/other/source_info.csv')
names      = df_utilSources['Name'].astype('string')
types      = df_utilSources['Type'].astype('string')
maxCaps    = df_utilSources['Max Cap'].astype('double')
minCaps    = df_utilSources['Min Cap'].astype('double')
runCosts   = df_utilSources['Running Cost'].astype('double')
rampRates  = df_utilSources['Ramp Rate'].astype('double')
rampCosts  = df_utilSources['Ramping Cost'].astype('double')
startCosts = df_utilSources['Startup Cost'].astype('double')

df_nonBusConsump = pd.read_csv('../resrc/other/non_bus_consump.csv', index_col='Hour')
df_nonBusConsump.reset_index(inplace=True)

##################################################
#                  Moving Mean                   #
##################################################
df_movingMean = pd.read_csv('../resrc/other/solar_wind_movmean.csv', index_col='Time')
solar_wind_movmean = df_movingMean['MovAvg3'].astype('double')
sw_movmean = solar_wind_movmean.values

##################################################
#        Parsing Bus Manager Information         #
##################################################
df_chargerInfo = pd.read_csv('../resrc/other/charger_info.csv')
chrgrIds      = df_chargerInfo['chrg_ID'].astype('int32')
chrgrName     = df_chargerInfo['chrg_name'].astype('string')
numPlugs      = df_chargerInfo['num_plugs'].astype('int32')

df_busCapacities = pd.read_csv('../resrc/other/bus_capacities.csv')
busIds        = df_busCapacities['bus_ID'].astype('int32')
capacities    = df_busCapacities['Capacity'].astype('double')
consumption   = df_busCapacities['Consumption'].astype('double')
chargeRates   = df_busCapacities['Charge_rate'].astype('double')
distFirstChrg = df_busCapacities['Dist_First_Charge'].astype('double')

def translate(x):
    if isinstance(x, float):
        return 102600 # 4:30am the next day
    (hourStr, minuteStr) = x.split(':')
    day = 1
    hour = int(hourStr)
    minute = int(minuteStr[0:2])
    if hour == 12:
        hour = 0
    if minuteStr[2] == 'p':
        hour += 12
    if (hour < 4) or (hour == 4 and minute < 30):
        day = 2
    time = datetime.datetime(year=1970, month=1, day=day, hour=hour, minute=minute)
    return (time - datetime.datetime(1970, 1, 1)).total_seconds()

df_busSchedule = pd.read_csv("../resrc/other/bus_charge_schedule.csv", parse_dates=True)
df_busSchedule['charge_st'] = df_busSchedule['charge_st'].apply(translate)
df_busSchedule['charge_end'] = df_busSchedule['charge_end'].apply(translate)
schedRouteIds = df_busSchedule['route_ID'].astype('int32')
schedBusIds   = df_busSchedule['bus_ID'].astype('int32')
chargeStrts   = df_busSchedule['charge_st'].astype('int32')
chargeEnds    = df_busSchedule['charge_end'].astype('int32')
distNextChrg  = df_busSchedule['dist_next_chg'].astype('double')
schedChrgrIds = df_busSchedule['charger_ID'].astype('int32')

##################################################
#                 Running Model                  #
##################################################
avgBusPower = 130.605 * 60 / 1000  # MW
totalCost = []
#busManagerMode = 2 # Diesel buses
#busManagerMode = 1 # Smart Charging
busManagerMode = 0 # Normal Operation Charging (No Discharge, Charge when needed)
movmean = False

cwd = os.getcwd()
source = cwd + "/output/"

for ffac in range(649, 650):
    ##################################################
    #          Initializing Utility Manager          #
    ##################################################
    AustinEnergy = UtilityManager.UtilityManager()
    AustinEnergy.init(names.values, types.values, maxCaps.values,
                    minCaps.values, runCosts.values, rampRates.values,
                    rampCosts.values, startCosts.values,
                    solar.values, wind.values)

    ##################################################
    #            Initializing Bus Manager            #
    ##################################################
    CapMetro = BusManager.BusManager()
    CapMetro.init_chargers(chrgrIds.values, chrgrName.values, 
                        numPlugs.values)
    CapMetro.init_buses(busIds.values, capacities.values, 
                        consumption.values, chargeRates.values, 
                        distFirstChrg.values)
    CapMetro.init_schedule(schedRouteIds.values, schedBusIds.values, chargeStrts.values,
                        chargeEnds.values, distNextChrg.values, schedChrgrIds.values)

    busPwrTime     = []
    busTrgtPwrTime = []
    fltPwrTime     = []
    renewPwrTime   = []
    fltrFactor = (ffac) / 1000.0
    print("\n\nFilter Factor: " + str(fltrFactor))

    #for idx, power in enumerate(df_nonBusConsump['Non-Bus Consumption (MW)']):
    for idx, row in df_nonBusConsump.iterrows():
        power = row[1]
        if (idx == 0):
            if (busManagerMode < 2):
                # Calculate filtered power
                if (movmean):
                    fltrPower = sw_movmean[idx]
                else:
                    fltrPower = solar[idx] + wind[idx]
                renewPwrTime.append(solar[idx] + wind[idx])
                fltPwrTime.append(fltrPower)

                # Find difference between power and filtered power, 
                # bus manager will attempt to absorb(+) or provide(-) this
                busTargetPower = avgBusPower + (solar[idx] + wind[idx]) - fltrPower
                busTrgtPwrTime.append(busTargetPower)

                busPower = CapMetro.run(busTargetPower*1000, busManagerMode, 16200 + (idx*60))
                busPwrTime.append(busPower/1000)

            else:
                busPower = 0
                fltPwrTime.append(0)
                busPwrTime.append(0)
            
            AustinEnergy.startup(power + busPower/1000)

        else:
            if (busManagerMode < 2):
                # Calculate filtered power
                if (movmean):
                    fltrPower = sw_movmean[idx]
                else:
                    fltrPower = (fltrFactor * fltrPower) + ((1-fltrFactor) * (solar[idx] + wind[idx]))
                renewPwrTime.append(solar[idx] + wind[idx])
                fltPwrTime.append(fltrPower)
                
                # Find difference between power and filtered power, 
                # bus manager will attempt to absorb(+) or provide(-) this
                busTargetPower = avgBusPower + (solar[idx] + wind[idx]) - fltrPower
                busTrgtPwrTime.append(busTargetPower)

                busPower = CapMetro.run(busTargetPower*1000, busManagerMode, 16200 + (idx*60))
                busPwrTime.append(busPower/1000)

            else:
                busPower = 0
                fltPwrTime.append(0)
                busPwrTime.append(0)

            AustinEnergy.power_request(power + busPower/1000)

    # Dump Information to files then clear manager objects
    AustinEnergy.file_dump()
    CapMetro.file_dump()
    totalCost.append(AustinEnergy.get_totalCost())

    time = range(len(busPwrTime))
    result = map(float.__sub__, busTrgtPwrTime, busPwrTime)

    pltTime     = pd.Series(data=time, name='time')
    busTarget   = pd.Series(data=busTrgtPwrTime, name='Bus Target Power')
    busAchieved = pd.Series(data=busPwrTime, name='Bus Achieved Power')
    df_busTarget = pd.concat([busTarget, busAchieved], axis=1)
    df_busTarget.set_index(pltTime, inplace=True)

    renewPwr    = pd.Series(data=renewPwrTime, name='Renewable Power')
    fltRenewPwr = pd.Series(data=fltPwrTime, name='Filtered Renewable Power')
    df_renewPwr = pd.concat([renewPwr, fltRenewPwr], axis=1)
    df_renewPwr.set_index(pltTime, inplace=True)

    df_busTarget.to_csv('output/BusManagerPower.csv')
    df_renewPwr.to_csv('output/RenewablePower.csv')
    '''
    df_busTarget.plot()
    plt.show()

    df_renewPwr.plot()
    plt.show()
    '''

    # Make new directory and move output files
    dest = cwd + "/ffac/ffac_" + str(ffac)
    os.mkdir(dest)
    files = os.listdir(source)

    for f in files:
        shutil.move(source+f, dest)


filterFactors = range(649, 650)
ff_index = pd.Series(data=filterFactors, name='Filter Factors')
ff_index = ff_index.apply(lambda x: (x)/1000.0)
df_filterFactorCost = pd.Series(data=totalCost, name='Total Cost').to_frame()
df_filterFactorCost.set_index(ff_index, inplace=True)

df_filterFactorCost.to_csv('output/FilterFactorCost.csv')
df_filterFactorCost.plot()
plt.show()
