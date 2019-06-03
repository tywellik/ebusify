#include "energy_source.hpp"

namespace NRG {

EnergySource::EnergySource(struct EnergySourceParameters const &esp)
:
    _maxOutputPower(esp.maxCapacity),
    _minOutputPower(esp.minCapacity),
    _maxPositiveRamp(esp.rampRate),
    _maxNegativeRamp(esp.rampRate),
    _runCost(esp.runCost)
{}


EnergySource::~EnergySource()
{}


float
EnergySource::get_maxOutputPower() const
{
    return _maxOutputPower;
}


float
EnergySource::get_minOutputPower() const
{
    return _minOutputPower;
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


void
EnergySource::set_powerPoint(float power)
{
    float maxPosChange = _maxOutputPower * _maxPositiveRamp;
    float maxNegChange = _maxOutputPower * _maxNegativeRamp;
    //std::min(std::max(,), )
}

} // namespace NRG