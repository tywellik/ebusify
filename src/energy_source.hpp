#ifndef ENERGYSOURCE_H
#define ENERGYSOURCE_H
#include <string>

namespace NRG {

#define     SUCCESS 0


enum EnergyFuels {
    eNOTSUPPORTED = 0,
    eBIOMASS,
    eCOAL,
    eHYDRO,
    eNATURALGAS,
    eNUCLEAR,
    eSOLAR,
    eWIND,
    eEND
};

struct EnergySourceParameters {
    float maxCapacity;  /** MW               */
    float minCapacity;  /** MW               */
    float runCost;      /** $/MWh            */
    float rampRate;     /** %maxCapacity/min */
};

class EnergySource
{
public:
    EnergySource(struct EnergySourceParameters const &esp);
    ~EnergySource();

    float get_maxOutputPower() const;
    float get_minOutputPower() const;
    float get_maxPositiveRamp() const;
    float get_maxNegativeRamp() const;

    void set_powerPoint(float power);

    struct Emissions {
        float carbonDioxide;
        float methane;
        float nitrousOxide;
        // Add more as necessary
    };

    virtual int get_emissionsOutput(Emissions& emissions, float powerRequest) = 0;

    virtual int get_productionCost(float &cost, float powerRequest) = 0;

protected:
    float _maxOutputPower;  /** MW               */
    float _minOutputPower;  /** MW               */
    float _maxPositiveRamp; /** %maxCapacity/min */
    float _maxNegativeRamp; /** %maxCapacity/min */
    float _runCost;         /** $/MWh            */

    float _currPowerOutput; /** MW               */
};


EnergySource* create_BiomassPlant(struct EnergySourceParameters const &esp);
EnergySource* create_CoalPlant(struct EnergySourceParameters const &esp);
EnergySource* create_HydroPlant(struct EnergySourceParameters const &esp);
EnergySource* create_NaturalGasPlant(struct EnergySourceParameters const &esp);
EnergySource* create_NuclearPlant(struct EnergySourceParameters const &esp);
EnergySource* create_SolarPlant(struct EnergySourceParameters const &esp);
EnergySource* create_WindPlant(struct EnergySourceParameters const &esp);

} // namespace NRG


#endif /** ENERGYSOURCE_H */
