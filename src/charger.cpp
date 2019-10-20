#include "charger.hpp"

namespace BUS {

Charger::Charger(int id, std::string name)
:
    _id(id),
    _name(name)
{}


Charger::~Charger()
{}


void
Charger::add_plugs(int numPlugs, PlugType plugType)
{
    auto it = _numPlugs.find(plugType);
    if ( it == _numPlugs.end() ){
        _numPlugs[plugType]      = numPlugs;
        _numPlugsAvail[plugType] = numPlugs;
    }
    else {
        _numPlugs[plugType]      += numPlugs;
        _numPlugsAvail[plugType] += numPlugs;
    }

    return;
}


int
Charger::get_numPlugs(PlugType plugType) const 
{
    auto it = _numPlugs.find(plugType);
    if ( it == _numPlugs.end() )
        return -1; // Could not find this plugType at this charger
    
    return it->second;
}


int
Charger::get_numPlugsAvail(PlugType plugType) const 
{
    auto it = _numPlugsAvail.find(plugType);
    if ( it == _numPlugsAvail.end() )
        return -1; // Could not find this plugType at this charger
    
    return it->second;
}


} /** namespace */
