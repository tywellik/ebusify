#include "bus.hpp"
#include "error.hpp"

namespace BUS {

Bus::Bus(int id, double capacity, double consumptionRate, double chargeRate, double distFirstCharge)
:
    _identifier(id),
    _capacity(capacity),
    _consumptionRate(consumptionRate),
    _chargeRate(chargeRate),
    _distFirstCharge(distFirstCharge),
    _minSoc(0.1),
    _maxSoc(0.9)
{
    _stateOfCharge = 0.5;
    _stateOfCharge -= (distFirstCharge * consumptionRate)/capacity;
    _socTime[16200] = _stateOfCharge;
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
Bus::command_power(double power, double timestep, int simTime, bool force)
{
    // Fill in SoC for timesteps that bus was not commanded power
    int tsDiff;
    for (tsDiff = simTime - _lastTsRun - 60; tsDiff > 0; tsDiff-=60){
        _socTime[simTime-tsDiff] = _socTime[_lastTsRun];
    }

    double deltaEnergy, newSoc;
    deltaEnergy = power * timestep / 3600;
    newSoc = _stateOfCharge + (deltaEnergy / _capacity);
    if ( !force && newSoc > _maxSoc )
        return OVER_MAX_SOC;
    else if ( !force && newSoc < _minSoc )
        return UNDER_MIN_SOC;

    _stateOfCharge = newSoc;
    _socTime[simTime] = _stateOfCharge;
    _lastTsRun = simTime;
    return 0;
}


} /** namespace */
