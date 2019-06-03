#include "energy_source.hpp"


namespace NRG {

class HydroPlant : public EnergySource 
{
public:
    HydroPlant(struct EnergySourceParameters const &esp);
    ~HydroPlant();

    int get_emissionsOutput(Emissions& emissions, float powerRequest) override;

    int get_productionCost(float &cost, float powerRequest) override;

private:
    HydroPlant(const HydroPlant&) = delete;
};


HydroPlant::HydroPlant(struct EnergySourceParameters const &esp) 
: 
    EnergySource::EnergySource(esp)
{}


HydroPlant::~HydroPlant()
{}


int
HydroPlant::get_emissionsOutput(Emissions& emissions, float powerRequest)
{
    emissions.carbonDioxide = 0;
    emissions.methane       = 0;
    emissions.nitrousOxide  = 0;

    return SUCCESS;
}


int
HydroPlant::get_productionCost(float &cost, float powerRequest)
{
    cost = powerRequest * _runCost;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_HydroPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::HydroPlant(esp);
}
