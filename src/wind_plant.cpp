#include "energy_source.hpp"


namespace NRG {

class WindPlant : public EnergySource 
{
public:
    WindPlant(struct EnergySourceParameters const &esp);
    ~WindPlant();

    int get_emissionsOutput(Emissions& emissions) override;

    int get_productionCost(float &cost, float powerRequest) override;

private:
    WindPlant(const WindPlant&) = delete;
};


WindPlant::WindPlant(struct EnergySourceParameters const &esp) 
: 
    EnergySource::EnergySource(esp)
{}


WindPlant::~WindPlant()
{}


int
WindPlant::get_emissionsOutput(Emissions& emissions)
{
    emissions.carbonDioxide = 0;
    emissions.methane       = 0;
    emissions.nitrousOxide  = 0;

    return SUCCESS;
}


int
WindPlant::get_productionCost(float &cost, float powerRequest)
{
    cost = powerRequest * _runCost / 3600;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_WindPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::WindPlant(esp);
}
