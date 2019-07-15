#include "energy_source.hpp"

namespace NRG {

EnergySource::EnergySource(struct EnergySourceParameters const &esp)
:
    _name(esp.name),
    _maxOutputPower(esp.maxCapacity),
    _minOutputPower(esp.minCapacity),
    _maxPositiveRamp(esp.rampRate),
    _maxNegativeRamp(esp.rampRate),
    _runCost(esp.runCost),
    _rampCost(esp.rampCost),
    _startupCost(esp.startupCost),
    _currPowerOutput(0.0),
    _currState(SourceState::e_SSOFF)
{}


EnergySource::~EnergySource()
{}


std::string
EnergySource::get_name() const
{
    return _name;
}


double
EnergySource::get_powerCost() const
{
    return _runCost;
}


double
EnergySource::get_maxOutputPower() const
{
    return _maxOutputPower;
}


double
EnergySource::get_minOutputPower() const
{
    return (_maxOutputPower * (_minOutputPower / 100.0)); // Min Output Power is a percentage of max output power
}


double
EnergySource::get_maxPositiveRamp() const
{
    return _maxPositiveRamp / 100.0;
}


double
EnergySource::get_maxNegativeRamp() const
{
    return _maxNegativeRamp / 100.0;
}


double
EnergySource::get_currPower() const
{
    return _currPowerOutput;
}


void
EnergySource::set_powerPoint(double power, bool overrideRamps)
{
    double setPower;
    if (!overrideRamps) {
        double maxPosChange = _maxOutputPower * _maxPositiveRamp;
        double maxNegChange = _maxOutputPower * _maxNegativeRamp;
        setPower = std::max(std::min(_currPowerOutput + maxPosChange, power), _currPowerOutput - maxNegChange);
    }
    else
        setPower = power;
    
    _currPowerOutput = setPower;
}


void
EnergySource::reset()
{
    _currPowerOutput = 0.0;
    _currState       = SourceState::e_SSOFF;
}

} // namespace NRG