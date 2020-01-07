// Cave
#include <renderNode.h>
#include <synchObject.h>
#include <config.h>

// Standart c++
#include <iostream>
#include <string>
#include <dirent.h>
#include <stdexcept>
#include <thread>
#include <functional>

// Selfmade scripts
#include "../include/magnum_helper.h"
#include "../include/simulation_configurations.h"
#include "utils.cpp"
#include <MagnumPlugins/AssimpImporter/AssimpImporter.h>

#ifdef CAVE
#include <Magnum/Platform/GlfwApplication.h> //FOR CAVE !
#endif

#define backaend
Matrix4x4 glm2magnum(const glm::mat4 &input)
{

    Matrix4 tmp(Vector4(input[0][0], input[0][1], input[0][2], input[0][3]),
                Vector4(input[1][0], input[1][1], input[1][2], input[1][3]),
                Vector4(input[2][0], input[2][1], input[2][2], input[2][3]),
                Vector4(input[3][0], input[3][1], input[3][2], input[3][3]));
    return tmp;
}

using namespace Math::Literals;
#ifdef CAVE
class MasterApp : public Platform::GlfwApplication
{
#else
class MainNode : public Platform::Application
{
#endif

public:
    explicit MainNode(const Arguments &arguments);

private:
    void drawEvent() override;
    void viewportEvent(ViewportEvent &event) override;
    void mouseScrollEvent(MouseScrollEvent &event) override;
    void mousePressEvent(MouseEvent &event) override;
    void mouseReleaseEvent(MouseEvent &event) override;
    void mouseMoveEvent(MouseMoveEvent &event) override;
    void keyPressEvent(KeyEvent &event) override;
    void keyReleaseEvent(KeyEvent &event) override;
    void textInputEvent(TextInputEvent &event) override;
    void render_streamlines();
    void load_configuration(std::string configuration_filename);
    void save_configuration(std::string configuration_filaname);
    void load_geometry();
    void load_geometry(std::string filename);
    void Imgui_interface_create();
    void draw_ImGui();
    Vector3 positionOnSphere(const Vector2i &position) const;

    // Scene
    Scene3D _scene;
    Object3D _manipulator, _cave_geo, _user, _cameraObject, _sliding_window, _streamline_objects, _static_objects, _geometry_from_file; // Data object(static) geometry object(s)
    SceneGraph::Camera3D *_camera;
    SceneGraph::DrawableGroup3D _drawables, _sliding_window_drawable;
    Vector3 _previousPosition;

    // Geometry Variables
    int number_of_geometries = 0;
    int maximum_available_geometry = 20;
    // Variables
    Float geo_bbox[6];
    Float sw_bbox[6];
    Rectengle *SLIDING_WINDOW = nullptr;
    Rectengle *GEOMETRY_BBOX = nullptr;
    Rectengle *CAVE_GEOMETRY = nullptr;
    std::vector<StreamTracer *> STREAMTRACERHOLDER;
    std::vector<Imported_Geometry *> GEOMETRY_HOLDER;
    Object3D *Geometries[20];

    // Imgui !
    ImGuiIntegration::Context _imgui{NoCreate};
    bool _sliding_window_gui = true;
    bool _show_geo_add = false;
    bool _show_geo_modify = false;
    bool _show_geo_delete = false;
    float _translate[3] = {0, 0, 0}; // for modification of geometry
    float _rotate[3] = {0, 0, 0};    // for modfication of geometry
    Float _floatValue = 0.0f;

    //Sliding window variables [UI]
    Color3 sliding_window_color = 0x00ff00_rgbf;   // Green
    Color3 geometry_outline_color = 0x0000ff_rgbf; // Blue
    Float _sliding_x = 0.0f;
    Float _sliding_y = 0.0f;
    Float _sliding_z = 0.0f;
    bool _showSW = false;
    // Domain bounding box
    Float domain_bbox_min[3] = {0.0, 0.0, 0.0};
    Float domain_bbox_max[3] = {18.0, 25.0, 36.0};

    // synchlib stuff
    std::shared_ptr<synchlib::renderNode> rNode;
    std::shared_ptr<synchlib::udpSynchObject<long long>> timeSyncher;
    std::shared_ptr<synchlib::udpSynchObject<glm::bvec4>> keyboardSyncher;
    std::shared_ptr<synchlib::udpSynchObject<glm::bvec4>> wandSyncher;
    std::shared_ptr<synchlib::udpSynchObject<glm::ivec2>> buttonSyncher;
    synchlib::caveConfig m_conf;
    long long m_lastTime = 0;
    std::chrono::high_resolution_clock::time_point time_start;
};
