#ifndef CHARGER_H
#define CHARGER_H

#include <string>

namespace BUS {

class Charger 
{
public:
    Charger(int id, std::string name, int numPlugs);
    ~Charger();
    
    int get_identifier() const {return _id;}
    std::string get_name() const {return _name;}
    int get_numPlugs() const {return _numPlugs;}
    int get_numPlugsAvail() const {return _numPlugsAvail;}

private:
    int _id;
    std::string _name;
    int _numPlugs;
    int _numPlugsAvail;
};

}


#endif /** CHARGER_H */