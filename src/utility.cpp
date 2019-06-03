#include "utility.hpp"

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


Utility::Utility(bp::dict const& facilities)
:
    _facilities(facilities)
{

}


Utility::~Utility()
{

}


int
Utility::init()
{
    int ret = convert_toSources(_facilities);
}


int
Utility::power_request(float power)
{
    float minPower = 0.0;
    std::cout << "Total Active Plants: " << _sources.size() << std::endl;

    for (auto& plant : _sources)
    {
        minPower += plant->get_minOutputPower();
    }

    std::cout << "Total Min Power: " << minPower << std::endl;

    return 0;
}


int
Utility::convert_toSources(bp::dict const& facilities)
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
            std::cout << "Creating Biomass Plant: " << key << std::endl;
            eSrc.reset(create_BiomassPlant(esp));
            break;
        case eCOAL:
            std::cout << "Creating Coal Plant: " << key << std::endl;
            eSrc.reset(create_CoalPlant(esp));
            break;
        case eHYDRO:
            std::cout << "Creating Hydro Plant: " << key << std::endl;
            eSrc.reset(create_HydroPlant(esp));
            break;
        case eNATURALGAS:
            std::cout << "Creating Natural Gas Plant: " << key << std::endl;
            eSrc.reset(create_NaturalGasPlant(esp));
            break;
        case eNUCLEAR:
            std::cout << "Creating Nuclear Plant: " << key << std::endl;
            eSrc.reset(create_NuclearPlant(esp));
            break;
        case eSOLAR:
            std::cout << "Creating Solar Plant: " << key << std::endl;
            eSrc.reset(create_SolarPlant(esp));
            break;
        case eWIND:
            std::cout << "Creating Wind Plant: " << key << std::endl;
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


BOOST_PYTHON_MODULE(Utility)
{
    bpn::initialize();

    bp::class_<NRG::Utility>("Utility", bp::init<bp::dict>())
        .def("init",          &NRG::Utility::init)
        .def("power_request", &NRG::Utility::power_request)
    ;
}
