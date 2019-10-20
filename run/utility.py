# -*- coding: utf-8 -*-

import UtilityManager
import pandas as pd

#df1 = pd.read_csv('../resrc/other/solar_wind_basecase.csv', index_col='Time of Day', parse_dates=True)
df1 = pd.read_csv('../resrc/other/solar_wind_none.csv', index_col='Time of Day', parse_dates=True)
print(df1)
solar = df1['Solar_Production_MW'].astype('float64')
wind  = df1['Wind_Production_MW'].astype('float64')

df2 = pd.read_csv('../resrc/other/source_info.csv')
names     = df2['Name'].astype('string')
types     = df2['Type'].astype('string')
maxCaps   = df2['Max Cap'].astype('double')
minCaps   = df2['Min Cap'].astype('double')
runCosts  = df2['Running Cost'].astype('double')
rampRates = df2['Ramp Rate'].astype('double')
#print(df2)n

df3 = pd.read_csv('../resrc/other/non_bus_consump.csv', index_col='Hour')

AustinEnergy = UtilityManager.UtilityManager()
AustinEnergy.init(names.values, types.values, maxCaps.values, \
                  minCaps.values, runCosts.values, rampRates.values, \
                  solar.values, wind.values)


# Running through one day
for idx, power in enumerate(df3['Non-Bus Consumption (MW)']):
    if (idx == 0):
        AustinEnergy.startup(power)
    else:
        AustinEnergy.power_request(power)


AustinEnergy.file_dump()