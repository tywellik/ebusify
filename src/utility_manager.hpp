#ifndef UTILITYMANAGER_H
#define UTILITYMANAGER_H

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include "energy_source.hpp"
#include "gurobi_c++.h"
#include <boost/python.hpp>
#include <boost/python/numpy.hpp>
#include <boost/python/stl_iterator.hpp>

namespace bp  = boost::python;
namespace bpn = boost::python::numpy;

namespace NRG {

class UtilityManager 
{
public:
    using ProdPtr = std::shared_ptr<double>;

    UtilityManager();
    ~UtilityManager();

    int init(bpn::ndarray const& sourceName, bpn::ndarray const& sourceType, 
            bpn::ndarray const& maxCap, bpn::ndarray const& minCap,
            bpn::ndarray const& runCost, bpn::ndarray const& rampRate,
            bpn::ndarray const& rampingCost, bpn::ndarray const& startingCost);

    int init(bpn::ndarray const& sourceName, bpn::ndarray const& sourceType, 
            bpn::ndarray const& maxCap, bpn::ndarray const& minCap,
            bpn::ndarray const& runCost, bpn::ndarray const& rampRate,
            bpn::ndarray const& rampingCost, bpn::ndarray const& startingCost,
            bpn::ndarray const& pvProduction_MW, bpn::ndarray const& windProduction_MW);

    int startup(double demandPower);

    int power_request(double demandPower);

    int set_power(std::map<std::string, double> prod, bool overrideRamps = false);

    bp::tuple get_totalEmissions();

    int register_uncontrolledSource(std::string src);

    void file_dump();

    double get_totalCost();

    void clear_memory();

private:
    std::vector<double> _pvProduction;
    std::vector<double> _windProduction;
    std::vector<std::string> _sourceNames;
    std::vector<std::string> _ucSourceNames;
    std::map<std::string, double> _sourcePrevProduction;
    std::map<std::string, SourceState> _sourcePrevState;
    std::map<std::string, std::shared_ptr<EnergySource>> _sources;

    // Time Series Data
    std::vector<double> _costValsTime;
    std::vector<std::shared_ptr<std::map<std::string, double>>> _prodValsTime;

    int run_optimization(int numSources, double* runCosts, double* rampCosts, double* startupCosts, double* minCapacity, double* maxCapacity,
                            std::string* plantNames, std::string* plantNames_on, std::string* plantNames_prod, std::string* plantNames_indOn, std::string* plantNames_indProd,
                            std::map<std::string, int> arrayLoc, double demandPower, std::map<std::string, double>& sourceProd);
    double get_currPower();
    int convert_toSources(bpn::ndarray const& sourceName, bpn::ndarray const& sourceType, 
                        bpn::ndarray const& maxCapacity, bpn::ndarray const& minCapacity,
                        bpn::ndarray const& runningCost, bpn::ndarray const& rampingRate,
                        bpn::ndarray const& rampingCost, bpn::ndarray const& startingCost);
};

}


#endif /** UTILITYMANAGER_H */