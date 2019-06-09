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
    UtilityManager();
    ~UtilityManager();

    int init(bpn::ndarray const& sourceName, bpn::ndarray const& sourceType, 
             bpn::ndarray const& maxCap, bpn::ndarray const& minCap,
             bpn::ndarray const& runCost, bpn::ndarray const& rampRate);

    int init(bpn::ndarray const& sourceName, bpn::ndarray const& sourceType, 
             bpn::ndarray const& maxCap, bpn::ndarray const& minCap,
             bpn::ndarray const& runCost, bpn::ndarray const& rampRate, 
             bpn::ndarray const& pvProduction_MW, bpn::ndarray const& windProduction_MW);

    int startup(double demandPower);

    int power_request(double demandPower);

    bp::tuple get_totalEmissions();

    int register_uncontrolledSource(std::string src);

private:
    std::vector<double> _pvProduction;
    std::vector<double> _windProduction;
    std::vector<std::string> _sourceNames;
    std::vector<std::string> _ucSourceNames;
    std::map<std::string, std::shared_ptr<EnergySource>> _sources;

    int run_optimization(int numSources, double* runCosts, double* minCapacity, double* maxCapacity,
                            std::string* plantNames, std::string* plantNames_on, std::string* plantNames_prod,
                            std::map<std::string, int> arrayLoc, double demandPower);
    double get_currPower();
    int convert_toSources(bpn::ndarray const& sourceName, bpn::ndarray const& sourceType, 
                            bpn::ndarray const& maxCap, bpn::ndarray const& minCap,
                            bpn::ndarray const& runCost, bpn::ndarray const& rampRate);
};

}


#endif /** UTILITYMANAGER_H */