#include "bus_manager.hpp"
#include "error.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>


//#define VERBOSE
#define LOGERR(fmt, args...)   do{ fprintf(stderr, fmt "\n", ##args); }while(0)
#ifdef VERBOSE
    #define LOGDBG(fmt, args...)   do{ fprintf(stdout, fmt "\n", ##args); }while(0)
#else
    #define LOGDBG(fmt, args...)   do{}while(0)
#endif


namespace BUS {

BusManager::BusManager()
{}


BusManager::~BusManager()
{}


int
BusManager::init_chargers(bpn::ndarray const& chargerIds, bpn::ndarray const& chargerNames,
                        bpn::ndarray const& numberPlugs)
{
    unsigned int dataLen = chargerIds.shape(0);
    if (dataLen != chargerNames.shape(0)  || dataLen != numberPlugs.shape(0)){
        PyErr_SetString(PyExc_TypeError, "Bus data lengths inconsistent");
        bp::throw_error_already_set();
    }

    int* chrgIds   = reinterpret_cast<int*>(chargerIds.get_data());
    int* numPlugs  = reinterpret_cast<int*>(numberPlugs.get_data());

    //std::map<int, std::shared_ptr<Charger>> _chargers;
    for (int line = 0; line < dataLen; ++line){
        int chrgId = chrgIds[line];
        std::string chargerName = std::string(bp::extract<char const *>(chargerNames[line]));
        std::shared_ptr<Charger> chrgPtr;

        // If charging station is new, add it to the map
        if ( _chargers.find(chrgId) == _chargers.end() ){
            Charger *charger = new Charger(chrgId, chargerName, numPlugs[line]);
            chrgPtr.reset(charger);    
            _chargers.insert(std::pair<int, std::shared_ptr<Charger>>(chrgId, chrgPtr));
        } else {
            chrgPtr = _chargers[chrgId];
        }
    }
}


int 
BusManager::init_buses(bpn::ndarray const& busIdentifiers, bpn::ndarray const& capacities,
                    bpn::ndarray const& consumptionRates, bpn::ndarray const& chargeRates,
                    bpn::ndarray const& distFirstCharge)
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
        // If bus is new, add it to the map
        if ( _buses.find(busIds[line]) == _buses.end() ){
            std::shared_ptr<Bus> busPtr;
            Bus *bus = new Bus(busIds[line], caps[line], consumpRates[line], chrgRates[line], distFirstChrg[line]);
            busPtr.reset(bus);
            _buses.insert(std::pair<int, std::shared_ptr<Bus>>(busIds[line], busPtr));
        }
    }

    /*
    for (auto& bus : _buses)
    {
        std::cout << "Bus " << bus.first << std::endl
            << "\tState of Charge:  " << bus.second->get_stateOfCharge()   << std::endl
            << "\tCapacity:         " << bus.second->get_capacity()        << std::endl
            << "\tCharge Rate:      " << bus.second->get_chargeRate()      << std::endl
            << "\tConsumption Rate: " << bus.second->get_consumptionRate() << std::endl
            << "\tFirst Chrg Dist:  " << bus.second->get_distFirstCharge() << std::endl;
    }
    */
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
        std::shared_ptr<Charger> chrgPtr;
        std::shared_ptr<Bus> busPtr;

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

        _nextTripDist[busIds[line]][chrgEnd] = distNextChrg[line];
    }
}


int
BusManager::run(double powerRequest, time_t simTime)
{
    if ( (simTime % 3600) == 0)
        std::cout << "Sim Time: " << simTime << std::endl;

    handle_charging(powerRequest, simTime);
    //handle_discharge();
    handle_routes(simTime);

    return 0;
}


void
BusManager::file_dump()
{
    std::ofstream outfile;
    outfile.open("charger_usage.csv");

    outfile << ",";
    for (auto& chrgr: *_chrgrsUsedTime[0])
        outfile << chrgr.first->get_name() << ",";
    outfile << std::endl;

    int simTime = 16200;
    for (auto& chrgrMap: _chrgrsUsedTime){
        //std::cout << "Time: " << simTime << std::endl;
        outfile << simTime << ",";
        for (auto& chrgr: *chrgrMap){
            //std::cout << "\t" << chrgr.first->get_name() << ": " << chrgr.second << std::endl;
            outfile << chrgr.second << ",";
        }
        outfile << std::endl;

        simTime += 60;
    }
    outfile.close();

    outfile.open("bus_soc.csv");
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

    return;
}


void
BusManager::handle_charging(double powerRequest, time_t simTime)
{
    int numPlugs, plugsInUse, ret;
    double chrgRate;
    std::vector<Priority> priorities;
    std::shared_ptr<std::map<std::shared_ptr<Charger>, int>> chrgrsUsedPtr;
    std::map<std::shared_ptr<Charger>, int> *chrgrsUsed = new std::map<std::shared_ptr<Charger>, int>;
    chrgrsUsedPtr.reset(chrgrsUsed);

    for(auto& chrgr: _busSchedule){
        //std::cout << "Charger: " << chrgr.first->get_name() << "\t";
        //std::cout << "Charger: " << chrgr.first->get_name() << "\n";

        numPlugs = chrgr.first->get_numPlugs();
        plugsInUse = 0;
        priorities = get_priorities(chrgr.first, simTime);

        for (auto& priority : priorities){
            std::shared_ptr<Bus> bus = priority.first;
            double chargePriority = priority.second;

            if ( chargePriority > 0.0 && plugsInUse < numPlugs){
                plugsInUse++;
                chrgRate = bus->get_chargeRate(); // kWh / min
                chrgRate *= 60; // kW
                ret = bus->command_power(chrgRate, 60, simTime);
                if ( ret != 0 ){
                    if ( ret == OVER_MAX_SOC ){
                        double busSoc = bus->get_stateOfCharge();
                        double busCap = bus->get_capacity();
                        double busMaxSoc = bus->get_maxSoc();
                        double energyToCharge = (busMaxSoc - busSoc) * busCap; // kWh
                        bus->command_power(energyToCharge*60, 60, simTime);
                    }
                    else if ( ret == UNDER_MIN_SOC ){
                        LOGDBG("Bus %i is below min SoC and charging", bus->get_identifier());
                        bus->command_power(chrgRate, 60, simTime, true);
                    }
                }
            }
        }

        //std::cout << "Chargers in use: " << plugsInUse << std::endl << std::endl;
        (*chrgrsUsed)[chrgr.first] = plugsInUse;
    }

    _chrgrsUsedTime.push_back(chrgrsUsedPtr);
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
        std::vector<std::shared_ptr<Bus>> first = chrgr.second[simTime-60];
        std::vector<std::shared_ptr<Bus>> second = chrgr.second[simTime];
        std::vector<std::shared_ptr<Bus>> departures(first.size());

        // Pretty weird to sort by pointer address but it works
        std::sort(first.begin(), first.end());
        std::sort(second.begin(), second.end());

        auto it = std::set_difference(first.begin(), first.end(), second.begin(), second.end(), departures.begin());
        departures.resize(it-departures.begin());

        //if ( departures.size() >= 1 )
            //std::cout << chrgr.first->get_name() << " Departures: " << departures.size() << std::endl << "\t";
        for (auto& bus: departures){
            busId = bus->get_identifier();
            // Get bus kWh,mi
            busEff = bus->get_consumptionRate();
            // Get bus next travel distance
            busTripDist = _nextTripDist[busId][simTime];
            // Calc necessary kWh to make next trip
            reqdEnrgForTrip = busTripDist * busEff;

            //std::cout << busId << ": " << reqdEnrgForTrip << " kWh, ";

            int ret = bus->command_power(-reqdEnrgForTrip, 3600, simTime);
            if ( ret != 0 )
                std::cout << "Bus " << busId << ": Not enough energy for route" << std::endl;
        }
        //if ( departures.size() >= 1 )
            //std::cout << std::endl;
    }
}


// Compares two bus pointers
bool
BusManager::compare_busPtrs(std::shared_ptr<Bus> lhs, std::shared_ptr<Bus> rhs){
    return (lhs->get_identifier() < rhs->get_identifier());
}


// Compares two priorities. 
bool 
BusManager::compare_priority(Priority lhs, Priority rhs){
    return (lhs.second > rhs.second);
}


std::vector<BusManager::Priority>
BusManager::get_priorities(std::shared_ptr<Charger> charger, time_t simTime)
{
    int busId;
    double busSoc, busCap, busEff, busTripDist;
    double reqdEnrgForTrip, reqdEnrgBforTrip, reqdChrgRate;
    std::map<std::shared_ptr<Bus>, int> nextDepart;

    // Get departure times for all buses at this charger
    nextDepart = get_nextDepartureTimes(charger, simTime);

    std::vector<Priority> priorities;
    for (auto& bus: _busSchedule[charger][simTime]){
        // Get bus ID
        busId = bus->get_identifier();
        // Get bus SOC
        busSoc = bus->get_stateOfCharge();
        // Get bus capacity
        busCap = bus->get_capacity();
        // Get bus kWh,mi
        busEff = bus->get_consumptionRate();
        // Get bus next travel distance
        busTripDist = _nextTripDist[busId][nextDepart[bus]];
        // Calc necessary kWh to make next trip
        reqdEnrgForTrip = busTripDist * busEff * 1.5;
        // Calc kWh required minus kWh already have
        reqdEnrgBforTrip = reqdEnrgForTrip - (busSoc - bus->get_minSoc()) * busCap;
        // Calc necessary kWh/min to achieve necessary kWh before charge end time
        reqdChrgRate = reqdEnrgBforTrip / ((nextDepart[bus] - simTime)/60);
        // Push bus id and kWh/min to priorities vector
        priorities.push_back(Priority(bus, reqdChrgRate));
    }
    // Sort in descending charge rate order
    std::sort(priorities.begin(), priorities.end(), compare_priority);
    
    /* for (auto& bus: priorities){
        std::cout << "Bus: " << bus.first->get_identifier() << ", \t" 
                  << std::fixed << std::setprecision(3) << bus.second << " kWh/min" << std::endl;
    } */

    return priorities;
}


std::map<std::shared_ptr<Bus>, int>
BusManager::get_nextDepartureTimes(std::shared_ptr<Charger> charger, int simTime)
{
    std::vector<std::shared_ptr<Bus>> primSet = _busSchedule[charger][simTime];
    std::vector<std::shared_ptr<Bus>> currSet;
    std::vector<std::shared_ptr<Bus>> departures(primSet.size());
    std::map<std::shared_ptr<Bus>, int> ret;

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
BusManager::get_nextDepartureTime(std::shared_ptr<Charger> charger, int busId, int simTime)
{
    std::map<int, std::vector<std::shared_ptr<Bus>>> chargingTimes; // Map of charging times and busses at those times
    chargingTimes = _busSchedule[charger];

    int srchTime = simTime;
    std::shared_ptr<Bus> srchBus = _buses[busId];
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
    ;
}
