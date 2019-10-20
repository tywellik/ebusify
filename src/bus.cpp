#include "bus.hpp"
#include "error.hpp"
#include <math.h>
#include <iostream>

namespace BUS {

Bus::Bus(int id, double capacity, double consumptionRate, double chargeRate, double distFirstCharge, PlugType plugType)
:
    _identifier(id),
    _capacity(capacity),
    _consumptionRate(consumptionRate),
    _chargeRate(chargeRate),
    _distFirstCharge(distFirstCharge),
    _minSoc(0.1),
    _maxSoc(0.9),
    _plugType(plugType)
{
    _stateOfCharge = 0.5;
    _stateOfCharge -= (distFirstCharge * consumptionRate)/capacity;
    _socTime[16200] = _stateOfCharge;
    _consumpChargerTime[16200] = 0.0;
    _consumpRouteTime[16200] = distFirstCharge * consumptionRate;
    _lastTsRun = 16200;
}


Bus::~Bus()
{}


int
Bus::init_soc(double stateOfCharge)
{
    _stateOfCharge = stateOfCharge;

    return 0;
}


int 
Bus::command_power(double power, double timestep, int simTime, BUS::PowerType pt, bool force)
{
    // Fill in SoC for timesteps that bus was not commanded power
    int tsDiff;
    for (tsDiff = simTime - _lastTsRun - 60; tsDiff > 0; tsDiff-=60){
        _socTime[simTime-tsDiff] = _socTime[_lastTsRun];
        _consumpChargerTime[simTime-tsDiff] = 0.0;
        _consumpRouteTime[simTime-tsDiff] = 0.0;
    }

    double deltaEnergy, newSoc;
    deltaEnergy = power * timestep / 3600;
    newSoc = _stateOfCharge + (deltaEnergy / _capacity);
    if ( newSoc > _maxSoc && !force )
        return OVER_MAX_SOC;
    else if ( newSoc < _minSoc && !force )
        return UNDER_MIN_SOC;

    switch( pt ){
        case PowerType::e_ATCHARGER:
            _consumpChargerTime[simTime] = deltaEnergy;
            _consumpRouteTime[simTime]   = 0.0;
            break;
        case PowerType::e_ONROUTE:
            _consumpChargerTime[simTime] = 0.0;
            _consumpRouteTime[simTime]   = -deltaEnergy;
            break;
        default:
            _consumpChargerTime[simTime] = 0.0;
            _consumpRouteTime[simTime]   = 0.0;
            break;
    }

    _stateOfCharge = newSoc;
    _socTime[simTime] = _stateOfCharge;
    _lastTsRun = simTime;
    return 0;
}


double
Bus::get_stateOfCharge(int ts) const 
{
    auto it = _socTime.find(ts);
    if ( it == _socTime.end() )
        return _socTime.find(_lastTsRun)->second;
    
    return it->second;
}


double
Bus::get_consumpCharger(int ts) const 
{
    auto it = _consumpChargerTime.find(ts);
    if ( it == _consumpChargerTime.end() )
        return 0.0;
    
    return it->second;
}


double
Bus::get_consumpRoute(int ts) const 
{
    auto it = _consumpRouteTime.find(ts);
    if ( it == _consumpRouteTime.end() )
        return 0.0;
    
    return it->second;
}


} /** namespace */
