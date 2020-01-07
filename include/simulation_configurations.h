#ifndef _SIMULATION_CONFIGURATIONS_H_
#define _SIMULATION_CONFIGURATIONS_H_

#include <string>
#include "Parameters.h"

class Simulation_Configurations{
    private:
        std::string _filename;
        int _dim;

    public:
        Simulation_Configurations();
        Simulation_Configurations(const std::string & filename);
        void loadParameters(Parameters & parameters);
        void saveParameters(Parameters & parameters);
};

#endif
