#ifndef CHARGER_H
#define CHARGER_H

#include <string>
#include <map>

namespace BUS {

enum class PlugType {
    SAEJ3105 = 0,
    EVA080K  = 1
};

static std::map<std::string, PlugType> plugNameToType = {
    {"SAEJ3105",    PlugType::SAEJ3105 },
    {"EVA080K",     PlugType::EVA080K  }
};

static std::map<PlugType, std::string> plugTypeToName = {
    {PlugType::SAEJ3105,    "SAEJ3105" },
    {PlugType::EVA080K,     "EVA080K"  }
};

class Charger 
{
public:
    Charger(int id, std::string name);
    ~Charger();

    void add_plugs(int numPlugs, PlugType plugType);
    
    int get_identifier() const {return _id;}
    std::string get_name() const {return _name;}
    std::map<PlugType, int> get_numPlugs() const {return _numPlugs;}
    int get_numPlugs(PlugType plugType) const;
    int get_numPlugsAvail(PlugType plugType) const;

private:
    int _id;
    std::string _name;
    std::map<PlugType, int> _numPlugs;
    std::map<PlugType, int> _numPlugsAvail;
};

}


#endif /** CHARGER_H */