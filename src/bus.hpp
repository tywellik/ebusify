#ifndef BUS_H
#define BUS_H
#include <map>
#include "charger.hpp"

namespace BUS {

enum class PowerType {
    e_ONROUTE = 1,
    e_ATCHARGER = 2,
    e_END
};

class Bus
{
public:
    Bus(int id, double capacity, double consumptionRate, double chargeRate, double distFirstCharge, PlugType plugType);
    ~Bus();
    
    bool operator <(Bus const &obj) {
         return _identifier < obj.get_identifier();
    }

    int init_soc(double stateOfCharge);

    int    get_identifier() const {return _identifier;}
    double get_capacity() const {return _capacity;}
    double get_consumptionRate() const {return _consumptionRate;}
    double get_chargeRate() const {return _chargeRate;}
    double get_distFirstCharge() const {return _distFirstCharge;}
    double get_stateOfCharge() const {return _stateOfCharge;}
    double get_stateOfCharge(int ts) const;
    double get_consumpCharger(int ts) const;
    double get_consumpRoute(int ts) const;
    double get_minSoc() const {return _minSoc;}
    double get_maxSoc() const {return _maxSoc;}
    PlugType get_plugType() const {return _plugType;}

    int command_power(double power, double timestep, int simTime, PowerType pt, bool force = false);

private:
    int    _identifier;
    double _capacity;
    double _consumptionRate;
    double _chargeRate;
    double _distFirstCharge;
    double _minSoc;
    double _maxSoc;
    PlugType _plugType;

    double _stateOfCharge;
    std::map<int, double> _socTime;
    std::map<int, double> _consumpChargerTime;
    std::map<int, double> _consumpRouteTime;
    int _lastTsRun;
};

}


#endif /** BUS_H */