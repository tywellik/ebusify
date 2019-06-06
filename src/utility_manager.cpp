#include "utility_manager.hpp"

#define LOGERR(fmt, args...)   do{ fprintf(stderr, fmt "\n", ##args); }while(0)
#ifdef VERBOSE
    #define LOGDBG(fmt, args...)   do{ fprintf(stdout, fmt "\n", ##args); }while(0)
#else
    #define LOGDBG(fmt, args...)   do{}while(0)
#endif

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


UtilityManager::UtilityManager()
{

}


UtilityManager::~UtilityManager()
{

}


int
UtilityManager::init(bp::dict const& facilities)
{
    int ret = convert_toSources(facilities);
    _pvProduction.push_back(0.0);
    _windProduction.push_back(920.0);

    return ret;
}


int
UtilityManager::init(bp::dict const& facilities, bpn::ndarray const& pvProduction_MW, bpn::ndarray const& windProduction_MW)
{
    int ret = convert_toSources(facilities);
    // Sanity check on input arrays.
    bpn::ndarray const* inputArrays[] = { &pvProduction_MW, &windProduction_MW };
    for (auto arr : inputArrays) {
        if (arr->get_dtype() != bpn::dtype::get_builtin<float>()) {
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
        assert(arr->strides(0) == sizeof(float));
    }

    unsigned int dataLen = pvProduction_MW.shape(0);
    float* pvProd = reinterpret_cast<float*>(pvProduction_MW.get_data());
    _pvProduction.assign(pvProd, pvProd + dataLen);
    float* windProd = reinterpret_cast<float*>(windProduction_MW.get_data());
    _windProduction.assign(windProd, windProd + dataLen);

    return ret;
}


int
UtilityManager::startup(float power)
{
    std::string name;
    for (auto& us : _uncontrolledSources)
    {
        name = us->get_name();
        if (name.compare("Wind (aggregated) - base case") == 0){
            us->set_powerPoint(_windProduction.front());
        }
        if (name.compare("Solar (aggregated) - base case") == 0){
            us->set_powerPoint(_pvProduction.front());
        }
    }

    // Model
    GRBEnv* env        = 0;
    GRBVar* plantOn    = 0;
    GRBVar* production = 0;

    env = new GRBEnv();
    GRBModel model = GRBModel(*env);
    model.set(GRB_StringAttr_ModelName, "startup");

    // Plant on decision variables
    plantOn = model.addVars(_sources.size(), GRB_BINARY);

    int p;
    for (p = 0; p < _sources.size(); ++p)
    {
        plantOn[p].set(GRB_DoubleAttr_Obj, _sources[p]->get_powerCost());
        plantOn[p].set(GRB_StringAttr_VarName, "PlantOn" + _sources[p]->get_name());
    }

    production = model.addVars(_sources.size());
    for (p = 0; p < _sources.size(); ++p)
    {
        production[p].set(GRB_DoubleAttr_Obj, _sources[p]->get_powerCost());
        production[p].set(GRB_StringAttr_VarName, "Prod" + _sources[p]->get_name());
    }

    model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);

    for (p = 0; p < _sources.size(); ++p)
    {
        GRBLinExpr ptot = 0;

        std::shared_ptr<EnergySource> eSrc = _sources[p];
        model.addConstr(ptot <= eSrc->get_maxOutputPower(), "CapacityMax" + eSrc->get_name());
        model.addConstr(ptot >= eSrc->get_maxOutputPower() * eSrc->get_minOutputPower() * plantOn[p], "CapacityMin" + eSrc->get_name());
        //model.addConstr(plantOn[p] ^ production[p] < 0.001, );
    }

    // First, open all plants
    for (p = 0; p < _sources.size(); ++p)
    {
      plantOn[p].set(GRB_DoubleAttr_Start, 1.0);
      production[p].set(GRB_DoubleAttr_Start, _sources[p]->get_maxOutputPower() * _sources[p]->get_minOutputPower());
    }

    GRBLinExpr output = 0;
    for (p = 0; p < _sources.size(); ++p)
    {
        output += production[p];
    }
    model.addConstr(output == power);

    // Use barrier to solve root relaxation
    model.set(GRB_IntParam_Method, GRB_METHOD_BARRIER);

    // Solve
    model.optimize();

    using std::cout;
    using std::endl;
    // Print solution
    cout << "\nTOTAL COSTS: " << model.get(GRB_DoubleAttr_ObjVal) << endl;
    cout << "SOLUTION:" << endl;
    for (p = 0; p < _sources.size(); ++p)
    {
        //cout << p << endl;
        cout << _sources[p]->get_name() << endl;
        cout << "Plant On:   " << plantOn[p].get(GRB_DoubleAttr_X) << endl;
        cout << "Production: " << production[p].get(GRB_DoubleAttr_X) << endl << endl;
        
        continue;
        if (plantOn[p].get(GRB_DoubleAttr_X) > 0.99)
        {
            cout << "Plant " << p << " on. Producing: " << production[p].get(GRB_DoubleAttr_X) << endl;
        }
        else
        {
            cout << "Plant " << p << " off!" << endl;
        }
    }

    delete[] plantOn;
    delete[] production;
    //delete env;

    return SUCCESS;
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
            .name        = key,
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
            eSrc.reset(create_SolarPlant(this, esp));
            break;
        case eWIND:
            LOGDBG("Creating Wind Plant: %s", key.c_str());
            eSrc.reset(create_WindPlant(this, esp));
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "Facility Type is not supported");
            bp::throw_error_already_set();
        }

        _sources.push_back(eSrc); // Add new plant to list of plants
    }

    return SUCCESS;
}



int
UtilityManager::register_uncontrolledSource(EnergySource* es)
{
    std::shared_ptr<EnergySource> eSrc;
    eSrc.reset(es);
    _uncontrolledSources.push_back(eSrc);

    return SUCCESS;
}


} /** namespace */

// Expose overloaded init function
int (NRG::UtilityManager::*init)(bp::dict const&) = &NRG::UtilityManager::init;
int (NRG::UtilityManager::*init_uc)(bp::dict const&, bpn::ndarray const&, bpn::ndarray const&) = &NRG::UtilityManager::init;

BOOST_PYTHON_MODULE(UtilityManager)
{
    bpn::initialize();

    bp::class_<NRG::UtilityManager>("UtilityManager")
        .def("init",                init)
        .def("init",                init_uc)
        .def("startup",             &NRG::UtilityManager::startup)
        .def("power_request",       &NRG::UtilityManager::power_request)
        .def("get_totalEmissions",  &NRG::UtilityManager::get_totalEmissions)
    ;
}
