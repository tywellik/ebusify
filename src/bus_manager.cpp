#include "bus_manager.hpp"
#include "error.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <math.h>


//#define VERBOSE
#define LOGERR(fmt, args...)   do{ fprintf(stderr, fmt "\n", ##args); }while(0)
#ifdef VERBOSE
    #define LOGDBG(fmt, args...)   do{ fprintf(stdout, fmt "\n", ##args); }while(0)
#else
    #define LOGDBG(fmt, args...)   do{}while(0)
#endif


namespace BUS {

#define SMART_CHARGE        0x01
#define ALLOW_DISCHARGE     0x02

BusManager::BusManager()
:
    _totalCharge(0.0)
{}


BusManager::~BusManager()
{}


int
BusManager::init_chargers(bpn::ndarray const& chargerIds, bpn::ndarray const& chargerNames,
                        bpn::ndarray const& numberPlugs, bpn::ndarray const& plugTypes)
{
    unsigned int dataLen = chargerIds.shape(0);
    if (dataLen != chargerNames.shape(0)  || dataLen != numberPlugs.shape(0)){
        PyErr_SetString(PyExc_TypeError, "Bus data lengths inconsistent");
        bp::throw_error_already_set();
    }

    int* chrgIds   = reinterpret_cast<int*>(chargerIds.get_data());
    int* numPlugs  = reinterpret_cast<int*>(numberPlugs.get_data());
    int* plgTypes  = reinterpret_cast<int*>(plugTypes.get_data());
    
    //std::map<int, ChargerPtr> _chargers;
    for (int line = 0; line < dataLen; ++line){
        int chrgId = chrgIds[line];
        std::string chargerName = std::string(bp::extract<char const *>(chargerNames[line]));
        std::string plugName = std::string(bp::extract<char const *>(plugTypes[line]));
        PlugType plugType = plugNameToType[plugName];
        ChargerPtr chrgPtr;

        // If charging station is new, add it to the map then add plugs
        // otherwise, just add plugs
        auto it = _chargers.find(chrgId);
        if ( it == _chargers.end() ){
            Charger *charger = new Charger(chrgId, chargerName);
            charger->add_plugs(numPlugs[line], plugType);
            chrgPtr.reset(charger);
            _chargers.insert(std::pair<int, ChargerPtr>(chrgId, chrgPtr));
        } else {
            it->second->add_plugs(numPlugs[line], plugType);
        }
    }
}


int 
BusManager::init_buses(bpn::ndarray const& busIdentifiers, bpn::ndarray const& capacities,
                    bpn::ndarray const& consumptionRates, bpn::ndarray const& chargeRates,
                    bpn::ndarray const& distFirstCharge, bpn::ndarray const& plugTypes)
{
    unsigned int dataLen = busIdentifiers.shape(0);
    if (dataLen != capacities.shape(0)  || dataLen != consumptionRates.shape(0) ||
        dataLen != chargeRates.shape(0) || dataLen != distFirstCharge.shape(0)){
        PyErr_SetString(PyExc_TypeError, "Bus data lengths inconsistent");
        bp::throw_error_already_set();
    }

    int* busIds           = reinterpret_cast<int*>(busIdentifiers.get_data());
    double* caps          = reinterpret_cast<double*>(capacities.get_data());
    double* consumpRates  = reinterpret_cast<double*>(consumptionRates.get_data());
    double* chrgRates     = reinterpret_cast<double*>(chargeRates.get_data());
    double* distFirstChrg = reinterpret_cast<double*>(distFirstCharge.get_data());

    for (int line = 0; line < dataLen; ++line)
    {
        std::string plugName = std::string(bp::extract<char const *>(plugTypes[line]));
        PlugType plugType = plugNameToType[plugName];

        // If bus is new, add it to the map
        if ( _buses.find(busIds[line]) == _buses.end() ){
            BusPtr busPtr;
            Bus *bus = new Bus(busIds[line], caps[line], consumpRates[line], chrgRates[line], distFirstChrg[line], plugType);
            busPtr.reset(bus);
            _buses.insert(std::pair<int, BusPtr>(busIds[line], busPtr));
        }
    }
}


int
BusManager::init_schedule(bpn::ndarray const& routeIdentifiers, bpn::ndarray const& busIdentifiers,
                        bpn::ndarray const& chargeStartTs, bpn::ndarray const& chargeEndTs,
                        bpn::ndarray const& distNextChrg_mi, bpn::ndarray const& chargerIdentifiers)
{
    unsigned int dataLen = routeIdentifiers.shape(0);
    if (dataLen != busIdentifiers.shape(0) || dataLen != chargeStartTs.shape(0) ||
        dataLen != chargeEndTs.shape(0)    || dataLen != distNextChrg_mi.shape(0) ||
        dataLen != chargerIdentifiers.shape(0)){
        PyErr_SetString(PyExc_TypeError, "Schedule data lengths inconsistent");
        bp::throw_error_already_set();
    }

    int* routeIds        = reinterpret_cast<int*>(routeIdentifiers.get_data());
    int* busIds          = reinterpret_cast<int*>(busIdentifiers.get_data());
    int* chargeStart     = reinterpret_cast<int*>(chargeStartTs.get_data());
    int* chargeEnd       = reinterpret_cast<int*>(chargeEndTs.get_data());
    double* distNextChrg = reinterpret_cast<double*>(distNextChrg_mi.get_data());
    int* chargerIds      = reinterpret_cast<int*>(chargerIdentifiers.get_data());

    LOGDBG("Parsing Bus Schedule");
    for (int line = 0; line < dataLen; ++line){
        ChargerPtr chrgPtr;
        BusPtr busPtr;

        // If charging station is new, add it to the map
        if ( _chargers.find(chargerIds[line]) == _chargers.end() ){
            PyErr_SetString(PyExc_TypeError, "Charger does not exist");
            bp::throw_error_already_set();
        } else {
            chrgPtr = _chargers[chargerIds[line]];
        }

        // Get pointer to bus
        if ( _buses.find(busIds[line]) == _buses.end() ){
            PyErr_SetString(PyExc_TypeError, "Bus does not exist");
            bp::throw_error_already_set();
        } else {
            busPtr = _buses[busIds[line]];
        }

        int chrgStrt = chargeStart[line];
        int chrgEnd  = chargeEnd[line];

        // Add bus to _busSchedule for every time it is charging
        for (int i = chrgStrt; i < chrgEnd; i+=60){
            _busSchedule[chrgPtr][i].push_back(busPtr);
        }

        // Get bus back to 50% SOC
        if ( std::isnan(distNextChrg[line]) )
            distNextChrg[line] = (0.5 - 0.1) * busPtr->get_capacity() / busPtr->get_consumptionRate();

        _nextTripDist[busIds[line]][chrgEnd] = distNextChrg[line];
    }
}


int
BusManager::run(double powerRequest, int mode, time_t simTime)
{    
    bool allowSmartCharge = (mode & SMART_CHARGE);

    if ( (simTime % 3600) == 0)
        std::cout << "Sim Time: " << simTime/3600 << std::endl;

    double powerConsumption = 0.0;
    std::vector<Priority> priorities;
    std::map<BusPtr, bool> necessities;

    // Get departure times for all buses at each charging station
    for (auto& chrgr: _chargers){
        _nextDepart[chrgr.second] = get_nextDepartureTimes(chrgr.second, simTime);
        get_priorities(priorities, necessities, chrgr.second, simTime);
        for (auto& bus: priorities)
            _busToCharger[bus.first] = chrgr.second;
        _priorities[chrgr.second] = priorities;
        _necessities[chrgr.second] = necessities;
        priorities.clear();
        necessities.clear();
    }

    std::map<ChargerPtr, std::map<PlugType, int>> *chrgrsUsed = new std::map<ChargerPtr, std::map<PlugType, int>>;
    std::shared_ptr<std::map<ChargerPtr, std::map<PlugType, int>>> chrgrsUsedPtr;
    handle_necessaryCharging(powerConsumption, chrgrsUsed, simTime);
    if (allowSmartCharge)
        handle_powerRequest(powerConsumption, chrgrsUsed, powerRequest, simTime);
    else
        handle_remainingCharging(powerConsumption, chrgrsUsed, simTime);
    chrgrsUsedPtr.reset(chrgrsUsed);
    _chrgrsUsedTime.push_back(chrgrsUsedPtr);
    handle_routes(simTime);

    _busToCharger.clear();

    return powerConsumption;
}


void
BusManager::file_dump()
{
    std::ofstream outfile;

    /** Charger Usage */
    outfile.open("output/charger_usage.csv");
    outfile << ",";
    for (auto& chrgr: *_chrgrsUsedTime[0]){
        for (auto plugType = chrgr.second.begin(); plugType != chrgr.second.end(); plugType++)
            outfile << chrgr.first->get_name() << " - " << plugTypeToName[plugType->first] << ",";
    }
    outfile << std::endl;

    int simTime = 16200;
    for (auto& chrgrMap: _chrgrsUsedTime){
        outfile << simTime << ",";
        for (auto& chrgr: *chrgrMap){
            for (auto plugType = chrgr.second.begin(); plugType != chrgr.second.end(); plugType++)
                outfile << plugType->second << ",";
        }
        outfile << std::endl;

        simTime += 60;
    }
    outfile.close();

    /** Bus SOC */
    outfile.open("output/bus_soc.csv");
    outfile << ",";
    for (auto& bus: _buses)
        outfile << bus.second->get_identifier() << ",";
    outfile << std::endl;

    for (simTime = 16200; simTime < 16200 + 3600*24; simTime+=60){
        outfile << simTime << ",";
        for (auto& bus: _buses)
            outfile << bus.second->get_stateOfCharge(simTime) << ",";
        outfile << std::endl;
    }
    outfile.close();

    /** Bus Energy Usage */
    outfile.open("output/bus_energy.csv");
    outfile << ",";
    for (auto& bus: _buses)
        outfile << bus.second->get_identifier() << ",";
    outfile << std::endl;

    for (simTime = 16200; simTime < 16200 + 3600*24; simTime+=60){
        outfile << simTime << ",";
        for (auto& bus: _buses)
            outfile << bus.second->get_consumpCharger(simTime) << ",";
        outfile << std::endl;
    }
    outfile.close();

    /** Bus Route Usage */
    outfile.open("output/bus_route.csv");
    outfile << ",";
    for (auto& bus: _buses)
        outfile << bus.second->get_identifier() << ",";
    outfile << std::endl;

    for (simTime = 16200; simTime < 16200 + 3600*24; simTime+=60){
        outfile << simTime << ",";
        for (auto& bus: _buses)
            outfile << bus.second->get_consumpRoute(simTime) << ",";
        outfile << std::endl;
    }
    outfile.close();

    LOGDBG("Total Charge: %.2f", _totalCharge);

    return;
}


void
BusManager::clear_memory()
{
    _chrgrsUsedTime.clear();
    _energyChargedTime.clear();
}


int
BusManager::handle_necessaryCharging(double& pwrConsump, std::map<ChargerPtr, std::map<PlugType, int>> *chrgrsUsed, time_t simTime)
{
    int ret;
    double chrgRate;
    std::map<PlugType, int> numPlugs, plugsInUse;
    std::vector<Priority> priorities;
    std::map<BusPtr, bool> necessities;
    
    if (simTime == 62640){
        while(1){
            int i = 1;
            break;
        }
    }

    for(auto& chrgr: _busSchedule){
        numPlugs = chrgr.first->get_numPlugs();
        for ( auto it = numPlugs.begin(); it != numPlugs.end(); it++ )
            plugsInUse[it->first] = 0;
        priorities = _priorities[chrgr.first];
        necessities = _necessities[chrgr.first];

        // Priorities vector is in order so we charge the most necessary bus first
        for (auto& priority : priorities){
            BusPtr bus = priority.first;
            PlugType plugType = bus->get_plugType();

            if ( necessities[bus] == true && plugsInUse[plugType] < numPlugs[plugType] ){
                plugsInUse[plugType]++;
                chrgRate = bus->get_chargeRate(); // kWh / min
                chrgRate *= 60; // kW
                ret = bus->command_power(chrgRate, 60, simTime, PowerType::e_ATCHARGER);
                if ( ret != 0 ){
                    if ( ret == OVER_MAX_SOC ){
                        LOGDBG("%i:\tBus %s : Command would place bus over max SOC", simTime, bus->get_identifier().c_str());
                        double busSoc = bus->get_stateOfCharge();
                        double busCap = bus->get_capacity();
                        double busMaxSoc = bus->get_maxSoc();
                        double energyToCharge = (busMaxSoc - busSoc) * busCap; // kWh
                        bus->command_power(energyToCharge*60, 60, simTime, PowerType::e_ATCHARGER);
                        _totalCharge += energyToCharge;
                        pwrConsump += energyToCharge*60;
                    }
                    else if ( ret == UNDER_MIN_SOC ){
                        LOGDBG("Bus %i is below min SoC and charging", bus->get_identifier());
                        bus->command_power(chrgRate, 60, simTime, PowerType::e_ATCHARGER, true);
                        _totalCharge += chrgRate / 60;
                        pwrConsump += chrgRate;
                    }
                } else {
                    _totalCharge += chrgRate / 60;
                    pwrConsump += chrgRate;
                }
            }
        }
        (*chrgrsUsed)[chrgr.first] = plugsInUse;
        plugsInUse.clear();
    }

    return 0;
}


int
BusManager::handle_remainingCharging(double& pwrConsump, std::map<ChargerPtr, std::map<PlugType, int>> *chrgrsUsed, time_t simTime)
{
    int ret;
    double chrgRate;
    std::map<PlugType, int> numPlugs, plugsInUse;
    std::vector<Priority> priorities;
    std::map<BusPtr, bool> necessities;

    for(auto& chrgr: _busSchedule){
        numPlugs = chrgr.first->get_numPlugs();
        for ( auto it = numPlugs.begin(); it != numPlugs.end(); it++ )
            plugsInUse[it->first] = (*chrgrsUsed)[chrgr.first][it->first];
        priorities = _priorities[chrgr.first];
        necessities = _necessities[chrgr.first];

        // Priorities vector is in order so we charge the most necessary bus first
        for (auto& priority : priorities){
            BusPtr bus = priority.first;
            PlugType plugType = bus->get_plugType();
            double chargePriority = priority.second;

            if ( necessities[bus] == false && chargePriority > 0.0 && plugsInUse[plugType] < numPlugs[plugType] ){
                plugsInUse[plugType]++;
                chrgRate = bus->get_chargeRate(); // kWh / min
                chrgRate *= 60; // kW
                ret = bus->command_power(chrgRate, 60, simTime, PowerType::e_ATCHARGER);
                if ( ret != 0 ){
                    if ( ret == OVER_MAX_SOC ){
                        LOGDBG("%i:\tBus %s : Command would place bus over max SOC", simTime, bus->get_identifier().c_str());
                        double busSoc = bus->get_stateOfCharge();
                        double busCap = bus->get_capacity();
                        double busMaxSoc = bus->get_maxSoc();
                        double energyToCharge = (busMaxSoc - busSoc) * busCap; // kWh
                        bus->command_power(energyToCharge*60, 60, simTime, PowerType::e_ATCHARGER);
                        _totalCharge += energyToCharge;
                        pwrConsump += energyToCharge*60;
                    }
                    else if ( ret == UNDER_MIN_SOC ){
                        LOGDBG("Bus %i is below min SoC and charging", bus->get_identifier());
                        bus->command_power(chrgRate, 60, simTime, PowerType::e_ATCHARGER, true);
                        _totalCharge += chrgRate / 60;
                        pwrConsump += chrgRate;
                    }
                } else {
                    _totalCharge += chrgRate / 60;
                    pwrConsump += chrgRate;
                }
            }
        }
        (*chrgrsUsed)[chrgr.first] = plugsInUse;
        plugsInUse.clear();
    }

    return 0;
}


int
BusManager::handle_powerRequest(double& pwrConsump, std::map<ChargerPtr, std::map<PlugType, int>> *chrgrsUsed, double powerRequest, time_t simTime)
{
    int ret;
    double chrgRate, targetPwr;
    std::map<PlugType, int> numPlugs, plugsInUse;
    std::vector<Priority> priorities;
    std::map<BusPtr, bool> necessities;

    // Calculate Target to hit
    targetPwr = powerRequest - pwrConsump;

    // Order all available buses by priority
    for(auto& chrgr: _busSchedule){
        auto appendPriorities = _priorities[chrgr.first];
        priorities.insert(priorities.end(), appendPriorities.begin(), appendPriorities.end());

        auto appendNecessities = _necessities[chrgr.first];
        necessities.insert(appendNecessities.begin(), appendNecessities.end());
    }
    std::sort(priorities.begin(), priorities.end(), compare_priority);

    while ( std::fabs(targetPwr) >= 1e-5 && priorities.size() != 0 ){
        if ( targetPwr > 0.0 ){
            auto bus   = priorities.front().first;
            auto chargePriority = priorities.front().second;
            PlugType plugType = bus->get_plugType();
            auto chrgr = _busToCharger[bus];
            numPlugs   = chrgr->get_numPlugs();
            plugsInUse = (*chrgrsUsed)[chrgr];

            if ( necessities[bus] == false && plugsInUse[plugType] < numPlugs[plugType] ){
                (*chrgrsUsed)[chrgr][plugType]++;
                chrgRate = std::min(bus->get_chargeRate()*60, targetPwr);
                ret = bus->command_power(chrgRate, 60, simTime, PowerType::e_ATCHARGER);
                if ( ret != 0 ){
                    if ( ret == OVER_MAX_SOC ){
                        LOGDBG("%i:\tBus %s : Command would place bus over max SOC", simTime, bus->get_identifier().c_str());
                        double busSoc = bus->get_stateOfCharge();
                        double busCap = bus->get_capacity();
                        double busMaxSoc = bus->get_maxSoc();
                        double energyToCharge = (busMaxSoc - busSoc) * busCap; // kWh
                        bus->command_power(energyToCharge*60, 60, simTime, PowerType::e_ATCHARGER);
                        _totalCharge += energyToCharge;
                        pwrConsump += energyToCharge*60;
                        targetPwr -= energyToCharge*60;
                    }
                    else if ( ret == UNDER_MIN_SOC ){
                        LOGDBG("Bus %i is below min SoC and charging", bus->get_identifier());
                        bus->command_power(chrgRate, 60, simTime, PowerType::e_ATCHARGER, true);
                        _totalCharge += chrgRate / 60;
                        pwrConsump += chrgRate;
                        targetPwr -= chrgRate;
                    }
                } else {
                    _totalCharge += chrgRate / 60;
                    pwrConsump += chrgRate;
                    targetPwr -= chrgRate;
                }
            }

            priorities.erase(priorities.begin());
        } 
        else if ( targetPwr < 0.0 ){
            auto bus   = priorities.back().first;
            auto chargePriority = priorities.back().second;
            PlugType plugType = bus->get_plugType();
            auto chrgr = _busToCharger[bus];
            numPlugs   = chrgr->get_numPlugs();
            plugsInUse = (*chrgrsUsed)[chrgr];

            if ( necessities[bus] == false && chargePriority < 0.0 && plugsInUse[plugType] < numPlugs[plugType] ){
                (*chrgrsUsed)[chrgr][plugType]++;
                chrgRate = std::max(-bus->get_chargeRate()*60, targetPwr);
                chrgRate = std::max(chrgRate, chargePriority*bus->get_chargeRate());
                
                ret = bus->command_power(chrgRate, 60, simTime, PowerType::e_ATCHARGER);
                if ( ret != 0 ){
                    if ( ret == OVER_MAX_SOC ){
                        LOGDBG("Bus %i is above max SoC and discharging", bus->get_identifier());
                        bus->command_power(chrgRate, 60, simTime, PowerType::e_ATCHARGER, true);
                        _totalCharge += chrgRate / 60;
                        pwrConsump += chrgRate;
                        targetPwr -= chrgRate;
                    }
                    else if ( ret == UNDER_MIN_SOC ){
                        double busSoc = bus->get_stateOfCharge();
                        double busCap = bus->get_capacity();
                        double busMinSoc = bus->get_minSoc();
                        double energyToDschrg = (busSoc - busMinSoc) * busCap; // kWh
                        bus->command_power(energyToDschrg*60, 60, simTime, PowerType::e_ATCHARGER);
                        _totalCharge += energyToDschrg;
                        pwrConsump += energyToDschrg*60;
                        targetPwr -= energyToDschrg*60;
                    }
                } else {
                    _totalCharge += chrgRate / 60;
                    pwrConsump += chrgRate;
                    targetPwr -= chrgRate;
                }
            }

            priorities.erase(--priorities.end());
        }

        plugsInUse.clear();
    }

    return 0;
}


void
BusManager::handle_routes(time_t simTime)
{   
    bool firstRun = (simTime == 16200);
    if ( firstRun )
        return; // No departures for the first timestep

    int busId;
    double busEff, busTripDist, reqdEnrgForTrip;

    for(auto& chrgr: _busSchedule){
        std::vector<BusPtr> first = chrgr.second[simTime-60];
        std::vector<BusPtr> second = chrgr.second[simTime];
        std::vector<BusPtr> departures(first.size());

        // Pretty weird to sort by pointer address but it works
        std::sort(first.begin(), first.end());
        std::sort(second.begin(), second.end());

        auto it = std::set_difference(first.begin(), first.end(), second.begin(), second.end(), departures.begin());
        departures.resize(it-departures.begin());

        for (auto& bus: departures){
            busId = bus->get_identifier();
            // Get bus kWh,mi
            busEff = bus->get_consumptionRate();
            // Get bus next travel distance
            busTripDist = _nextTripDist[busId][simTime];
            // Calc necessary kWh to make next trip
            reqdEnrgForTrip = busTripDist * busEff;

            int ret = bus->command_power(-reqdEnrgForTrip, 3600, simTime, PowerType::e_ONROUTE);
            if ( ret != 0 )
                std::cout << simTime << ":\tBus " << busId << ": Not enough energy for route" << std::endl;
        }
    }
}


// Compares two priorities. 
bool 
BusManager::compare_priority(Priority lhs, Priority rhs){
    return (lhs.second > rhs.second);
}


int
BusManager::get_priorities(std::vector<Priority> &priorities, std::map<BusPtr, bool> &necessities, 
                            ChargerPtr charger, time_t simTime)
{
    int busId;
    double busSoc, busCap, busEff, busTripDist;
    double reqdEnrgForTrip, reqdEnrgBeforeTrip, reqdChrgRate, normPriority;
    std::map<BusPtr, int> nextDepart;

    // Get departure times for all buses at this charger
    nextDepart = _nextDepart[charger];

    for (auto& bus: _busSchedule[charger][simTime]){
        // Get bus ID
        busId = bus->get_identifier();
        // Get bus SOC
        busSoc = bus->get_stateOfCharge();
        // Get bus capacity
        busCap = bus->get_capacity();
        // Get bus kWh/mi
        busEff = bus->get_consumptionRate();
        // Get bus next travel distance
        busTripDist = _nextTripDist[busId][nextDepart[bus]];
        // Calc necessary kWh to make next trip
        reqdEnrgForTrip = busTripDist * busEff;
        // Calc kWh required minus kWh already have
        reqdEnrgBeforeTrip = reqdEnrgForTrip - (busSoc - bus->get_minSoc()) * busCap;
        // Calc necessary kWh/min to achieve necessary kWh before charge end time
        normPriority = (reqdEnrgBeforeTrip / ((nextDepart[bus] - simTime)/60)) / bus->get_chargeRate();
        // Push bus id and kWh/min to priorities vector
        priorities.push_back(Priority(bus, normPriority));
        // Calc necessary kWh/min for next time step to achieve necessary kWh before charge end time
        reqdChrgRate = reqdEnrgBeforeTrip / ((nextDepart[bus] - (simTime + 59.999999))/60);
        if ( reqdChrgRate > bus->get_chargeRate() ){
            necessities[bus] = true;
            /*std::cout << "Bus " << bus->get_identifier() << " needs to charge" << std::endl;
            std::cout << "\tBus Energy: " << (busSoc-0.1) * busCap 
                      << "\tTrip Energy: " << reqdEnrgForTrip 
                      << "\tCharge Rate: " << reqdChrgRate 
                      << "\tTime To Trip: " << ((nextDepart[bus] - simTime)/60) << std::endl;
        */}
        else
            necessities[bus] = false;        
    }
    // Sort in descending charge rate order
    std::sort(priorities.begin(), priorities.end(), compare_priority);
    
    /* for (auto& bus: priorities){
        std::cout << "Bus: " << bus.first->get_identifier() << ", \t" 
                  << std::fixed << std::setprecision(3) << bus.second << " kWh/min" << std::endl;
    } */

    return 0;
}


std::map<BUS::BusManager::BusPtr, int>
BusManager::get_nextDepartureTimes(ChargerPtr charger, int simTime)
{
    std::vector<BusPtr> primSet = _busSchedule[charger][simTime];
    std::vector<BusPtr> currSet;
    std::vector<BusPtr> departures(primSet.size());
    std::map<BusPtr, int> ret;

    while (primSet.size() > 0){
        simTime += 60;
        currSet = _busSchedule[charger][simTime];
        departures.resize(primSet.size());

        // Pretty weird to sort by pointer address but it works so ¯\_(ツ)_/¯
        std::sort(primSet.begin(), primSet.end());
        std::sort(currSet.begin(), currSet.end());

        auto it = std::set_difference(primSet.begin(), primSet.end(), currSet.begin(), currSet.end(), departures.begin());
        departures.resize(it-departures.begin());

        //if ( departures.size() >= 1 )
        //    std::cout << simTime << ": " << charger->get_name() << " Departures: " << departures.size() << std::endl << "\t";
        for (auto& bus: departures){
            ret[bus] = simTime;
            auto it = std::find (primSet.begin(), primSet.end(), bus);
            primSet.erase(it);
        }
    }

    return ret;
}


int
BusManager::get_nextDepartureTime(ChargerPtr charger, int busId, int simTime)
{
    std::map<int, std::vector<BusPtr>> chargingTimes; // Map of charging times and busses at those times
    chargingTimes = _busSchedule[charger];

    int srchTime = simTime;
    BusPtr srchBus = _buses[busId];
    auto it = std::find(chargingTimes[srchTime].begin(), chargingTimes[srchTime].end(), srchBus);
    assert(it != chargingTimes[srchTime].end());

    // Find where bus has left the charging station
    while( it != chargingTimes[srchTime].end() ){
        srchTime += 60;
        it = std::find(chargingTimes[srchTime].begin(), chargingTimes[srchTime].end(), srchBus);
    }

    return srchTime;
}


} /** namespace BUS */


BOOST_PYTHON_MODULE(BusManager)
{
    bpn::initialize();
    Py_Initialize();

    bp::class_<BUS::BusManager>("BusManager")
        .def("init_chargers", &BUS::BusManager::init_chargers)
        .def("init_buses",    &BUS::BusManager::init_buses)
        .def("init_schedule", &BUS::BusManager::init_schedule)
        .def("run",           &BUS::BusManager::run)
        .def("file_dump",     &BUS::BusManager::file_dump)
        .def("clear_memory",  &BUS::BusManager::clear_memory)
    ;
}
