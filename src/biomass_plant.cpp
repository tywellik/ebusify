#include "energy_source.hpp"


namespace NRG {

class BiomassPlant : public EnergySource 
{
public:
    BiomassPlant(struct EnergySourceParameters const &esp);
    ~BiomassPlant();

    int get_emissionsOutput(Emissions& emissions, float powerRequest) override;

    int get_productionCost(float &cost, float powerRequest) override;

private:
    BiomassPlant(const BiomassPlant&) = delete;

    float _maxOutputPower;  /** MW               */
    float _minOutputPower;  /** MW               */
    float _maxPositiveRamp; /** %maxCapacity/min */
    float _maxNegativeRamp; /** %maxCapacity/min */
    float _runCost;         /** $/MWh            */

    float _currPowerOutput; /** MW               */
};


BiomassPlant::BiomassPlant(struct EnergySourceParameters const &esp)
: 
    EnergySource::EnergySource(esp)
{}


BiomassPlant::~BiomassPlant()
{}


int
BiomassPlant::get_emissionsOutput(Emissions& emissions, float powerRequest)
{
    emissions.carbonDioxide = _currPowerOutput * .099;
    emissions.methane       = _currPowerOutput * .0001;
    emissions.nitrousOxide  = _currPowerOutput * .0009;

    return SUCCESS;
}


int
BiomassPlant::get_productionCost(float &cost, float powerRequest)
{
    cost = powerRequest * _runCost;
}

} // namespace NRG


NRG::EnergySource*
NRG::create_BiomassPlant(NRG::EnergySourceParameters const &esp)
{
    return new NRG::BiomassPlant(esp);
}
