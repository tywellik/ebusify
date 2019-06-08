#include "energy_source.hpp"

#define LOGERR(fmt, args...)   do{ fprintf(stderr, fmt "\n", ##args); }while(0)
#define LOGDBG(fmt, args...)   do{ fprintf(stdout, fmt "\n", ##args); }while(0)


namespace NRG {

EnergySource::EnergySource(struct EnergySourceParameters const &esp)
:
    _name(esp.name),
    _maxOutputPower(esp.maxCapacity),
    _minOutputPower(esp.minCapacity),
    _maxPositiveRamp(esp.rampRate),
    _maxNegativeRamp(esp.rampRate),
    _runCost(esp.runCost),
    _currPowerOutput(0.0)//esp.maxCapacity * esp.minCapacity)
{}


EnergySource::~EnergySource()
{}


std::string
EnergySource::get_name() const
{
    return _name;
}


float
EnergySource::get_powerCost() const
{
    return _runCost;
}


float
EnergySource::get_maxOutputPower() const
{
    return _maxOutputPower;
}


float
EnergySource::get_minOutputPower() const
{
    return (_maxOutputPower * (_minOutputPower / 100.0)); // Min Output Power is a percentage of max output power
}


float
EnergySource::get_maxPositiveRamp() const
{
    return _maxPositiveRamp;
}


float
EnergySource::get_maxNegativeRamp() const
{
    return _maxNegativeRamp;
}


float
EnergySource::get_currPower() const
{
    return _currPowerOutput;
}


void
EnergySource::set_powerPoint(float power)
{
    float maxPosChange = _maxOutputPower * _maxPositiveRamp;
    float maxNegChange = _maxOutputPower * _maxNegativeRamp;
    float setPower = std::max(std::min(_currPowerOutput + maxPosChange, _currPowerOutput), _currPowerOutput - maxNegChange);

    _currPowerOutput = setPower;
}

} // namespace NRG