import UtilityManager
import BusManager
import numpy as np

def run_model(model_settings, inFile_data):

    ##################################################
    #          Initializing Utility Manager          #
    ##################################################
    AustinEnergy = UtilityManager.UtilityManager()
    AustinEnergy.init(inFile_data['utilSources']['names'],
                    inFile_data['utilSources']['types'],
                    inFile_data['utilSources']['maxCaps'],
                    inFile_data['utilSources']['minCaps'],
                    inFile_data['utilSources']['runCosts'],
                    inFile_data['utilSources']['rampRates'],
                    inFile_data['utilSources']['rampCosts'],
                    inFile_data['utilSources']['startCosts'],
                    inFile_data['utilSolarWind']['solar'],
                    inFile_data['utilSolarWind']['wind'])

    ##################################################
    #            Initializing Bus Manager            #
    ##################################################
    CapMetro = BusManager.BusManager()
    CapMetro.init_chargers(inFile_data['chargerInfo']['chrgrIds'],
                        inFile_data['chargerInfo']['chrgrName'],
                        inFile_data['chargerInfo']['numPlugs'],
                        inFile_data['chargerInfo']['plugTypes'])

    CapMetro.init_buses(inFile_data['busCapacities']['busIds'],
                        inFile_data['busCapacities']['capacities'],
                        inFile_data['busCapacities']['consumption'],
                        inFile_data['busCapacities']['chargeRates'], 
                        inFile_data['busCapacities']['distFirstChrg'],
                        inFile_data['busCapacities']['plugTypes'])

    CapMetro.init_schedule(inFile_data['busSchedule']['schedRouteIds'],
                        inFile_data['busSchedule']['schedBusIds'],
                        inFile_data['busSchedule']['chargeStrts'],
                        inFile_data['busSchedule']['chargeEnds'],
                        inFile_data['busSchedule']['distNextChrg'],
                        inFile_data['busSchedule']['schedChrgrIds'])

    busPwrTime     = []
    busTrgtPwrTime = []
    fltPwrTime     = []
    renewPwrTime   = []
    useMovMean = model_settings['use_movMean']
    busManagerMode = model_settings['busMan_mode']
    avgBusPower = model_settings['avg_busPower']
    fltrFactor = model_settings['filtfactor']
    print("\n\nFilter Factor: " + str(fltrFactor))
    
    for idx, power in enumerate(inFile_data['nonBusConsump']):
        solar = inFile_data['utilSolarWind']['solar'][idx]
        wind = inFile_data['utilSolarWind']['wind'][idx]
        sw_movmean = inFile_data['utilSolarWind']['solar_mm'][idx] \
                   + inFile_data['utilSolarWind']['wind_mm'][idx]

        if (idx == 0):
            if (busManagerMode < 2):
                # Calculate filtered power
                if (useMovMean):
                    fltrPower = sw_movmean
                else:
                    fltrPower = solar + wind
                renewPwrTime.append(solar + wind)
                fltPwrTime.append(fltrPower)

                # Find difference between power and filtered power, 
                # bus manager will attempt to absorb(+) or provide(-) this
                busTargetPower = avgBusPower + (solar + wind) - fltrPower
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
                if (useMovMean):
                    fltrPower = sw_movmean
                else:
                    fltrPower = (fltrFactor * fltrPower) + ((1-fltrFactor) * (solar + wind))
                renewPwrTime.append(solar + wind)
                fltPwrTime.append(fltrPower)
                
                # Find difference between power and filtered power, 
                # bus manager will attempt to absorb(+) or provide(-) this
                busTargetPower = avgBusPower + (solar + wind) - fltrPower
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

    modelOutput = {
        'totalCost': AustinEnergy.get_totalCost(),
        'busPwrTime': busPwrTime,
        'busTrgtPwrTime': busTrgtPwrTime,
        'renewPwrTime': renewPwrTime,
        'fltPwrTime': fltPwrTime
    }

    return modelOutput
