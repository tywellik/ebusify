#include "energy_source.hpp"


namespace NRG {

class BiomassPlant : public EnergySource 
{
public:
    BiomassPlant(struct EnergySourceParameters const &esp);
    ~BiomassPlant();

    int get_emissionsOutput(Emissions& emissions) override;

    int get_productionCost(double &cost, double powerRequest) override;

private:
    BiomassPlant(const BiomassPlant&) = delete;
};


BiomassPlant::BiomassPlant(struct EnergySourceParameters const &esp)
: 
    EnergySource::EnergySource(esp)
{}


BiomassPlant::~BiomassPlant()
{}


int
BiomassPlant::get_emissionsOutput(Emissions& emissions)
{
    emissions.carbonDioxide = _currPowerOutput * 0;
    emissions.methane       = _currPowerOutput * 0;
    emissions.nitrousOxide  = _currPowerOutput * 0;

    return SUCCESS;
}


int
BiomassPlant::get_productionCost(double &cost, double powerRequest)
{
    cost = powerRequest * _runCost / 3600;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_BiomassPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::BiomassPlant(esp);
}
