#include "utility_manager.hpp"

#define LOGERR(fmt, args...)   do{ fprintf(stderr, fmt "\n", ##args); }while(0)
#define LOGDBG(fmt, args...)   do{ fprintf(stdout, fmt "\n", ##args); }while(0)


namespace NRG {

std::map<std::string, enum EnergyFuels> fuelStringToEnum = {
    {   "Biomass",      eBIOMASS        },
    {   "CoalPlant",    eCOAL           },
    {   "Hydro",        eHYDRO          },
    {   "NatGasPlant",  eNATURALGAS     },
    {   "NuclearPlant", eNUCLEAR        },
    {   "Solar",        eSOLAR          },
    {   "Wind",         eWIND         }
};


template< typename T >
inline
std::vector< T > to_std_vector( const bp::object& iterable )
{
    return std::vector< T >( bp::stl_input_iterator< T >( iterable ),
                             bp::stl_input_iterator< T >( ) );
}


UtilityManager::UtilityManager(bp::dict const& facilities)
:
    _facilities(facilities)
{

}


UtilityManager::~UtilityManager()
{

}


int
UtilityManager::init()
{
    int ret = convert_toSources(_facilities);
}


int
UtilityManager::power_request(float power)
{
    /*float minPower = 0.0;
    LOGDBG("Total Active Plants: %zu", _sources.size());

    for (auto& plant : _sources)
    {
        minPower += plant->get_minOutputPower();
    }

    LOGDBG("Total Min Power: %.2f MW", minPower);
    */

    float currPower = 0.0;
    currPower = get_currPower();



    return SUCCESS;
}


bp::tuple
UtilityManager::get_totalEmissions()
{
    EnergySource::Emissions sourceEmissions = {
        .carbonDioxide = 0,
        .methane       = 0,
        .nitrousOxide  = 0
    };

    EnergySource::Emissions totalEmissions = {
        .carbonDioxide = 0,
        .methane       = 0,
        .nitrousOxide  = 0
    };

    for (auto& source : _sources)
    {
        source->get_emissionsOutput(sourceEmissions);
        addEmissions(sourceEmissions, totalEmissions);
    }

    return bp::make_tuple(totalEmissions.carbonDioxide, totalEmissions.methane, totalEmissions.nitrousOxide);
}


void
UtilityManager::addEmissions(EnergySource::Emissions &sourceEmissions, EnergySource::Emissions &totalEmissions)
{
    totalEmissions.carbonDioxide += sourceEmissions.carbonDioxide;
    totalEmissions.methane       += sourceEmissions.methane;
    totalEmissions.nitrousOxide  += sourceEmissions.nitrousOxide;
}


float
UtilityManager::get_currPower()
{
    float currPower = 0.0;

    for (auto& source : _sources)
    {
        currPower += source->get_currPower();
    }

    return currPower;
}


int
UtilityManager::convert_toSources(bp::dict const& facilities)
{
    std::vector<std::string> keys = to_std_vector<std::string>(facilities.keys());

    for (auto& key : keys)
    {
        std::string type;
        float maxCap;
        float minCap;
        float runCost;
        float rampRate;

        // @TODO: Error checking here
        /*
        if (arr->get_dtype() != bpn::dtype::get_builtin<double>()) {
            PyErr_SetString(PyExc_TypeError, "Incorrect array data type");
            bp::throw_error_already_set();
        }
        if (arr->get_nd() != 1) {
            PyErr_SetString(PyExc_TypeError, "Incorrect number of dimensions");
            bp::throw_error_already_set();
        }
        if ((arr->get_flags() & bpn::ndarray::C_CONTIGUOUS) == 0) {
            PyErr_SetString(PyExc_TypeError, "Array must be row-major contiguous");
            bp::throw_error_already_set();
        }
        */

        bp::dict source  = (bp::dict)facilities[key];

        type     = bp::extract<std::string>(bp::str(source["type"]))();
        maxCap   = bp::extract<float>(source["MaxCap"])();
        minCap   = bp::extract<float>(source["MinCap"])();
        runCost  = bp::extract<float>(source["Running Cost"])();
        rampRate = bp::extract<float>(source["Ramp Rate"])();

        struct EnergySourceParameters esp = {
            .maxCapacity = maxCap,
            .minCapacity = minCap,
            .runCost     = runCost,
            .rampRate    = rampRate
        };

        enum EnergyFuels fuelType = fuelStringToEnum[type];
        std::shared_ptr<EnergySource> eSrc;

        switch( fuelType ){
        case eBIOMASS:
            LOGDBG("Creating Biomass Plant: %s", key.c_str());
            eSrc.reset(create_BiomassPlant(esp));
            break;
        case eCOAL:
            LOGDBG("Creating Coal Plant: %s", key.c_str());
            eSrc.reset(create_CoalPlant(esp));
            break;
        case eHYDRO:
            LOGDBG("Creating Hydro Plant: %s", key.c_str());
            eSrc.reset(create_HydroPlant(esp));
            break;
        case eNATURALGAS:
            LOGDBG("Creating Natural Gas Plant: %s", key.c_str());
            eSrc.reset(create_NaturalGasPlant(esp));
            break;
        case eNUCLEAR:
            LOGDBG("Creating Nuclear Plant: %s", key.c_str());
            eSrc.reset(create_NuclearPlant(esp));
            break;
        case eSOLAR:
            LOGDBG("Creating Solar Plant: %s", key.c_str());
            eSrc.reset(create_SolarPlant(esp));
            break;
        case eWIND:
            LOGDBG("Creating Wind Plant: %s", key.c_str());
            eSrc.reset(create_WindPlant(esp));
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "Facility Type is not supported");
            bp::throw_error_already_set();
        }

        _sources.push_back(eSrc); // Add new plant to list of plants
    }

    return SUCCESS;
}


} /** namespace */


BOOST_PYTHON_MODULE(UtilityManager)
{
    bpn::initialize();

    bp::class_<NRG::UtilityManager>("UtilityManager", bp::init<bp::dict>())
        .def("init",                &NRG::UtilityManager::init)
        .def("power_request",       &NRG::UtilityManager::power_request)
        .def("get_totalEmissions",  &NRG::UtilityManager::get_totalEmissions)
    ;
}
