#ifndef UTILITYMANAGER_H
#define UTILITYMANAGER_H

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include "energy_source.hpp"
#include <boost/python.hpp>
#include <boost/python/numpy.hpp>
#include <boost/python/stl_iterator.hpp>

namespace bp  = boost::python;
namespace bpn = boost::python::numpy;

namespace NRG {

class UtilityManager 
{
public:
    UtilityManager(bp::dict const& facilities);
    ~UtilityManager();

    int init();
    int power_request(float power);
    bp::tuple get_totalEmissions();

private:
    bp::dict _facilities;
    std::vector<std::shared_ptr<EnergySource>> _sources;

    void addEmissions(EnergySource::Emissions &sourceEmissions, EnergySource::Emissions &totalEmissions);
    float get_currPower();
    int convert_toSources(bp::dict const& facilities);
};

}


#endif /** UTILITYMANAGER_H */