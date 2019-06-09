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
    double       maxCapacity;  /** MW                 */
    double       minCapacity;  /** % Max output power */
    double       runCost;      /** $/MWh              */
    double       rampRate;     /** %maxCapacity/min   */
};

class EnergySource
{
public:
    EnergySource(struct EnergySourceParameters const &esp);
    ~EnergySource();

    std::string get_name() const;
    double get_powerCost() const;
    double get_maxOutputPower() const;
    double get_minOutputPower() const;
    double get_maxPositiveRamp() const;
    double get_maxNegativeRamp() const;
    double get_currPower() const;

    void set_powerPoint(double power);

    struct Emissions {
        double carbonDioxide;
        double methane;
        double nitrousOxide;
        
        Emissions operator+(Emissions rhs) {
            return {
                rhs.carbonDioxide + carbonDioxide,
                rhs.methane + methane,
                rhs.nitrousOxide + nitrousOxide,
            };
        }
        Emissions operator+=(Emissions rhs) {
            return {
                rhs.carbonDioxide + carbonDioxide,
                rhs.methane + methane,
                rhs.nitrousOxide + nitrousOxide,
            };
        }
    };

    virtual EnergySource::Emissions get_emissionsOutput() = 0;

    virtual double get_productionCost(double powerRequest) = 0;

protected:
    std::string _name;            /** Plant name */
    double      _maxOutputPower;  /** MW                 */
    double      _minOutputPower;  /** % Max output power */
    double      _maxPositiveRamp; /** %maxCapacity/min   */
    double      _maxNegativeRamp; /** %maxCapacity/min   */
    double      _runCost;         /** $/MWh              */

    double      _currPowerOutput; /** MW                 */
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
