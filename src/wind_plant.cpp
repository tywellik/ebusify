#include "energy_source.hpp"
#include "utility_manager.hpp"


namespace NRG {

class WindPlant : public EnergySource 
{
public:
    WindPlant(UtilityManager* um, struct EnergySourceParameters const &esp);
    ~WindPlant();

    EnergySource::Emissions get_emissionsOutput() override;

    double get_productionCost(double powerRequest) override;

private:
    UtilityManager* _um;

    WindPlant(const WindPlant&) = delete;
};


WindPlant::WindPlant(UtilityManager* um, struct EnergySourceParameters const &esp) 
: 
    EnergySource::EnergySource(esp),
    _um(um)
{
    _um->register_uncontrolledSource(get_name());
}


WindPlant::~WindPlant()
{}


EnergySource::Emissions
WindPlant::get_emissionsOutput()
{
    EnergySource::Emissions emissions = {
        .carbonDioxide = _currPowerOutput * 0,
        .methane       = _currPowerOutput * 0,
        .nitrousOxide  = _currPowerOutput * 0
    };

    return emissions;
}


double
WindPlant::get_productionCost(double powerRequest)
{
    return powerRequest * _runCost / 3600;
}


} // namespace NRG


NRG::EnergySource*
NRG::create_WindPlant(UtilityManager* um, NRG::EnergySourceParameters const &esp)
{
    return new NRG::WindPlant(um, esp);
}
