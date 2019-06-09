#include "energy_source.hpp"


namespace NRG {

class HydroPlant : public EnergySource 
{
public:
    HydroPlant(struct EnergySourceParameters const &esp);
    ~HydroPlant();

    EnergySource::Emissions get_emissionsOutput() override;

    double get_productionCost(double powerRequest) override;

private:
    HydroPlant(const HydroPlant&) = delete;
};


HydroPlant::HydroPlant(struct EnergySourceParameters const &esp) 
: 
    EnergySource::EnergySource(esp)
{}


HydroPlant::~HydroPlant()
{}


EnergySource::Emissions
HydroPlant::get_emissionsOutput()
{
    EnergySource::Emissions emissions = {
        .carbonDioxide = _currPowerOutput * 0,
        .methane       = _currPowerOutput * 0,
        .nitrousOxide  = _currPowerOutput * 0
    };

    return emissions;
}


double
HydroPlant::get_productionCost(double powerRequest)
{
    return powerRequest * _runCost / 3600;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_HydroPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::HydroPlant(esp);
}
