#ifndef BUSMANAGER_H
#define BUSMANAGER_H

#include "bus.hpp"
#include "charger.hpp"

#include <map>
#include <boost/python.hpp>
#include <boost/python/numpy.hpp>
#include <boost/python/stl_iterator.hpp>

namespace bp  = boost::python;
namespace bpn = boost::python::numpy;

namespace BUS {

class BusManager 
{
public:
    using BusPtr     = std::shared_ptr<Bus>;
    using ChargerPtr = std::shared_ptr<Charger>;
    using Priority   = std::pair<BusPtr, double>;
    BusManager();
    ~BusManager();

    int init_chargers(bpn::ndarray const& chargerId, bpn::ndarray const& chargerName,
                    bpn::ndarray const& numberPlugs);

    int init_buses(bpn::ndarray const& busIds, bpn::ndarray const& capacities,
                    bpn::ndarray const& consumptionRates, bpn::ndarray const& chargeRates,
                    bpn::ndarray const& distFirstCharge); 

    int init_schedule(bpn::ndarray const& routeIds, bpn::ndarray const& busIds,
                    bpn::ndarray const& chargeStart, bpn::ndarray const& chargeEnd,
                    bpn::ndarray const& distNextChrg, bpn::ndarray const& chargerId);

    int run(double powerRequest, int mode, time_t simTime);

    void file_dump();

private:
    double _totalCharge;
    std::map<int, BusPtr> _buses;
    std::map<int, ChargerPtr> _chargers;
    std::map<ChargerPtr, std::map<int, std::vector<BusPtr>>> _busSchedule;

    // Unique for each timestep
    std::map<BusPtr, ChargerPtr> _busToCharger;
    std::map<int, std::map<int, double>> _nextTripDist;
    std::map<ChargerPtr, std::map<BusPtr, int>> _nextDepart;
    std::map<ChargerPtr, std::vector<Priority>> _priorities;
    std::map<ChargerPtr, std::map<BusPtr, bool>> _necessities;

    // Time Series Data per Charging Station
    std::vector<std::shared_ptr<std::map<ChargerPtr, int>>> _chrgrsUsedTime;
    std::vector<std::shared_ptr<std::map<ChargerPtr, double>>> _energyChargedTime;

    
    int handle_necessaryCharging(double& pwrConsump, std::map<ChargerPtr, int> *chrgrsUsed, time_t simTime);
    int handle_remainingCharging(double& pwrConsump, std::map<ChargerPtr, int> *chrgrsUsed, time_t simTime);
    int handle_powerRequest(double& pwrConsump, std::map<ChargerPtr, int> *chrgrsUsed, double powerRequest, time_t simTime);
    void handle_charging(double powerRequest, time_t simTime);
    void handle_routes(time_t simTime);

    static bool compare_priority(Priority lhs, Priority rhs);

    /** Returns a list of buses that require charging sorted by the rate at which they need to charge in kWh/min */
    int get_priorities(std::vector<Priority> &priorities, std::map<BusPtr, bool> &necessities, 
                        ChargerPtr charger, time_t simTime);

    int get_nextDepartureTime(ChargerPtr charger, int busId, int simTime);
    std::map<BusPtr, int> get_nextDepartureTimes(ChargerPtr charger, int simTime);
};

}


#endif /** BUSMANAGER_H */