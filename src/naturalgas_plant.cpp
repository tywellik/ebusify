#include "energy_source.hpp"


namespace NRG {

class NaturalGasPlant : public EnergySource 
{
public:
    NaturalGasPlant(struct EnergySourceParameters const &esp);
    ~NaturalGasPlant();

    EnergySource::Emissions get_emissionsOutput() override;

    double get_productionCost(double powerRequest) override;

private:
    NaturalGasPlant(const NaturalGasPlant&) = delete;
};


NaturalGasPlant::NaturalGasPlant(struct EnergySourceParameters const &esp) 
: 
    EnergySource::EnergySource(esp)
{}


NaturalGasPlant::~NaturalGasPlant()
{}


EnergySource::Emissions
NaturalGasPlant::get_emissionsOutput()
{
    EnergySource::Emissions emissions = {
        .carbonDioxide = _currPowerOutput * 505,
        .methane       = _currPowerOutput * 0,
        .nitrousOxide  = _currPowerOutput * 0
    };

    return emissions;
}


double
NaturalGasPlant::get_productionCost(double powerRequest)
{
    return powerRequest * _runCost / 3600;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_NaturalGasPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::NaturalGasPlant(esp);
}
