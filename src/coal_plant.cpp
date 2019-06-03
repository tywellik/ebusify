#include "energy_source.hpp"


namespace NRG {

class CoalPlant : public EnergySource 
{
public:
    CoalPlant(struct EnergySourceParameters const &esp);
    ~CoalPlant();

    int get_emissionsOutput(Emissions& emissions, float powerRequest) override;

    int get_productionCost(float &cost, float powerRequest) override;

private:
    CoalPlant(const CoalPlant&) = delete;
};


CoalPlant::CoalPlant(struct EnergySourceParameters const &esp) 
: 
    EnergySource::EnergySource(esp)
{}


CoalPlant::~CoalPlant()
{}


int
CoalPlant::get_emissionsOutput(Emissions& emissions, float powerRequest)
{
    emissions.carbonDioxide = _currPowerOutput / 3600 * 960.6;
    emissions.methane       = _currPowerOutput / 3600 * 0;
    emissions.nitrousOxide  = _currPowerOutput / 3600 * 0;

    return SUCCESS;
}


int
CoalPlant::get_productionCost(float &cost, float powerRequest)
{
    cost = powerRequest * _runCost;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_CoalPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::CoalPlant(esp);
}
