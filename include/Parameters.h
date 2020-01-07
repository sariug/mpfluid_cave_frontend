#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_
#include <iostream>
#include <vector>
#include <string>

/** A class to store and pass around the parameters
 */
struct geometry
{
        std::string filename;
        double translate_x;
        double translate_y;
        double translate_z;
        double rotate_x;
        double rotate_y;
        double rotate_z;
        int is_primary;
        int gid;
        double temperature;
	void print_self()
	{std::cout<<filename<<" "<< translate_x<<" "<<\
	 translate_y<<" "<< translate_z<<" "<< rotate_x<<" "<< rotate_y<<" "<<rotate_z<<" "<<is_primary<<\
	 " "<<temperature<<std::endl;}
	std::vector<float> get_translation_and_rotation()
	{
		std::vector<float> tmp;
		tmp.push_back(translate_x);
		tmp.push_back(translate_y);
		tmp.push_back(translate_z);
		tmp.push_back(rotate_x);
		tmp.push_back(rotate_y);
		tmp.push_back(rotate_z);
		return tmp;
	}
};

class Parameters {
    public:
        Parameters(){}
        ~Parameters(){}

	std::string geometry_name;
	double main_bbox[6];
    double t_inf;
	std::vector<geometry> configurationGeometries;
};


#endif
