#pragma once
#ifndef BASIC_DEFS_H
#define BASIC_DEFS_H
// c++ includes
#include <iostream>
#include <string.h>
#include <vector>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iterator>
#include <list>
#include <thread>
#include <functional>
#include <chrono>
#include "../../kernel/general_defs.h"

#define MAX_PATH_LENGTH 200
#define MAXIMUM_NUM_OF_GEO 20
#define MAXIMUM_POINT_VECTOR_LENGTH 500000
#define MAXIMUM_COLOR_VECTOR_LENGTH MAXIMUM_POINT_VECTOR_LENGTH * 4 / 3
#define MAXIMUM_NUM_OF_CELLS 400

typedef std::vector<std::vector<std::vector<float>>> trivectorfloat;
typedef std::vector<std::vector<float>> twovectorfloat;
typedef std::vector<float> onevectorfloat;

#define CAVE_DIMENSION 270 // Centimeters
#define IMGUI_WINDOW_X 1195
#define IMGUI_WINDOW_Y 800

struct folder_paths
{
    std::string geometry, configuration, CWD, assets;
};
// For camera rotations in the cave
enum CameraView
{
    RESET,
    XY,
    YX,
    XZ,
    ZX,
    YZ,
    ZY,
    UNDEF
};
// Slice syncher
struct slice_syncher
{   
    int  number_of_slices;
    double origins[60];
    double normals[60];
};
// Sliding Window
struct sliding_window
{
    bool show_sw = false;
    float sw_limits[6] = {0, 1, 0, 1, 0, 1};;
};
// Geometry
struct message
{
    char message[5000];
    bool show_flag;
};
struct load_config
{
    char geometries[5000];
    char bcs[5000];
    char wall_bcs[5000];
    char general_configs[5000];
    char cubemap[5000];
};
struct imgui_BC
{
    char name[128];
    float domain_min[3] = {0, 0, 0};
    float domain_max[3] = {1, 1, 1};
    int bc_type = 0;
    bool _apply_to_solids = false;
    int serial[200]; // just use for communication;
    int counter;
    float temperature;
    float U[3] = {0, 0, 0};
    template <class T>
    imgui_BC &operator=(const T &copy)
    {
        strcpy(name, copy.name);
        for (int a = 0; a < 3; a++)
        {
            domain_min[a] = copy.domain_min[a];
            domain_max[a] = copy.domain_max[a];
            U[a] = copy.U[a];
        }
        bc_type = copy.bc_type;
        _apply_to_solids = copy._apply_to_solids;
        temperature = copy.temperature;
        return *this;
    }
    void printSelf() const
    {
        std::cout << "Boundary Condition:"
                  << "\n\tName: " << name << "\n\tLocation:";
        for (int i = 0; i < 3; i++)
        {
            std::cout << domain_min[i] << " ";
            std::cout << domain_max[i] << " ";
        }
        std::cout << "\n\tU: ";
        for (int i = 0; i < 3; i++)
        {
            std::cout << U[i] << " ";
        }
        std::cout << "\n\tbc_type:" << bc_type << "\n\t_apply_to_solids: " << _apply_to_solids << "\n\tTemperature:" << temperature;
        std::cout << std::endl;
    }
    template <typename Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
    ar &name;
    ar &domain_min;
    ar &domain_max;
    ar &bc_type;
    ar &_apply_to_solids;
    ar &counter;
    ar &temperature;
    ar &U[3];
    }
};
/* Slice */
struct Slice
{
    Slice(){};
    float normal[3] = {1, 0, 0};
    float origin[3] = {0, 0, 0};
    std::string name;
    int id;
    void printSelf() const
    {
        std::cout << "Slice:"
                  << "\n\tName: " << name << "\n\tOrigin:";
        for (int i = 0; i < 3; i++)
        {
            std::cout << origin[i] << " ";
        }
        std::cout << "\n\tNormal: ";
        for (int i = 0; i < 3; i++)
        {
            std::cout << normal[i] << " ";
        }
        std::cout << std::endl;
    }
};
#endif
