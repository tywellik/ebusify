#include "energy_source.hpp"


namespace NRG {

class BiomassPlant : public EnergySource 
{
public:
    BiomassPlant(struct EnergySourceParameters const &esp);
    ~BiomassPlant();

    EnergySource::Emissions get_emissionsOutput() override;

    double get_productionCost(double powerRequest) override;

private:
    BiomassPlant(const BiomassPlant&) = delete;
};


BiomassPlant::BiomassPlant(struct EnergySourceParameters const &esp)
: 
    EnergySource::EnergySource(esp)
{}


BiomassPlant::~BiomassPlant()
{}


EnergySource::Emissions
BiomassPlant::get_emissionsOutput()
{
    EnergySource::Emissions emissions = {
        .carbonDioxide = _currPowerOutput * 0,
        .methane       = _currPowerOutput * 0,
        .nitrousOxide  = _currPowerOutput * 0
    };

    return emissions;
}


double
BiomassPlant::get_productionCost(double powerRequest)
{
    return powerRequest * _runCost / 3600;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_BiomassPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::BiomassPlant(esp);
}
