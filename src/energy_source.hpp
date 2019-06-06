#ifndef ENERGYSOURCE_H
#define ENERGYSOURCE_H
#include <string>

namespace NRG {

#define     SUCCESS 0
#define     FAILURE 1


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
    std::string name;         /** Plant name         */
    float       maxCapacity;  /** MW                 */
    float       minCapacity;  /** % Max output power */
    float       runCost;      /** $/MWh              */
    float       rampRate;     /** %maxCapacity/min   */
};

class EnergySource
{
public:
    EnergySource(struct EnergySourceParameters const &esp);
    ~EnergySource();

    std::string get_name() const;
    float get_powerCost() const;
    float get_maxOutputPower() const;
    float get_minOutputPower() const;
    float get_maxPositiveRamp() const;
    float get_maxNegativeRamp() const;
    float get_currPower() const;

    void set_powerPoint(float power);

    struct Emissions {
        float carbonDioxide;
        float methane;
        float nitrousOxide;
        // Add more as necessary
    };

    virtual int get_emissionsOutput(Emissions& emissions) = 0;

    virtual int get_productionCost(float &cost, float powerRequest) = 0;

protected:
    std::string _name;            /** Plant name */
    float       _maxOutputPower;  /** MW                 */
    float       _minOutputPower;  /** % Max output power */
    float       _maxPositiveRamp; /** %maxCapacity/min   */
    float       _maxNegativeRamp; /** %maxCapacity/min   */
    float       _runCost;         /** $/MWh              */

    float       _currPowerOutput; /** MW                 */
};


class UtilityManager; // Forward declaration
EnergySource* create_BiomassPlant(struct EnergySourceParameters const &esp);
EnergySource* create_CoalPlant(struct EnergySourceParameters const &esp);
EnergySource* create_HydroPlant(struct EnergySourceParameters const &esp);
EnergySource* create_NaturalGasPlant(struct EnergySourceParameters const &esp);
EnergySource* create_NuclearPlant(struct EnergySourceParameters const &esp);
EnergySource* create_SolarPlant(UtilityManager* um, struct EnergySourceParameters const &esp);
EnergySource* create_WindPlant(UtilityManager* um, struct EnergySourceParameters const &esp);

} // namespace NRG


#endif /** ENERGYSOURCE_H */
