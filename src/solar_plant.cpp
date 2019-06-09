#include "energy_source.hpp"
#include "utility_manager.hpp"


namespace NRG {

class SolarPlant : public EnergySource 
{
public:
    SolarPlant(UtilityManager* um, struct EnergySourceParameters const &esp);
    ~SolarPlant();

    EnergySource::Emissions get_emissionsOutput() override;

    double get_productionCost(double powerRequest) override;

private:
    UtilityManager* _um;

    SolarPlant(const SolarPlant&) = delete;
};


SolarPlant::SolarPlant(UtilityManager* um, struct EnergySourceParameters const &esp) 
: 
    EnergySource::EnergySource(esp),
    _um(um)
{
    _um->register_uncontrolledSource(get_name());
}


SolarPlant::~SolarPlant()
{}


EnergySource::Emissions
SolarPlant::get_emissionsOutput()
{
    EnergySource::Emissions emissions = {
        .carbonDioxide = _currPowerOutput * 0,
        .methane       = _currPowerOutput * 0,
        .nitrousOxide  = _currPowerOutput * 0
    };

    return emissions;
}


double
SolarPlant::get_productionCost(double powerRequest)
{
    return powerRequest * _runCost / 3600;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_SolarPlant(UtilityManager* um, NRG::EnergySourceParameters const &esp)
{
    return new NRG::SolarPlant(um, esp);
}
