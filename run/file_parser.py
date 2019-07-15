import pandas as pd
import datetime

def parse_files(allFiles, movMeanWindow = 10):

    # Function used to translate AM/PM time values into seconds
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

    ##################################################
    #      Parsing Utility Manager Information       #
    ##################################################
    df_solarWind = pd.read_csv(allFiles['solarWind'], index_col='Time of Day')
    solar = df_solarWind['Solar_Production_MW'].astype('float64')
    wind  = df_solarWind['Wind_Production_MW'].astype('float64')

    solar_mm = solar.rolling(window=movMeanWindow, min_periods=1, center=True).mean()
    wind_mm = wind.rolling(window=movMeanWindow, min_periods=1, center=True).mean()

    utilSolarWind_data = {
        'solar': solar.values,
        'wind': wind.values,
        'solar_mm': solar_mm.values,
        'wind_mm': wind_mm.values
    }

    df_utilSources = pd.read_csv(allFiles['utilSources'])
    names      = df_utilSources['Name'].astype('string')
    types      = df_utilSources['Type'].astype('string')
    maxCaps    = df_utilSources['Max Cap'].astype('double')
    minCaps    = df_utilSources['Min Cap'].astype('double')
    runCosts   = df_utilSources['Running Cost'].astype('double')
    rampRates  = df_utilSources['Ramp Rate'].astype('double')
    rampCosts  = df_utilSources['Ramping Cost'].astype('double')
    startCosts = df_utilSources['Startup Cost'].astype('double')

    utilSources_data = {
        'names': names.values,
        'types': types.values,
        'maxCaps': maxCaps.values,
        'minCaps': minCaps.values,
        'runCosts': runCosts.values,
        'rampRates': rampRates.values,
        'rampCosts': rampCosts.values,
        'startCosts': startCosts.values
    }
    
    df_nonBusConsump = pd.read_csv(allFiles['nonBusConsump'])

    ##################################################
    #            Bus Manager Information             #
    ##################################################
    df_busCapacities = pd.read_csv(allFiles['busCapacities'])
    busIds        = df_busCapacities['bus_ID'].astype('int32')
    capacities    = df_busCapacities['Capacity'].astype('double')
    consumption   = df_busCapacities['Consumption'].astype('double')
    chargeRates   = df_busCapacities['Charge_rate'].astype('double')
    distFirstChrg = df_busCapacities['Dist_First_Charge'].astype('double')

    busCapacities_data = {
        'busIds': busIds.values,
        'capacities': capacities.values, 
        'consumption': consumption.values,
        'chargeRates': chargeRates.values,
        'distFirstChrg': distFirstChrg.values
    }

    df_busSchedule = pd.read_csv(allFiles['busSchedule'])
    df_busSchedule['charge_st'] = df_busSchedule['charge_st'].apply(translate)
    df_busSchedule['charge_end'] = df_busSchedule['charge_end'].apply(translate)
    busToCharger  = df_busSchedule.groupby(['bus_ID']).charger_ID.first().to_frame().to_dict()['charger_ID']
    schedRouteIds = df_busSchedule['route_ID'].astype('int32')
    schedBusIds   = df_busSchedule['bus_ID'].astype('int32')
    chargeStrts   = df_busSchedule['charge_st'].astype('int32')
    chargeEnds    = df_busSchedule['charge_end'].astype('int32')
    distNextChrg  = df_busSchedule['dist_next_chg'].astype('double')
    schedChrgrIds = df_busSchedule['charger_ID'].astype('int32')

    busSchedule_data = {
        'schedRouteIds': schedRouteIds.values,
        'schedBusIds': schedBusIds.values, 
        'chargeStrts': chargeStrts.values,
        'chargeEnds': chargeEnds.values,
        'distNextChrg': distNextChrg.values,
        'schedChrgrIds': schedChrgrIds.values
    }

    df_chargerInfo = pd.read_csv(allFiles['chargerInfo'])
    chrgrIds  = df_chargerInfo['chrg_ID'].astype('int32')
    chrgrName = df_chargerInfo['chrg_name'].astype('string')
    numPlugs  = df_chargerInfo['num_plugs'].astype('int32')

    # Add chargers included with buses into chargerInfo_data
    df_chargerInfo.set_index('chrg_ID', inplace=True)
    chrgStationInclPlugs = {}
    for idx, row in df_busCapacities.loc[df_busCapacities['Capacity'] == 80].iterrows():
        chargerID = busToCharger[int(row['bus_ID'])]
        if chargerID in chrgStationInclPlugs.keys():
            chrgStationInclPlugs[chargerID] += 1
        else:
            chrgStationInclPlugs[chargerID] = 1

    for chrgr in chrgStationInclPlugs:
        df_chargerInfo.at[chrgr, 'incl_plugs'] = int(chrgStationInclPlugs[chrgr])

    df_chargerInfo.fillna(0, inplace=True)
    inclPlugs  = df_chargerInfo['incl_plugs'].astype('int32')

    chargerInfo_data = {
        'chrgrIds': chrgrIds.values,
        'chrgrName': chrgrName.values, 
        'numPlugs': numPlugs.values,
        'inclPlugs': inclPlugs.values
    }

    ##################################################
    #             Return All Information             #
    ##################################################
    return {
        'utilSolarWind': utilSolarWind_data,
        'utilSources': utilSources_data,
        'nonBusConsump': df_nonBusConsump['Non-Bus Consumption (MW)'].astype('double').values,
        'chargerInfo': chargerInfo_data,
        'busCapacities': busCapacities_data,
        'busSchedule': busSchedule_data,        
    }