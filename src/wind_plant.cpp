#include "energy_source.hpp"
#include "utility_manager.hpp"


namespace NRG {

class WindPlant : public EnergySource 
{
public:
    WindPlant(UtilityManager* um, struct EnergySourceParameters const &esp);
    ~WindPlant();

    int get_emissionsOutput(Emissions& emissions) override;

    int get_productionCost(double &cost, double powerRequest) override;

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


int
WindPlant::get_emissionsOutput(Emissions& emissions)
{
    emissions.carbonDioxide = 0;
    emissions.methane       = 0;
    emissions.nitrousOxide  = 0;

    return SUCCESS;
}


int
WindPlant::get_productionCost(double &cost, double powerRequest)
{
    cost = powerRequest * _runCost / 3600;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_WindPlant(UtilityManager* um, NRG::EnergySourceParameters const &esp)
{
    return new NRG::WindPlant(um, esp);
}
