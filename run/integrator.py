from model_runner import run_model
from file_parser import parse_files
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import shutil
import os

##################################################
#             Filter Factor Settings             #
##################################################
ffac_runOpt     = True
ffac_statFac    = 0.55 # Filter factor used when ffac_runOpt is False
ffac_rangeBegin = 0.01
ffac_rangeEnd   = 0.99
ffac_rangeStep  = 0.01
ffac_range      = [x/1000 for x in np.arange(start=ffac_rangeBegin*1000, stop=(ffac_rangeEnd+ffac_rangeStep)*1000, step=ffac_rangeStep*1000)]
ffac_useMovMean = False
ffac_movMeanWin = 30

##################################################
#              Bus Manager Settings              #
##################################################
# Modes
# 0 - Normal Operation Charging
# 1 - Smart Charging
# 2 - Diesel buses
busMan_mode = 1
avgBusPower = 130.605 * 60 / 1000  # MW


##################################################
#                  Input Files                   #
##################################################
inFile_solarWind     = '../resrc/other/solar_wind_basecase.csv'
inFile_utilSources   = '../resrc/other/source_info.csv'
inFile_nonBusConsump = '../resrc/other/non_bus_consump.csv'
inFile_chargerInfo   = '../resrc/other/charger_info.csv'
inFile_busCapacities = '../resrc/other/bus_capacities.csv'
inFile_busSchedule   = '../resrc/other/bus_charge_schedule.csv'
inFile_allFiles = {
    'solarWind': inFile_solarWind,
    'utilSources': inFile_utilSources,
    'nonBusConsump': inFile_nonBusConsump,
    'chargerInfo': inFile_chargerInfo,
    'busCapacities': inFile_busCapacities,
    'busSchedule': inFile_busSchedule
}
inFile_data = parse_files(inFile_allFiles, ffac_movMeanWin)


##################################################
#                 Model Settings                 #
##################################################
modelSettings = {
    'busMan_mode': busMan_mode,
    'inFile_data': inFile_data,
    'use_movMean': ffac_useMovMean,
    'avg_busPower': avgBusPower
}


##################################################
#                 Running Model                  #
##################################################
cwd = os.getcwd()
source = cwd + "/output/"

if ffac_runOpt:
    totalCost = []
    for ffac in ffac_range:
        modelSettings['filtfactor'] = ffac
        modelOutput = run_model(modelSettings, inFile_data)

        totalCost.append(modelOutput['totalCost'])

        time = range(len(modelOutput['busPwrTime']))
        result = map(float.__sub__, modelOutput['busTrgtPwrTime'], modelOutput['busPwrTime'])

        pltTime     = pd.Series(data=time, name='time')
        busTarget   = pd.Series(data=modelOutput['busTrgtPwrTime'], name='Bus Target Power')
        busAchieved = pd.Series(data=modelOutput['busPwrTime'], name='Bus Achieved Power')
        df_busTarget = pd.concat([busTarget, busAchieved], axis=1)
        df_busTarget.set_index(pltTime, inplace=True)

        renewPwr    = pd.Series(data=modelOutput['renewPwrTime'], name='Renewable Power')
        fltRenewPwr = pd.Series(data=modelOutput['fltPwrTime'], name='Filtered Renewable Power')
        df_renewPwr = pd.concat([renewPwr, fltRenewPwr], axis=1)
        df_renewPwr.set_index(pltTime, inplace=True)

        df_busTarget.to_csv('output/BusManagerPower.csv')
        df_renewPwr.to_csv('output/RenewablePower.csv')

        # Make new directory and move output files
        dest = cwd + "/ffac/ffac_" + str(int(ffac*1000))
        os.mkdir(dest)
        files = os.listdir(source)

        for f in files:
            shutil.move(source+f, dest)
    
    ff_index = pd.Series(data=ffac_range, name='Filter Factors')
    df_filterFactorCost = pd.Series(data=totalCost, name='Total Cost').to_frame()
    df_filterFactorCost.set_index(ff_index, inplace=True)

    df_filterFactorCost.to_csv('output/FilterFactorCost.csv')
    df_filterFactorCost.plot()
    plt.show()

else:
    modelSettings['filtfactor'] = ffac_statFac
    modelOutput = run_model(modelSettings, inFile_data)

    print("Total Cost: " + str(modelOutput['totalCost']))

    time = range(len(modelOutput['busPwrTime']))
    result = map(float.__sub__, modelOutput['busTrgtPwrTime'], modelOutput['busPwrTime'])

    pltTime     = pd.Series(data=time, name='time')
    busTarget   = pd.Series(data=modelOutput['busTrgtPwrTime'], name='Bus Target Power')
    busAchieved = pd.Series(data=modelOutput['busPwrTime'], name='Bus Achieved Power')
    df_busTarget = pd.concat([busTarget, busAchieved], axis=1)
    df_busTarget.set_index(pltTime, inplace=True)

    renewPwr    = pd.Series(data=modelOutput['renewPwrTime'], name='Renewable Power')
    fltRenewPwr = pd.Series(data=modelOutput['fltPwrTime'], name='Filtered Renewable Power')
    df_renewPwr = pd.concat([renewPwr, fltRenewPwr], axis=1)
    df_renewPwr.set_index(pltTime, inplace=True)

    df_busTarget.to_csv('output/BusManagerPower.csv')
    df_renewPwr.to_csv('output/RenewablePower.csv')
