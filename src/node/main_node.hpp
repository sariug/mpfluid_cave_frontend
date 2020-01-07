#ifndef MAIN_NODE_HPP
#define MAIN_NODE_HPP
// Cave
#include <renderNode.h>
#include <synchObject.h>
#ifdef CAVE
#include <config.h>
#else
#include <configSYNCH.h>
#endif
// Selfmade scripts
#include "../../include/basic_defs.h"
#include "../../include/magnum_helper.h"
#include "../../include/eaf_loader.h"
#include "../../include/vtkProcessor.h"
#include "../../include/ImGui_interface.h"
#include "../../../kernel/fluid.h"

//#define TAKE_PHOTO
#define CONSTRUCT_CUBEMAP
namespace Magnum
{

using namespace Math::Literals;


class MainNode : public Platform::Application
{
public:
    explicit MainNode(const Arguments &arguments);
    ~MainNode();

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
    void render_field_data();
    void load_configuration();
    void load_geometry(std::string filename);
    void move_geometry(int id, onevectorfloat &change);                     // for imgui
    void move_geometry_in_configuration(int j, onevectorfloat &attributes); // for configuration
    void set_working_directories();
    void render_boundary_conditions();
    void scale_drawables_for_cave();
    void update_scaling_factors();
    void construct_cubemap(std::string* cubemap=nullptr);
    void change_camera_view(CameraView X);
    void render_gui();
    void SliceField();
    void setup_imgui();
    void update_synchers();
    void process_wand();
    void check_imgui();
    void update_imgui(int x);
    void log_message(std::string log);

    /*  GUI FRAME BUFFER
     */

    GL::Framebuffer framebuffer{{Vector2i(0, 0), Vector2i(IMGUI_WINDOW_X, IMGUI_WINDOW_Y)}};

    //    GL::Framebuffer framebuffer{GL::defaultFramebuffer.viewport()};
    GL::Texture2D color, normal;

    bool show_scaled = true;
    // Folder Paths
    folder_paths paths;

    // Scene
    Scene3D _scene;
    Object3D _cave_geo, _cameraObject, _sliding_window, _streamline_objects, _cubemap_object,
        _slice_objects, _static_objects, _temperature_objects, gui_object, _bc_objects; // Data object(static) geometry object(s)
    SceneGraph::Camera3D *_camera;
    SceneGraph::DrawableGroup3D _drawables, _slices_drawable, _sliding_window_drawable, _bc_drawables, floor_drawable, _gui_drawables,_streamline_drawables,_temperature_drawables;
    Vector3 _previousPosition;
    Vector2i _previousMousePosition, _mousePressPosition;
    // Geometry Variables
    std::size_t number_of_geometries = 0;
    const std::size_t maximum_available_geometry = 20;
    // Variables
    Float sw_bbox[6];
    Rectengle *SLIDING_WINDOW = nullptr;
    Rectengle *GEOMETRY_BBOX = nullptr;
    Rectengle *CAVE_GEOMETRY = nullptr;
    GUIPLANE *cave_gui = nullptr;
    Examples::CubeMap *CUBEMAP = nullptr;
    Rectengle *floorPlane = nullptr;
    std::vector<TemperatureSphere *> TEMPERATUREHOLDER;
    ScalarPoints *PointHolder = nullptr;
    std::vector<StreamTracer *> STREAMTRACERHOLDER;
    std::vector<Imported_Geometry *> GEOMETRY_HOLDER;
    std::vector<Rectengle *> BC_HOLDER;
    std::vector<SliceVR *> SLICEHOLDER;

    Object3D *Geometries[20];
    Object3D *gui_plane;
    GL::Mesh _plane{NoCreate};
    Magnum::GL::Texture2D _texture;
    Examples::CubeMapResourceManager _resourceManager;
    PluginManager::Manager<Trade::AbstractImporter> manager;

    // Cave variables
    Float cave_geo[6] = {0, CAVE_DIMENSION, 0, CAVE_DIMENSION, 0, CAVE_DIMENSION};

    // Scaling
    bool scale_up = true;
    float scaling_factor = 100.0f;
    float scaling_factorx = 100.0f;
    float scaling_factory = 100.0f;
    float scaling_factorz = 100.0f;
    Vector3 scaling_translate = {0.f, 0.f, 0.f};
    Vector3 sw_cog, sw_translate = {0.f, 0.f, 0.f};

    //Sliding window variables [UI]
    Color3 sliding_window_color = 0x00ff00_rgbf;   // Green
    Color3 geometry_outline_color = 0x0000ff_rgbf; // Blue
    Float _sliding_x = 0.0f;
    Float _sliding_y = 0.0f;
    Float _sliding_z = 0.0f;
    bool _showSW = true;

    FloorTextureShader _shader;
    bool show_gui = false;
    bool show_slices = true;
    bool show_bc = false;
    bool show_streamlines = true;
    bool show_temperatures = true;
    // VTK processor
    std::shared_ptr<vtkProcessor> Processor;
    // GUIma
    ImGui_gui *_imgui = new ImGui_gui();
    float gui_cursor_speed = 1.0f;
    bool virtual_mouse_commands[4] = {false, false, false, false};
    Scene2D _scene_gui;
    Object2D *_cameraObject_gui;
    Object2D *virtual_cursor;
    SceneGraph::Camera2D *_camera_gui;
    SceneGraph::DrawableGroup2D _drawables_gui;
    glm::ivec2 imgui_pos = {0, 0};
    /* Variables for synchronization with backend */
    std::vector<geometry_master *> geometries;
    general_configurations gConfs;
    std::vector<BC_struct> BC_holder;
    std::vector<BC_struct> Wall_BC_holder;

    // synchlib stuff
    std::shared_ptr<synchlib::renderNode> rNode;
    std::shared_ptr<synchlib::udpSynchObject<long long>> timeSyncher;
    std::shared_ptr<synchlib::udpSynchObject<sliding_window>> swsyncher;
    std::shared_ptr<synchlib::udpSynchObject<int>> visdataSyncher;
    // Geometry Synch Stuff
    std::shared_ptr<synchlib::udpSynchObject<load_config>> configureSyncher;
    std::shared_ptr<synchlib::udpSynchObject<message>> bcSyncher;
    std::shared_ptr<synchlib::udpSynchObject<int>> cameraSyncher;
    std::shared_ptr<synchlib::udpSynchObject<slice_syncher>> sliceSyncher;
    std::shared_ptr<synchlib::udpSynchObject<bool>> showSlices;
    std::shared_ptr<synchlib::udpSynchObject<glm::ivec2>> analogsyncher;
    std::shared_ptr<synchlib::udpSynchObject<glm::bvec4>> buttonSyncher;
    std::shared_ptr<synchlib::udpSynchObject<message>> logSyncher;
    std::shared_ptr<synchlib::udpSynchObject<bool>> backendSyncher;

    synchlib::caveConfig m_conf;
    long long m_lastTime = 0;
    std::chrono::high_resolution_clock::time_point time_start;
};

#endif