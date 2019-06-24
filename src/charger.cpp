#include "charger.hpp"

namespace BUS {

Charger::Charger(int id, std::string name, int numPlugs)
:
    _id(id),
    _name(name),
    _numPlugs(numPlugs)
{}


Charger::~Charger()
{}

} /** namespace */
