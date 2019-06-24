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
    using Priority = std::pair<std::shared_ptr<Bus>, double>;
    
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

    int run(double powerRequest, time_t simTime);

    void file_dump();

private:
    std::map<int, std::shared_ptr<Bus>> _buses;
    std::map<int, std::shared_ptr<Charger>> _chargers;
    std::map<int, std::map<int, double>> _nextTripDist;
    std::map<std::shared_ptr<Charger>, std::map<int, std::vector<std::shared_ptr<Bus>>>> _busSchedule;

    // Time Series Data per Charging Station
    std::vector<std::shared_ptr<std::map<std::shared_ptr<Charger>, int>>> _chrgrsUsedTime;
    std::vector<std::shared_ptr<std::map<std::shared_ptr<Charger>, double>>> _energyChargedTime;

    void handle_charging(double powerRequest, time_t simTime);
    void handle_routes(time_t simTime);

    static bool compare_busPtrs(std::shared_ptr<Bus> lhs, std::shared_ptr<Bus> rhs);
    static bool compare_priority(Priority lhs, Priority rhs);

    /** Returns a list of buses that require charging sorted by the rate at which they need to charge in kWh/min */
    std::vector<Priority> get_priorities(std::shared_ptr<Charger> charger, time_t simTime);

    int get_nextDepartureTime(std::shared_ptr<Charger> charger, int busId, int simTime);
    std::map<std::shared_ptr<Bus>, int> get_nextDepartureTimes(std::shared_ptr<Charger> charger, int simTime);
};

}


#endif /** BUSMANAGER_H */