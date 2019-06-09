#include "utility_manager.hpp"
#include <sstream>
#include <iomanip>

//#define VERBOSE
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
UtilityManager::init(bpn::ndarray const& sourceName, bpn::ndarray const& sourceType, 
                     bpn::ndarray const& maxCap, bpn::ndarray const& minCap,
                     bpn::ndarray const& runCost, bpn::ndarray const& rampRate)
{
    std::cout << "POOP!!!" << std::endl;
    int ret = convert_toSources(sourceName, sourceType, maxCap, minCap, runCost, rampRate);
    _pvProduction.push_back(0.0);
    _windProduction.push_back(920.0);
    return ret;
}


int
UtilityManager::init(bpn::ndarray const& sourceName, bpn::ndarray const& sourceType, 
                     bpn::ndarray const& maxCap, bpn::ndarray const& minCap,
                     bpn::ndarray const& runCost, bpn::ndarray const& rampRate, 
                     bpn::ndarray const& pvProduction_MW, bpn::ndarray const& windProduction_MW)
{
    int ret = convert_toSources(sourceName, sourceType, maxCap, minCap, runCost, rampRate);

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
    if (dataLen != windProduction_MW.shape(0)){
        PyErr_SetString(PyExc_TypeError, "PV and Solar data lengths inconsistent");
        bp::throw_error_already_set();
    }

    float* pvProd   = reinterpret_cast<float*>(pvProduction_MW.get_data());
    float* windProd = reinterpret_cast<float*>(windProduction_MW.get_data());

    //float pvProd[dataLen];
    //float windProd[dataLen];
    //&pvProd[0]   = reinterpret_cast<float*>(pvProduction_MW.get_data());
    //&windProd[0] = reinterpret_cast<float*>(windProduction_MW.get_data());
    _pvProduction.assign(pvProd, pvProd + dataLen);
    _windProduction.assign(windProd, windProd + dataLen);
    return ret;
}


int
UtilityManager::startup(float power)
{
    using std::cout;
    using std::endl;

    // Model
    GRBEnv* env        = 0;
    GRBVar* production = 0;
    GRBVar* plantOn    = 0;
    try
    {
        // Model
        env = new GRBEnv();
        GRBModel model = GRBModel(*env);
        env->set(GRB_IntParam_OutputFlag, 0);        
        model.set(GRB_StringAttr_ModelName, "startup");

        int numSources = _sources.size();
        std::map<std::string, int> arrayLoc;
        std::string plantNames[numSources];
        std::string plantNames_on[numSources];
        std::string plantNames_prod[numSources];
        double      runCosts[numSources];
        double      minCapacity[numSources];
        double      maxCapacity[numSources];

        int k=0;
        for (auto& src: _sourceNames)
        {
            plantNames[k]       = src;
            plantNames_on[k]    = src + "_on";
            plantNames_prod[k]  = src + "_cost";
            runCosts[k]         = _sources[src]->get_powerCost();
            minCapacity[k]      = _sources[src]->get_minOutputPower();
            maxCapacity[k]      = _sources[src]->get_maxOutputPower();
            arrayLoc.insert(std::pair<std::string, int>(src, k)); 
            k++;
        }
        
        for (auto& ucSrc : _ucSourceNames)
        {
            if (ucSrc.compare("Wind_(aggregated)_-_base_case") == 0){
                LOGDBG("Setting max wind to:  %f", _windProduction.front());
                maxCapacity[arrayLoc[ucSrc]] = _windProduction.front();
            }
            if (ucSrc.compare("Solar_(aggregated)_-_base_case") == 0){
                LOGDBG("Setting max solar to: %f", _pvProduction.front());
                maxCapacity[arrayLoc[ucSrc]] = _pvProduction.front();
            }
        }
        
        double zeros[numSources];
        double ones[numSources];
        char types_bin[numSources];
        char types_cont[numSources];
        std::fill(zeros, zeros + numSources, 0);
        std::fill(ones, ones + numSources, 1);
        std::fill(types_bin, types_bin + numSources, GRB_BINARY);
        std::fill(types_cont, types_cont + numSources, GRB_CONTINUOUS);

        production = model.addVars(minCapacity, maxCapacity, ones, types_cont, plantNames_prod, numSources);
        plantOn    = model.addVars(zeros, ones, ones, types_bin, plantNames_on, numSources);

        GRBQuadExpr costTotal = 0;
        for (k=0; k < numSources; ++k)
        {
            costTotal += (runCosts[k] * production[k] * plantOn[k]);
        }
        model.setObjective(costTotal, GRB_MINIMIZE);

        GRBQuadExpr proTotal = 0;
        for (k=0; k < numSources; ++k)
        {
            proTotal += production[k] * plantOn[k];
            model.addConstr((production[k] - maxCapacity[k]) <= 0,              plantNames[k] + "_NoOverProd");
            model.addConstr((production[k] - minCapacity[k] * plantOn[k]) >= 0, plantNames[k] + "_OffOrGtMinCap");
            model.addConstr( production[k] >= 0,                                plantNames[k] + "_ProdGtEZ");
        }
        model.addQConstr(proTotal == power, "DemandConstraint");
        
        // First, close all plants
        for (k = 0; k < numSources; ++k)
        {
            plantOn[k].set(GRB_DoubleAttr_Start, 0.0);
        }

        model.update();
        model.optimize();
        //model.write("test.mps");
        //model.write("test.prm");
        //model.write("test.mst");

        double totalPower = 0.0;
        LOGDBG("TOTAL COSTS: %f", model.get(GRB_DoubleAttr_ObjVal));
        LOGDBG("SOLUTION:");
        for (k = 0; k < numSources; ++k)
        {
            std::stringstream ss;
            if (plantOn[k].get(GRB_DoubleAttr_X) > 0.99)
            {
                ss << std::left << std::setw(40)
                   << plantNames[k].c_str() << " open and producing: "
                   << std::fixed << std::setprecision(2)
                   << production[k].get(GRB_DoubleAttr_X) << "MW ("
                   << 100*production[k].get(GRB_DoubleAttr_X)/maxCapacity[k]
                   << "% Cap Factor)";
                

                totalPower += production[k].get(GRB_DoubleAttr_X);
            }
            else
            {
                ss << std::left << std::setw(40) << plantNames[k].c_str() 
                   << " closed";
            }
            LOGDBG("%s", ss.str().c_str());
        }

        LOGDBG("Total Power Produced: %.2f", totalPower);
    }
    catch (GRBException e)
    {
        LOGERR("Error code = %i: %s", e.getErrorCode(), e.getMessage().c_str());
    }
    catch (...)
    {
        LOGERR("Exception during optimization");
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

    for (auto& src: _sourceNames)
    {
        _sources[src]->get_emissionsOutput(sourceEmissions);
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

    for (auto& src: _sourceNames)
    {
        currPower += _sources[src]->get_currPower();
    }

    return currPower;
}


int
UtilityManager::convert_toSources(bpn::ndarray const& sourceName, bpn::ndarray const& sourceType, 
                                  bpn::ndarray const& maxCapacity, bpn::ndarray const& minCapacity,
                                  bpn::ndarray const& runningCost, bpn::ndarray const& rampingRate)
{
    using std::cout;
    using std::endl;

    unsigned int dataLen = sourceName.shape(0);
    double* maxCap       = reinterpret_cast<double*>(maxCapacity.get_data());
    double* minCap       = reinterpret_cast<double*>(minCapacity.get_data());
    double* runCost      = reinterpret_cast<double*>(runningCost.get_data());
    double* rampRate     = reinterpret_cast<double*>(rampingRate.get_data());    


    for (int src = 0; src < dataLen; ++src)
    {
        std::shared_ptr<EnergySource> eSrc;
        std::string name = std::string(bp::extract<char const *>(sourceName[src]));
        std::string type = std::string(bp::extract<char const *>(sourceType[src]));
        
        // Remove spaces in names
        std::transform(name.begin(), name.end(), name.begin(), [](char ch) {
            return ch == ' ' ? '_' : ch;
        });
        std::transform(type.begin(), type.end(), type.begin(), [](char ch) {
            return ch == ' ' ? '_' : ch;
        });

        struct EnergySourceParameters esp = {
            .name        = name,
            .maxCapacity = (float)maxCap[src],
            .minCapacity = (float)minCap[src],
            .runCost     = (float)runCost[src],
            .rampRate    = (float)rampRate[src]
        };
        enum EnergyFuels fuelType = fuelStringToEnum[type];

        switch( fuelType ){
        case eBIOMASS:
            LOGDBG("Creating Biomass Plant: %s", name.c_str());
            eSrc.reset(create_BiomassPlant(esp));
            break;
        case eCOAL:
            LOGDBG("Creating Coal Plant: %s", name.c_str());
            eSrc.reset(create_CoalPlant(esp));
            break;
        case eHYDRO:
            LOGDBG("Creating Hydro Plant: %s", name.c_str());
            eSrc.reset(create_HydroPlant(esp));
            break;
        case eNATURALGAS:
            LOGDBG("Creating Natural Gas Plant: %s", name.c_str());
            eSrc.reset(create_NaturalGasPlant(esp));
            break;
        case eNUCLEAR:
            LOGDBG("Creating Nuclear Plant: %s", name.c_str());
            eSrc.reset(create_NuclearPlant(esp));
            break;
        case eSOLAR:
            LOGDBG("Creating Solar Plant: %s", name.c_str());
            eSrc.reset(create_SolarPlant(this, esp));
            break;
        case eWIND:
            LOGDBG("Creating Wind Plant: %s", name.c_str());
            eSrc.reset(create_WindPlant(this, esp));
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "Facility Type is not supported");
            bp::throw_error_already_set();
        }

        _sourceNames.push_back(name);
        _sources.insert(std::pair<std::string, std::shared_ptr<EnergySource>>(name, eSrc)); 
    }
    return SUCCESS;
}



int
UtilityManager::register_uncontrolledSource(std::string src)
{
    _ucSourceNames.push_back(src);

    return SUCCESS;
}


} /** namespace */

// Expose overloaded init function
int (NRG::UtilityManager::*init)(bpn::ndarray const&, bpn::ndarray const&, bpn::ndarray const&, 
                                 bpn::ndarray const&, bpn::ndarray const&, bpn::ndarray const&) 
                                 = &NRG::UtilityManager::init;
int (NRG::UtilityManager::*init_uc)(bpn::ndarray const&, bpn::ndarray const&, bpn::ndarray const&, 
                                    bpn::ndarray const&, bpn::ndarray const&, bpn::ndarray const&, 
                                    bpn::ndarray const&, bpn::ndarray const&) 
                                    = &NRG::UtilityManager::init;

BOOST_PYTHON_MODULE(UtilityManager)
{
    bpn::initialize();
    Py_Initialize();

    bp::class_<NRG::UtilityManager>("UtilityManager")
        .def("init",                init)
        .def("init",                init_uc)
        .def("startup",             &NRG::UtilityManager::startup)
        .def("power_request",       &NRG::UtilityManager::power_request)
        .def("get_totalEmissions",  &NRG::UtilityManager::get_totalEmissions)
    ;
}
