import UtilityManager
import BusManager
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import datetime
import math

##################################################
#      Parsing Utility Manager Information       #
##################################################
df_solarWind = pd.read_csv('../resrc/other/solar_wind_basecase.csv', index_col='Time of Day', parse_dates=True)
solar = df_solarWind['Solar_Production_MW'].astype('float64')
wind  = df_solarWind['Wind_Production_MW'].astype('float64')

df_utilSources = pd.read_csv('../resrc/other/source_info.csv')
names     = df_utilSources['Name'].astype('string')
types     = df_utilSources['Type'].astype('string')
maxCaps   = df_utilSources['Max Cap'].astype('double')
minCaps   = df_utilSources['Min Cap'].astype('double')
runCosts  = df_utilSources['Running Cost'].astype('double')
rampRates = df_utilSources['Ramp Rate'].astype('double')

df_nonBusConsump = pd.read_csv('../resrc/other/non_bus_consump.csv', index_col='Hour')

##################################################
#          Initializing Utility Manager          #
##################################################
AustinEnergy = UtilityManager.UtilityManager()
AustinEnergy.init(names.values, types.values, maxCaps.values,
                  minCaps.values, runCosts.values, rampRates.values,
                  solar.values, wind.values)


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

##################################################
#                 Running Model                  #
##################################################
avgBusPower = 130.605 * 60 / 1000  # MW
fltrFactor = 0.75
busPwrTime = []
busTrgtPwrTime = []

for idx, power in enumerate(df_nonBusConsump['Non-Bus Consumption (MW)']):
    if (idx == 0):
        fltrPower = power - solar[idx] - wind[idx]

        busTargetPower = avgBusPower + (power - solar[idx] - wind[idx]) - fltrPower
        busTrgtPwrTime.append(busTargetPower)

        busPower = CapMetro.run(busTargetPower*1000, 16200 + (idx*60))
        busPwrTime.append(busPower/1000)

        AustinEnergy.startup(power)

    else:
        # Calculate filtered power
        fltrPower = (fltrFactor * fltrPower) + ((1-fltrFactor) * (power - solar[idx] - wind[idx]))
        
        # Find difference between power and filtered power, 
        # bus manager will attempt to absorb(+) or provide(-) this
        busTargetPower = avgBusPower + (power - solar[idx] - wind[idx]) - fltrPower
        busTrgtPwrTime.append(busTargetPower)

        busPower = CapMetro.run(busTargetPower*1000, 16200 + (idx*60))
        busPwrTime.append(busPower/1000)

        AustinEnergy.power_request(power + busPower/1000)


AustinEnergy.file_dump()
CapMetro.file_dump()

time = range(len(busPwrTime))
plt.plot(time, busTrgtPwrTime, 'r', time, busPwrTime, 'b')
plt.show()
