#define BOOST_TEST_DYN_LINK
#ifdef STAND_ALONE
#define BOOST_TEST_MODULE Main
#endif
#include <boost/test/unit_test.hpp>
// Gonna be added if works good !
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "../include/connect_fluconnector.h"

// Prepared according to
// https://gist.github.com/endJunction/4030890

std::string filename = "/home/ugurcan/Desktop/mpfluid_vis_sari/src/scenograph/geometry_configurations/op_room_standard.xml";
std::string savename = "/home/ugurcan/Desktop/mpfluid_vis_sari/src/scenograph/geometry_configurations/testsave.xml";

std::vector<geometry_master *> A;
std::vector<BC_struct> B, WB;
general_configurations C;
int number_of_geometries = 27;
bool loaded = false;

void loadConfigurationMASTER(std::string configration_name, std::vector<geometry_master *> &geometries, std::vector<BC_struct> &BC_holder, std::vector<BC_struct> &Wall_BC_holder, general_configurations &gConfs)
{
    loaded = true;
    // Reset Current items such as Geometry HOLDER !
    geometries.clear();
    BC_holder.clear();
    Wall_BC_holder.clear();
    StaticCounter::geometry_number = 0;
    std::vector<std::string> filenames;
    std::map<std::string, SCENEGRAPH_BC> x__map = {{"INFLOW_TEMP", SCENEGRAPH_BC::INFLOW_TEMP}, {"OUTFLOW", SCENEGRAPH_BC::OUTLET}, {"SOLID", SCENEGRAPH_BC::SOLID}, {"INFLOW", SCENEGRAPH_BC::INFLOW}};
    // Create an empty property tree object
    using boost::property_tree::ptree;
    ptree pt;

    read_xml(filename, pt);

    // Geometries
    for (ptree::value_type &v : pt.get_child("Geometries"))
    {
        geometry_master *geo = new geometry_master(v.second.get<std::string>("filename"));
        float val;
        std::stringstream lineStream(v.second.get<std::string>("position"));
        auto it = geo->position_modification.begin();
        while (lineStream >> val)
        {
            *it = val;
            it++;
        }
        geo->temperature = v.second.get<float>("temperature");
        geo->gid_attribute = v.second.get<int>("gid_attribute");
        geo->is_scale_according_to_bbox = v.second.get<int>("is_scale_according_to_bbox");
        geo->is_primary_geometry = v.second.get<int>("is_primary");
        geometries.push_back(geo);
    }
    number_of_geometries = geometries.size();
    // Boundary conditions
    for (ptree::value_type &v : pt.get_child("Boundary_Conditions"))
    {
        BC_struct bc;
        strcpy(bc.name, v.second.get<std::string>("name").c_str());
        bc.bc_type = x__map[v.second.get<std::string>("type")];
        float val;
        std::vector<float> values;
        std::stringstream lineStream(v.second.get<std::string>("bbox"));
        while (lineStream >> val)
            values.push_back(val);
        for (int i = 0; i < 3; i++)
        {
            bc.domain_min[i] = values[2 * i];
            bc.domain_max[i] = values[2 * i + 1];
        }
        values.clear();
        bc.temperature = v.second.get<float>("temperature");
        std::stringstream lineStream1(v.second.get<std::string>("velocity"));
        while (lineStream1 >> val)
            values.push_back(val);
        for (int i = 0; i < 3; i++)
            bc.U[i] = values[i];
        bc._apply_to_solids = v.second.get<bool>("apply_to_solids");
        bc.printSelf();
        BC_holder.push_back(bc);
    }
    // global configurations
    gConfs.t_inf = pt.get<double>("Global_Configuration.T_inf");
    float val;
    std::vector<float> values;
    std::stringstream lineStream(pt.get_child("Global_Configuration.main_bounding_box").data());
    while (lineStream >> val)
        values.push_back(val);
    for (int i = 0; i < 3; i++)
    {
        gConfs.domain_bbox_min[i] = values[2 * i];
        gConfs.domain_bbox_max[i] = values[2 * i + 1];
    }
    std::vector<int> mesh_values;
    std::stringstream meshStreamS(pt.get_child("Global_Configuration.mesh_s").data());
    std::stringstream meshStreamR(pt.get_child("Global_Configuration.mesh_r").data());
    std::stringstream meshStreamB(pt.get_child("Global_Configuration.mesh_b").data());
    while (meshStreamS >> val)
        mesh_values.push_back(val);
    for (int i = 0; i < 3; i++)
        gConfs.mesh_s[i] = mesh_values[i];
    mesh_values.clear();
    while (meshStreamR >> val)
        mesh_values.push_back(val);
    for (int i = 0; i < 3; i++)
        gConfs.mesh_r[i] = mesh_values[i];
    mesh_values.clear();
    while (meshStreamB >> val)
        mesh_values.push_back(val);
    for (int i = 0; i < 3; i++)
        gConfs.mesh_b[i] = mesh_values[i];
    mesh_values.clear();
    gConfs.mesh_depth = pt.get<int>("Global_Configuration.mesh_depth");

    gConfs.printSelf();

    // Wall Boundary conditions Wall_BC_holder
    for (ptree::value_type &v : pt.get_child("Wall_Boundaries"))
    {
        BC_struct bc;
        strcpy(bc.name, v.second.get<std::string>("name").c_str());
        bc.bc_type = x__map[v.second.get<std::string>("type")];
        float val;
        std::vector<float> values;
        bc.temperature = v.second.get<float>("temperature");
        std::stringstream lineStream1(v.second.get<std::string>("velocity"));
        while (lineStream1 >> val)
            values.push_back(val);
        for (int i = 0; i < 3; i++)
            bc.U[i] = values[i];
        bc.printSelf();
        Wall_BC_holder.push_back(bc);
    }
    /*
    auto dummy_configure = configureSyncher->getData();
    strcpy(dummy_configure.geometries, get_serialized_data(geometries).c_str());
    strcpy(dummy_configure.bcs, get_serialized_data(BC_holder).c_str());
    strcpy(dummy_configure.wall_bcs, get_serialized_data(Wall_BC_holder).c_str());
    strcpy(dummy_configure.general_configs, get_serialized_data(gConfs).c_str());

    std::stringstream ss(pt.get<std::string>("Cubemap"));
    std::string item;
    std::getline(ss, item);
    strcpy(dummy_configure.cubemap, item.c_str());
    configureSyncher->setData(dummy_configure);
    configureSyncher->send();

    this->setup_imgui();
    //send_BC_to_frontend();
    this->updateSlidingWindow();
    */
}
void saveConfiguration(std::string configration_name, std::vector<geometry_master *> &geometries, std::vector<BC_struct> &BC_holder, std::vector<BC_struct> &Wall_BC_holder, general_configurations &gConfs)
{
    boost::property_tree::ptree pt;
    /* 
    General Configurations 
    */
    // - Main bbox
    std::vector<float> values;
    for (int i = 0; i < 3; i++)
    {
        values.push_back(gConfs.domain_bbox_min[i]);
        values.push_back(gConfs.domain_bbox_max[i]);
    }
    std::ostringstream vts;
    std::copy(values.begin(), values.end() - 1,
              std::ostream_iterator<float>(vts, " "));
    vts << values.back();

    pt.put("Global_Configuration.main_bounding_box", vts.str());
    // - T_inf
    pt.put("Global_Configuration.T_inf", gConfs.t_inf);
    // - mesh_s
    values.clear();
    for (int i = 0; i < 3; i++)
    {
        values.push_back(gConfs.mesh_s[i]);
    }
    vts.str("");
    vts.clear();
    std::copy(values.begin(), values.end() - 1,
              std::ostream_iterator<int>(vts, " "));
    vts << values.back();
    pt.put("Global_Configuration.mesh_s", vts.str());

    // - mesh_r
    values.clear();
    for (int i = 0; i < 3; i++)
    {
        values.push_back(gConfs.mesh_r[i]);
    }
    vts.str("");
    vts.clear();
    std::copy(values.begin(), values.end() - 1,
              std::ostream_iterator<int>(vts, " "));
    vts << values.back();
    pt.put("Global_Configuration.mesh_r", vts.str());

    // - mesh_b
    values.clear();
    for (int i = 0; i < 3; i++)
    {
        values.push_back(gConfs.mesh_b[i]);
    }
    vts.str("");
    vts.clear();
    std::copy(values.begin(), values.end() - 1,
              std::ostream_iterator<int>(vts, " "));
    vts << values.back();
    pt.put("Global_Configuration.mesh_b", vts.str());

    // - mesh_depth
    pt.put("Global_Configuration.mesh_depth", gConfs.mesh_depth);

    /* 
    Geometries
    */
    for (auto g : geometries)
    {
        boost::property_tree::ptree dummy_pt;
        // - name
        dummy_pt.add("Geometry.filename", g->file_name);
        // - position
        values.clear();
        for (int i = 0; i < 6; i++)
        {
            values.push_back(g->position_modification[i]);
        }
        vts.str("");
        vts.clear();
        std::copy(values.begin(), values.end() - 1,
                  std::ostream_iterator<float>(vts, " "));
        vts << values.back();
        dummy_pt.put("Geometry.position", vts.str());
        // - is primary
        dummy_pt.put("Geometry.is_primary", g->is_primary_geometry);

        // - gid attribute
        dummy_pt.put("Geometry.gid_attribute", g->gid_attribute);

        // - temperature
        dummy_pt.put("Geometry.temperature", g->temperature);

        // - is_scale_according to bbox
        dummy_pt.put("Geometry.is_scale_according_to_bbox", g->is_scale_according_to_bbox);
        pt.add_child("Geometries.Geometry", dummy_pt.get_child("Geometry"));
    }

    /* 
    Boundary Conditions
    */
    std::string x__map_reverse[4] = {"INFLOW_TEMP", "OUTFLOW", "SOLID", "INFLOW"};

    for (auto bc : BC_holder)
    {
        boost::property_tree::ptree dummy_pt;
        // - name
        dummy_pt.add("BC.name", bc.name);
        // - type
        dummy_pt.add("BC.type", x__map_reverse[bc.bc_type]); // FIX
        // - bbox
        values.clear();
        for (int i = 0; i < 3; i++)
        {
            values.push_back(bc.domain_min[i]);
            values.push_back(bc.domain_max[i]);
        }
        vts.str("");
        vts.clear();
        std::copy(values.begin(), values.end() - 1,
                  std::ostream_iterator<float>(vts, " "));
        vts << values.back();
        dummy_pt.put("BC.bbox", vts.str());
        // - velocity
        values.clear();
        for (int i = 0; i < 3; i++)
        {
            values.push_back(bc.U[i]);
        }
        vts.str("");
        vts.clear();
        std::copy(values.begin(), values.end() - 1,
                  std::ostream_iterator<float>(vts, " "));
        vts << values.back();
        dummy_pt.put("BC.velocity", vts.str());
        // - temperature
        dummy_pt.put("BC.temperature", bc.temperature);

        // - apply_to_solids
        dummy_pt.put("BC.apply_to_solids", static_cast<int>(bc._apply_to_solids));
        pt.add_child("Boundary_Conditions.BC", dummy_pt.get_child("BC"));
    }
        /* 
    Wall Boundary Conditions
    */

    for (auto bc : Wall_BC_holder)
    {
        boost::property_tree::ptree dummy_pt;
        // - name
        dummy_pt.add("Wall.name", bc.name);
        // - type
        dummy_pt.add("Wall.type", x__map_reverse[bc.bc_type]); // FIX
        // - velocity
        values.clear();
        for (int i = 0; i < 3; i++)
        {
            values.push_back(bc.U[i]);
        }
        vts.str("");
        vts.clear();
        std::copy(values.begin(), values.end() - 1,
                  std::ostream_iterator<float>(vts, " "));
        vts << values.back();
        dummy_pt.put("Wall.velocity", vts.str());
        // - temperature
        dummy_pt.put("Wall.temperature", bc.temperature);

        pt.add_child("Wall_Boundaries.Wall", dummy_pt.get_child("Wall"));
    }
    pt.put("Cubemap", "");
    write_xml(configration_name, pt);
}

BOOST_AUTO_TEST_SUITE(test1_suite)

BOOST_AUTO_TEST_CASE(ConfigurationLoading)
{

    loadConfigurationMASTER(filename, A, B, WB, C);

    BOOST_CHECK(4 == number_of_geometries);
}
BOOST_AUTO_TEST_CASE(ConfigurationSaving)
{

    loadConfigurationMASTER(filename, A, B, WB, C);
    saveConfiguration(savename, A, B, WB, C);
}
BOOST_AUTO_TEST_SUITE_END()