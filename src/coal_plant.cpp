#include "energy_source.hpp"


namespace NRG {

class CoalPlant : public EnergySource 
{
public:
    CoalPlant(struct EnergySourceParameters const &esp);
    ~CoalPlant();

    EnergySource::Emissions get_emissionsOutput() override;

    double get_productionCost(double powerRequest) override;

private:
    CoalPlant(const CoalPlant&) = delete;
};


CoalPlant::CoalPlant(struct EnergySourceParameters const &esp) 
: 
    EnergySource::EnergySource(esp)
{}


CoalPlant::~CoalPlant()
{}


EnergySource::Emissions
CoalPlant::get_emissionsOutput()
{
    Emissions emissions = {
        .carbonDioxide = _currPowerOutput * 960.6,
        .methane       = _currPowerOutput * 0,
        .nitrousOxide  = _currPowerOutput * 0
    };

    return emissions;
}


double
CoalPlant::get_productionCost(double powerRequest)
{
    return powerRequest * _runCost / 3600;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_CoalPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::CoalPlant(esp);
}
