# -*- coding: utf-8 -*-

import BusManager
import pandas as pd
import numpy as np
import datetime
import math

CapMetro = BusManager.BusManager()

df3 = pd.read_csv('../resrc/other/charger_info.csv')
chrgrIds      = df3['chrg_ID'].astype('int32')
chrgrName     = df3['chrg_name'].astype('string')
numPlugs      = df3['num_plugs'].astype('int32')
CapMetro.init_chargers(chrgrIds.values, chrgrName.values, 
                    numPlugs.values)


df = pd.read_csv('../resrc/other/bus_capacities.csv')
busIds        = df['bus_ID'].astype('int32')
capacities    = df['Capacity'].astype('double')
consumption   = df['Consumption'].astype('double')
chargeRates   = df['Charge_rate'].astype('double')
distFirstChrg = df['Dist_First_Charge'].astype('double')
#print (df)
CapMetro.init_buses(busIds.values, capacities.values, 
                    consumption.values, chargeRates.values, 
                    distFirstChrg.values)

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

df2 = pd.read_csv("../resrc/other/bus_charge_schedule.csv", parse_dates=True)
#print (df2)
df2['charge_st'] = df2['charge_st'].apply(translate)
df2['charge_end'] = df2['charge_end'].apply(translate)
#print(df2)

routeIds      = df2['route_ID'].astype('int32')
busIds        = df2['bus_ID'].astype('int32')
chargeStrts   = df2['charge_st'].astype('int32')
chargeEnds    = df2['charge_end'].astype('int32')
distNextChrg  = df2['dist_next_chg'].astype('double')
chrgrIds      = df2['charger_ID'].astype('int32')
CapMetro.init_schedule(routeIds.values, busIds.values, chargeStrts.values,
                        chargeEnds.values, distNextChrg.values, chrgrIds.values)

for i in range(0, 1440):
    CapMetro.run(1234.56, 16200+i*60)

CapMetro.file_dump()
