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

    int startup(float power);

    int power_request(float power);

    bp::tuple get_totalEmissions();

    int register_uncontrolledSource(std::string src);

private:
    std::vector<float> _pvProduction;
    std::vector<float> _windProduction;
    //std::vector<std::shared_ptr<EnergySource>> _sources;
    //std::vector<std::shared_ptr<EnergySource>> _uncontrolledSources;
    std::vector<std::string> _sourceNames;
    std::vector<std::string> _ucSourceNames;
    std::map<std::string, std::shared_ptr<EnergySource>> _sources;

    void addEmissions(EnergySource::Emissions &sourceEmissions, EnergySource::Emissions &totalEmissions);
    float get_currPower();
    int convert_toSources(bpn::ndarray const& sourceName, bpn::ndarray const& sourceType, 
                          bpn::ndarray const& maxCap, bpn::ndarray const& minCap,
                          bpn::ndarray const& runCost, bpn::ndarray const& rampRate);
};

}


#endif /** UTILITYMANAGER_H */