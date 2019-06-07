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
    using std::cout;
    using std::endl;

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
    GRBVar* production = 0;
    GRBVar* plantOn    = 0;
    GRBVar demand;
    
    try
    {
        // Model
        env = new GRBEnv();
        //GRBModel model = GRBModel(*env);
        GRBModel model = GRBModel(*env);
        model.set(GRB_StringAttr_ModelName, "startup");

        std::string plantNames_cost[_sources.size()];
        std::string plantNames_on[_sources.size()];
        double      runCosts[_sources.size()];
        double      minCapacity[_sources.size()];
        double      maxCapacity[_sources.size()];
        for (int src = 0; src < _sources.size(); ++src)
        {
            plantNames_cost[src]  = _sources[src]->get_name() + "_cost";
            plantNames_on[src]    = _sources[src]->get_name() + "_on";
            runCosts[src]         = _sources[src]->get_powerCost();
            minCapacity[src]      = _sources[src]->get_minOutputPower() * _sources[src]->get_maxOutputPower();
            maxCapacity[src]      = _sources[src]->get_maxOutputPower();
        }

        double zeros[_sources.size()];
        double ones[_sources.size()];
        char types_bin[_sources.size()];
        char types_cont[_sources.size()];
        std::fill(zeros, zeros + _sources.size(), 0);
        std::fill(ones, ones + _sources.size(), 1);
        std::fill(types_bin, types_bin + _sources.size(), GRB_BINARY);
        std::fill(types_cont, types_cont + _sources.size(), GRB_CONTINUOUS);

        demand     = model.addVar(power, power, 0, GRB_CONTINUOUS, "Demand");
        production = model.addVars(minCapacity, maxCapacity, runCosts, types_cont, plantNames_cost, _sources.size());
        plantOn    = model.addVars(zeros, ones, zeros, types_bin, plantNames_on, _sources.size());

        model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);

        GRBQuadExpr powTotal = 0;
        for (int k=0; k < _sources.size(); ++k)
        {
            powTotal += (plantOn[k] * production[k]);
        }

        model.addQConstr(powTotal == demand, "DemandConstraint");

        model.optimize();
        model.write("out.mst");

        for (int src = 0; src < _sources.size(); ++src)
        {
            cout << _sources[src]->get_name() << endl;
            cout << plantOn[src].get(GRB_DoubleAttr_ObjVal) << endl;
            cout << production[src].get(GRB_DoubleAttr_ObjVal) << endl << endl;
        }
    }
    catch (GRBException e)
    {
        cout << "Error code = " << e.getErrorCode() << endl;
        cout << e.getMessage() << endl;
    }
    catch (...)
    {
        cout << "Exception during optimization" << endl;
    }

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
        std::string *type;
        std::string *name;
        float *maxCap;
        float *minCap;
        float *runCost;
        float *rampRate;
        std::shared_ptr<EnergySource> eSrc;

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

        bp::dict source = (bp::dict)facilities[key];

        name     = new std::string(key);
        type     = new std::string(bp::extract<std::string>(bp::str(source["type"]))());
        maxCap   = new float(bp::extract<float>(source["MaxCap"])());
        minCap   = new float(bp::extract<float>(source["MinCap"])());
        runCost  = new float(bp::extract<float>(source["Running Cost"])());
        rampRate = new float(bp::extract<float>(source["Ramp Rate"])());

        struct EnergySourceParameters esp = {
            //.name        = key,
            .name        = *name,
            .maxCapacity = *maxCap,
            .minCapacity = *minCap,
            .runCost     = *runCost,
            .rampRate    = *rampRate
        };

        enum EnergyFuels fuelType = fuelStringToEnum[*type];

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
