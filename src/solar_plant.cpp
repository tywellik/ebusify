#include "energy_source.hpp"


namespace NRG {

class SolarPlant : public EnergySource 
{
public:
    SolarPlant(struct EnergySourceParameters const &esp);
    ~SolarPlant();

    int get_emissionsOutput(Emissions& emissions, float powerRequest) override;

    int get_productionCost(float &cost, float powerRequest) override;

private:
    SolarPlant(const SolarPlant&) = delete;
};


SolarPlant::SolarPlant(struct EnergySourceParameters const &esp) 
: 
    EnergySource::EnergySource(esp)
{}


SolarPlant::~SolarPlant()
{}


int
SolarPlant::get_emissionsOutput(Emissions& emissions, float powerRequest)
{
    emissions.carbonDioxide = _currPowerOutput * .099;
    emissions.methane       = _currPowerOutput * .0001;
    emissions.nitrousOxide  = _currPowerOutput * .0009;

    return SUCCESS;
}


int
SolarPlant::get_productionCost(float &cost, float powerRequest)
{
    cost = powerRequest * _runCost;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_SolarPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::SolarPlant(esp);
}
