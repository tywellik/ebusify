#include "energy_source.hpp"


namespace NRG {

class SolarPlant : public EnergySource 
{
public:
    SolarPlant(struct EnergySourceParameters const &esp);
    ~SolarPlant();

    int get_emissionsOutput(Emissions& emissions) override;

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
SolarPlant::get_emissionsOutput(Emissions& emissions)
{
    emissions.carbonDioxide = _currPowerOutput * 0;
    emissions.methane       = _currPowerOutput * 0;
    emissions.nitrousOxide  = _currPowerOutput * 0;

    return SUCCESS;
}


int
SolarPlant::get_productionCost(float &cost, float powerRequest)
{
    cost = powerRequest * _runCost / 3600;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_SolarPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::SolarPlant(esp);
}
