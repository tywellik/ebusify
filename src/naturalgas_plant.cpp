#include "energy_source.hpp"


namespace NRG {

class NaturalGasPlant : public EnergySource 
{
public:
    NaturalGasPlant(struct EnergySourceParameters const &esp);
    ~NaturalGasPlant();

    int get_emissionsOutput(Emissions& emissions) override;

    int get_productionCost(float &cost, float powerRequest) override;

private:
    NaturalGasPlant(const NaturalGasPlant&) = delete;
};


NaturalGasPlant::NaturalGasPlant(struct EnergySourceParameters const &esp) 
: 
    EnergySource::EnergySource(esp)
{}


NaturalGasPlant::~NaturalGasPlant()
{}


int
NaturalGasPlant::get_emissionsOutput(Emissions& emissions)
{
    emissions.carbonDioxide = _currPowerOutput / 3600 * 505;
    emissions.methane       = _currPowerOutput / 3600 * 0;
    emissions.nitrousOxide  = _currPowerOutput / 3600 * 0;

    return SUCCESS;
}


int
NaturalGasPlant::get_productionCost(float &cost, float powerRequest)
{
    cost = powerRequest * _runCost / 3600;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_NaturalGasPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::NaturalGasPlant(esp);
}
