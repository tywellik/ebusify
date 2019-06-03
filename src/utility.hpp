#ifndef UTILITY_H
#define UTILITY_H

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

class Utility 
{
public:
    Utility(bp::dict const& facilities);
    ~Utility();

    int init();
    int power_request(float power);

private:
    bp::dict _facilities;
    std::vector<std::shared_ptr<EnergySource>> _sources;

    int convert_toSources(bp::dict const& facilities);
};

}


#endif /** UTILITY_H */