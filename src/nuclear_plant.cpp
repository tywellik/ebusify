#include "energy_source.hpp"


namespace NRG {

class NuclearPlant : public EnergySource 
{
public:
    NuclearPlant(struct EnergySourceParameters const &esp);
    ~NuclearPlant();

    EnergySource::Emissions get_emissionsOutput() override;

    double get_productionCost(double powerRequest) override;

private:
    NuclearPlant(const NuclearPlant&) = delete;
};


NuclearPlant::NuclearPlant(struct EnergySourceParameters const &esp) 
: 
    EnergySource::EnergySource(esp)
{}


NuclearPlant::~NuclearPlant()
{}


EnergySource::Emissions
NuclearPlant::get_emissionsOutput()
{
    EnergySource::Emissions emissions = {
        .carbonDioxide = _currPowerOutput * 0,
        .methane       = _currPowerOutput * 0,
        .nitrousOxide  = _currPowerOutput * 0
    };

    return emissions;
}


double
NuclearPlant::get_productionCost(double powerRequest)
{
    return powerRequest * _runCost / 3600;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_NuclearPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::NuclearPlant(esp);
}
