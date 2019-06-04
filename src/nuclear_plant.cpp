#include "energy_source.hpp"


namespace NRG {

class NuclearPlant : public EnergySource 
{
public:
    NuclearPlant(struct EnergySourceParameters const &esp);
    ~NuclearPlant();

    int get_emissionsOutput(Emissions& emissions) override;

    int get_productionCost(float &cost, float powerRequest) override;

private:
    NuclearPlant(const NuclearPlant&) = delete;
};


NuclearPlant::NuclearPlant(struct EnergySourceParameters const &esp) 
: 
    EnergySource::EnergySource(esp)
{}


NuclearPlant::~NuclearPlant()
{}


int
NuclearPlant::get_emissionsOutput(Emissions& emissions)
{
    emissions.carbonDioxide = _currPowerOutput * 0;
    emissions.methane       = _currPowerOutput * 0;
    emissions.nitrousOxide  = _currPowerOutput * 0;

    return SUCCESS;
}


int
NuclearPlant::get_productionCost(float &cost, float powerRequest)
{
    cost = powerRequest * _runCost / 3600;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_NuclearPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::NuclearPlant(esp);
}
