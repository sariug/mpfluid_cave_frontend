//Cave stuff
//Include this before everything else !
#include <renderServer.h>
#include <synchObject.h>
#ifdef CAVE
#include <config.h>
#else
#include <configSYNCH.h>
#endif
#include "../../include/connect_fluconnector.h"
#include "../../include/magnum_helper.h"
#include "../../include/ImGui_interface.h"

namespace Magnum
{
using namespace Math::Literals;

auto controller = new control_backend;

class MasterApp : public Platform::GlfwApplication
{
public:
    explicit MasterApp(const Arguments &arguments);
    ~MasterApp()
    {
        if (rServer.get())
        {
            rServer->stopSynching();
        }
    }

private:
    /* VIRTUAL CURSOR */
    GL::Mesh _mesh{NoCreate};
    Shaders::Flat2D _shader{NoCreate};

    Scene2D _scene;
    Object2D *_cameraObject;
    Object2D *virtual_cursor;
    SceneGraph::Camera2D *_camera;
    SceneGraph::DrawableGroup2D _drawables;

    void set_working_directories();
    void drawEvent() override;
    // Mouse events for imgui
    void mouseScrollEvent(MouseScrollEvent &event) override;
    void mousePressEvent(MouseEvent &event) override;
    void mouseReleaseEvent(MouseEvent &event) override;
    void mouseMoveEvent(MouseMoveEvent &event) override;
    // Keyboard events
    void keyPressEvent(KeyEvent &event);
    void keyReleaseEvent(KeyEvent &event);
    void textInputEvent(TextInputEvent &event);
    void viewportEvent(ViewportEvent &event) override;
    // Sliding window
    void updateSlidingWindow();
    void updateCameraView(int camera_view);
    // Geometry
    int add_geometry(std::string filename);
    void delete_geometry(int geometry_delete_num);
    void load_configuration(std::string configration_name);
    void save_configuration(std::string configuration_filaname);
    void setup_imgui();

    void send_scene_to_backend();
    void create_streamlines();
    void create_temperature_field();

    void log_message(std::string log);
    // Boundary conditions
    void send_BC_to_frontend();
    // Slices conditions
    void send_slices_to_frontend();
    void update_imgui(int x);

    void process_wand();
    // Folder Paths
    folder_paths paths;

    // Geometry variables
    int number_of_geometries = 0;
    // int maximum_available_geometry = 20;
    std::vector<geometry_master *> geometries;
    general_configurations gConfs;
    std::vector<BC_struct> BC_holder;
    std::vector<BC_struct> Wall_BC_holder;
    // Sliding window master
    sliding_window sw_master;
    bool succesful_sizing = true;
    bool show_bc = false;
    bool loaded = false;
    // Synch structs  = = camelCase
    // Slice holder
    std::vector<Slice *> slices;
    // synchlib stuff
    std::shared_ptr<synchlib::renderServer> rServer;
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

    std::chrono::high_resolution_clock::time_point time_start;

    // IMGUI
    ImGui_gui *_imgui = new ImGui_gui();
    void check_imgui_io();
    float gui_cursor_speed_x = 1.0f * IMGUI_WINDOW_X / CAVE_DIMENSION;
    float gui_cursor_speed_y = 1.0f * IMGUI_WINDOW_Y / CAVE_DIMENSION;
    bool virtual_mouse_commands[4] = {false, false, false, false};
};

MasterApp::MasterApp(const Arguments &arguments) : Platform::Application{
                                                       arguments,
                                                       Configuration{}.setTitle("Controller").setWindowFlags(Configuration::WindowFlag::Resizable).setSize(Magnum::Vector2i(IMGUI_WINDOW_X, IMGUI_WINDOW_Y))}
{
    set_working_directories();

    /*!!!#######################################################################
                IMGUI 
    !!!#######################################################################*/
    _imgui->_imgui = ImGuiIntegration::Context(Vector2{windowSize()} / dpiScaling(),
                                               windowSize(), framebufferSize());
    _imgui->is_server = true;
    /*!!!#######################################################################
                IMGUI 
    !!!#######################################################################*/
    _cameraObject = new Object2D{&_scene};
    _cameraObject->rotate(90.0_degf);
    _camera = new SceneGraph::Camera2D{*_cameraObject};
    _camera->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix3::projection({IMGUI_WINDOW_X, IMGUI_WINDOW_Y}))
        .setViewport(GL::defaultFramebuffer.viewport().size());
    _cameraObject->translate(Vector2(IMGUI_WINDOW_Y / 2, IMGUI_WINDOW_X / 2));
    virtual_cursor = new VirtualCursor{_scene, _drawables};
    virtual_cursor->setScaling(Vector2(10.0f, 10.0f));

    // Object2D *Dummy, *Dummy2;
    // Dummy = new VirtualCursor{_scene, _drawables};
    // Dummy->setScaling(Vector2(5.0f, 5.0f));
    // Dummy->setTranslation(Vector2(.0f, .0f));
    // Dummy2 = new VirtualCursor{_scene, _drawables};
    // Dummy2->setScaling(Vector2(5.0f, 5.0f));
    // Dummy2->setTranslation(Vector2(IMGUI_WINDOW_X, IMGUI_WINDOW_Y));
    /*##################################################################
            SYNCH
    ##################################################################*/
    // Define Rnode
    rServer = std::make_shared<synchlib::renderServer>(arguments.argv[1], arguments.argc, arguments.argv);
    /*##################################################################
                Create Objects
        ##################################################################*/
    timeSyncher = synchlib::udpSynchObject<long long>::create();
    swsyncher = synchlib::udpSynchObject<sliding_window>::create();

    //visdataSyncher = synchlib::tcpSynchObject<streamline_syncher >::create();
    visdataSyncher = synchlib::udpSynchObject<int>::create();
    configureSyncher = synchlib::udpSynchObject<load_config>::create();
    bcSyncher = synchlib::udpSynchObject<message>::create();
    cameraSyncher = synchlib::udpSynchObject<int>::create();
    sliceSyncher = synchlib::udpSynchObject<slice_syncher>::create();
    showSlices = synchlib::udpSynchObject<bool>::create();
    analogsyncher = synchlib::udpSynchObject<glm::ivec2>::create();
    buttonSyncher = synchlib::udpSynchObject<glm::bvec4>::create();
    logSyncher = synchlib::udpSynchObject<message>::create();
    backendSyncher = synchlib::udpSynchObject<bool>::create();

    /*##################################################################
                Add objects to rServer
        ##################################################################*/
    rServer->addSynchObject(timeSyncher, synchlib::renderServer::SENDER, 0, 0);
    rServer->addSynchObject(swsyncher, synchlib::renderServer::SENDER, 0, 0);
    rServer->addSynchObject(visdataSyncher, synchlib::renderServer::SENDER, 0, 0);
    rServer->addSynchObject(configureSyncher, synchlib::renderServer::SENDER, 0, 0);
    rServer->addSynchObject(bcSyncher, synchlib::renderServer::SENDER, 0, 0);
    rServer->addSynchObject(cameraSyncher, synchlib::renderServer::SENDER, 0, 0);
    rServer->addSynchObject(sliceSyncher, synchlib::renderServer::SENDER, 0, 0);
    rServer->addSynchObject(showSlices, synchlib::renderServer::SENDER, 0, 0);
    rServer->addSynchObject(analogsyncher, synchlib::renderServer::SENDER, 0, 0);
    rServer->addSynchObject(buttonSyncher, synchlib::renderServer::SENDER, 0, 0);
    rServer->addSynchObject(logSyncher, synchlib::renderServer::SENDER, 0, 0);
    rServer->addSynchObject(backendSyncher, synchlib::renderServer::SENDER, 0, 0);

    rServer->init();

    rServer->startSynching();
    /*##################################################################
            SYNCH
    ##################################################################*/
    // There needs sole latency for nodes to catch up.
    //#ifdef CAVE
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    //#endif
    //load_configuration("cavity_flow.xml");
}

void MasterApp::drawEvent()
{
    GL::Renderer::setClearColor(Color4(0.0, 0, 0, 0.f));
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    if (ImGui::GetIO().WantTextInput && !isTextInputActive())
        startTextInput();
    else if (!ImGui::GetIO().WantTextInput && isTextInputActive())
        stopTextInput();

    process_wand();
    Vector2 Pos = virtual_cursor->absoluteTransformation().translation();
    _imgui->draw_interface(virtual_mouse_commands, Pos.y(), Pos.x()); // y to x; x to y for reversing frame
    check_imgui_io();

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    _camera->draw(_drawables);

    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
    rServer->useDefaultNavigation(false);

    auto timeNow = std::chrono::high_resolution_clock::now();
    long long dur = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - time_start).count();
    if (!loaded)
        load_configuration("op_room_standard.xml");
    timeSyncher->setData(dur);
    timeSyncher->send();
    swapBuffers();
    redraw();
}

// Update Sliding Window
void MasterApp::updateSlidingWindow()
{
    auto tmp = swsyncher->getData();
    swsyncher->setData(sw_master);
    swsyncher->send();
}
// Update Camera
void MasterApp::updateCameraView(int camera_view)
{
    auto tmp = cameraSyncher->getData();
    cameraSyncher->setData(camera_view);
    cameraSyncher->send();
}

void MasterApp::set_working_directories()
{
    char cwd[MAX_PATH_LENGTH];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("Current working dir: %s\n", cwd);
    }
    else
    {
        perror("getcwd() error");
    }
    std::string cwd_str = cwd;
    if (cwd_str.substr(cwd_str.length() - 5) == "build")
    {
        paths.geometry = cwd_str.substr(0, cwd_str.length() - 5) + "src/scenograph/geometry_files/";
        paths.configuration = cwd_str.substr(0, cwd_str.length() - 5) + "src/scenograph/geometry_configurations/";
        std::cout << "Geometry folder : " << paths.geometry << std::endl;
        ;
        std::cout << "Configuration folder : " << paths.configuration << std::endl;
        ;
    }
    else if (cwd_str.substr(cwd_str.length() - 3) == "bin")
    {
        paths.geometry = cwd_str.substr(0, cwd_str.length() - 3) + "geo_examples/";
        paths.configuration = cwd_str.substr(0, cwd_str.length() - 3) + "geometry_configurations/";
        // Debug{}<<"ImGui Geometry folder : "<< paths.geometry;
        // Debug{}<<"ImGui Configuration folder : "<< paths.configuration;
    }
    else
        throw "In which folders  I am ?";
}

void MasterApp::keyPressEvent(KeyEvent &event)
{
    if (_imgui->_imgui.handleKeyPressEvent(event))
        return;

    else if (event.key() == KeyEvent::Key::NumOne)
    {
    }
    else if (event.key() == KeyEvent::Key::K)
    {
        Magnum::DebugTools::screenshot(GL::defaultFramebuffer, "Wow.tga");
    }
    else if (event.key() == KeyEvent::Key::NumEight)
    {
        virtual_cursor->setTranslation(Vector2(virtual_cursor->translation().x() - gui_cursor_speed_x, virtual_cursor->translation().y()));
        glm::ivec2 cursor_pos = analogsyncher->getData();
        Vector2 Pos = virtual_cursor->absoluteTransformation().translation();
        cursor_pos[0] = Pos.y();
        cursor_pos[1] = Pos.x();
        analogsyncher->setData(cursor_pos);
        analogsyncher->send();
    }
    else if (event.key() == KeyEvent::Key::NumTwo)
    {
        virtual_cursor->setTranslation(Vector2(virtual_cursor->translation().x() + gui_cursor_speed_x, virtual_cursor->translation().y()));
        glm::ivec2 cursor_pos = analogsyncher->getData();
        Vector2 Pos = virtual_cursor->absoluteTransformation().translation();
        cursor_pos[0] = Pos.y();
        cursor_pos[1] = Pos.x();
        analogsyncher->setData(cursor_pos);
        analogsyncher->send();
    }
    else if (event.key() == KeyEvent::Key::NumSix)

    {
        virtual_cursor->setTranslation(Vector2(virtual_cursor->translation().x(), virtual_cursor->translation().y() + gui_cursor_speed_y));
        glm::ivec2 cursor_pos = analogsyncher->getData();
        Vector2 Pos = virtual_cursor->absoluteTransformation().translation();
        cursor_pos[0] = Pos.y();
        cursor_pos[1] = Pos.x();
        analogsyncher->setData(cursor_pos);
        analogsyncher->send();
    }
    else if (event.key() == KeyEvent::Key::NumFour)
    {
        virtual_cursor->setTranslation(Vector2(virtual_cursor->translation().x(), virtual_cursor->translation().y() - gui_cursor_speed_y));
        glm::ivec2 cursor_pos = analogsyncher->getData();
        Vector2 Pos = virtual_cursor->absoluteTransformation().translation();
        cursor_pos[0] = Pos.y();
        cursor_pos[1] = Pos.x();
        analogsyncher->setData(cursor_pos);
        analogsyncher->send();
    }
    if (event.key() == KeyEvent::Key::Space)
    {
        virtual_mouse_commands[2] = true;
    }
}
//SLIDING_WINDOW->x_min = SLIDING_WINDOW->x_min +1.0f;
void MasterApp::keyReleaseEvent(KeyEvent &event)
{
    if (event.key() == KeyEvent::Key::Space)
    {
        virtual_mouse_commands[2] = false;
    }
    if (_imgui->_imgui.handleKeyReleaseEvent(event))
        return;
}

void MasterApp::textInputEvent(TextInputEvent &event)
{
    if (_imgui->_imgui.handleTextInputEvent(event))
        return;
}

// MOUSE EVENTS FOR IMGUI
void MasterApp::mouseScrollEvent(MouseScrollEvent &event)
{
    if (_imgui->_imgui.handleMouseScrollEvent(event))
        return;
}
void MasterApp::mousePressEvent(MouseEvent &event)
{
    //    if ()
    //        return;
    // Debug{}<<event.button();
    //
    if (event.button() == MouseEvent::Button::Left)
    {
        glm::ivec2 cursor_pos = analogsyncher->getData();
        virtual_cursor->setTranslation(Vector2(event.position().y(), event.position().x()));
        cursor_pos[0] = event.position().x();
        cursor_pos[1] = event.position().y();
        analogsyncher->setData(cursor_pos);
        analogsyncher->send();

        glm::bvec4 buttonPressed = buttonSyncher->getData();
        buttonPressed[2] = true;
        buttonSyncher->setData(buttonPressed);
        buttonSyncher->send();
    }
    if (event.button() == MouseEvent::Button::Middle)
    {
        glm::bvec4 buttonPressed = buttonSyncher->getData();
        buttonPressed[0] = true;
        buttonSyncher->setData(buttonPressed);
        buttonSyncher->send();
    }
    _imgui->_imgui.handleMousePressEvent(event);
    redraw();
}
void MasterApp::mouseReleaseEvent(MouseEvent &event)
{
    //if (_imgui->_imgui.handleMouseReleaseEvent(event))
    //  return;
    _imgui->_imgui.handleMouseReleaseEvent(event);
    glm::bvec4 buttonPressed = buttonSyncher->getData();
    buttonPressed[2] = false;
    buttonPressed[0] = false;
    //virtual_mouse_commands[2] = false;
    buttonSyncher->setData(buttonPressed);
    buttonSyncher->send();
    redraw();
}
void MasterApp::mouseMoveEvent(MouseMoveEvent &event)
{
    if (_imgui->_imgui.handleMouseMoveEvent(event))
        return;
}
void MasterApp::check_imgui_io()
{
    // Simulation Running ?
    _imgui->simulation_backend_running = controller->is_backend_simulation_running;
    backendSyncher->setData(_imgui->simulation_backend_running);
    backendSyncher->send();
    // BACKEND CONTROL
    {
        if (_imgui->get_update == 1)
        {
            _imgui->get_update = false;
            create_streamlines();
        }
        else if (_imgui->get_update == 2)
        {
            _imgui->get_update = false;
            create_temperature_field();
        }

        if (_imgui->connect_to_backend)
        {
            _imgui->connect_to_backend = false;
            if (controller->connect_to_server() == 0)
            {
                log_message("Connected to backend.");
            }
            else
            {
                log_message("Could not connect to collector.");
            }
        }
        if (_imgui->disconnect_from_backend)
        {
            _imgui->disconnect_from_backend = false;
            if (controller->disconnect_from_server() == 0)
            {
                log_message("Disconnected from backend.");
            }
            else
            {
                log_message("Could not disconnect from collector?!?!?!");
            }
        }
         if (_imgui->start_back_end_simulation)
        {
            _imgui->start_back_end_simulation = false;
            if (controller->start_backend_simulation() == 0)
            {
                log_message("Start backend signal is sent.");
            }
            else
            {
                log_message("Could not connect to collector.Start Backend is not send.");
            }
        }
        if (_imgui->restart)
        {
            _imgui->restart = false;
            if (controller->restart_simulation() == 0)
                log_message("Restart backend signal is sent.");
            else
            {
                log_message("Could not connect to collector.Restart Backend is not send.");
            }
        }
        if (_imgui->pause)
        {
            _imgui->pause = false;
            if (controller->pause_simulation() == 0)
                log_message("Pause backend signal is sent.");
            else
            {
                log_message("Could not connect to collector.Pause Backend is not send.");
            }
        }
        if (_imgui->cont)
        {
            _imgui->cont = false;

            if (controller->continue_simulation() == 0)
                log_message("Continue Simulation backend signal is sent.");
            else
            {
                log_message("Could not connect to collector.continue_simulation Backend is not send.");
            }
        }
        if (_imgui->send_scene)
        {
            _imgui->send_scene = false;
            send_scene_to_backend();
        }
        if (_imgui->checkpoint)
        {
            _imgui->checkpoint = false;
            if (controller->dump_checkpoint_simulation() == 0)
                log_message("Dump Checkpoint backend signal is sent.");
            else
            {
                log_message("Could not connect to collector.Dump Checkpoint Backend is not send.");
            }
        }
        if (_imgui->kill)
        {
            _imgui->kill = false;
            if (controller->kill_simulation() == 0)
                log_message("Kill backend signal is sent.");
            else
            {
                log_message("Could not connect to collector.Kill Backend is not send.");
            }
        }
    }
    // SLIDING WINDOW
    if (_imgui->_updateimgui) // todo
    {
        sw_master.show_sw = _imgui->_showSW;
        for (int i = 0; i < 6; i++)
            sw_master.sw_limits[i] = _imgui->sw_box[i];
        updateSlidingWindow();
    }
    // Camera
    if (_imgui->imgui_camera != CameraView::UNDEF)
    {
        std::cout << _imgui->imgui_camera << std::endl;
        this->updateCameraView(_imgui->imgui_camera);
        _imgui->imgui_camera = CameraView::UNDEF;
    }
    // GEOMETRY
    {
        /*############
        ADD GEOMETRY
        ############*/
        if (_imgui->add_geometry)
        {
            log_message(_imgui->geometry_add_name + " is loading...");
            _imgui->add_geometry = false;
            if (add_geometry(_imgui->geometry_add_name) == 0)
            {
                _imgui->geo_names.push_back(geometries[number_of_geometries - 1]->name);
                log_message(_imgui->geometry_add_name + " is loaded.");
            }
            else
            {
                log_message(_imgui->geometry_add_name + " could not be loaded.");
            }
        }
        /*############
        MODIFY GEOMETRY
        ############*/
        if (_imgui->modify_geo)
        {
            _imgui->modify_geo = false;
            geometries = _imgui->geometries;
        }
        /*############
        DELETE GEOMETRY
        ############*/
        if (_imgui->delete_geo)
        {
            _imgui->delete_geo = false;
            delete_geometry(_imgui->geometry_delete_num);
            log_message(_imgui->geometry_delete_name + "  is deleted.");
            _imgui->geometries = geometries;
        }
        /*############
        SET PRIMARY GEOMETRY
        ############*/
        if (_imgui->set_primary_geo_modify)
        {
            geometries[_imgui->set_primary_geometry_id]->is_primary_geometry = 1;
            for (int i = 0; i < geometries.size(); i++)
            {
                if (i == _imgui->set_primary_geometry_id)
                    geometries[_imgui->set_primary_geometry_id]->is_primary_geometry = 1;
                else
                    geometries[i]->is_primary_geometry = 0;
            }
            _imgui->set_primary_geo_modify = false;
        }
        /*############
        SET PRIMARY GEOMETRY
        ############*/
        if (_imgui->scale_to_the_bbox)
        {
            geometries[_imgui->scale_to_the_bbox_id]->is_scale_according_to_bbox = 1;
            _imgui->scale_to_the_bbox = false;
        }
        /*############
        DOMAIN BBOX
        ############*/
        if (_imgui->domain_bbox_changed)
        {
            _imgui->domain_bbox_changed = false;
            for (int i = 0; i < 3; i++)
            {
                gConfs.domain_bbox_min[i] = _imgui->domain_bbox_min[i];
                gConfs.domain_bbox_max[i] = _imgui->domain_bbox_max[i];
                gConfs.mesh_s[i] = _imgui->mesh_s[i];
                gConfs.mesh_r[i] = _imgui->mesh_r[i];
                gConfs.mesh_b[i] = _imgui->mesh_b[i];
            }
            gConfs.mesh_depth = _imgui->d;
            this->updateSlidingWindow();
            log_message(gConfs.printSelf());
        }
    }
    // WALL BOUNDARY_CONDITIONS
    {
        if (_imgui->wall_bc_updated)
        {
            Wall_BC_holder.clear();
            Wall_BC_holder.resize(_imgui->Wall_BC_holder.size());
            for (int i = 0; i < _imgui->Wall_BC_holder.size(); i++)
                Wall_BC_holder[i] = _imgui->Wall_BC_holder[i];
            _imgui->wall_bc_updated = false;
        }
    }
    // BOUNDARY_CONDITIONS
    {
        if (_imgui->bc_updated)
        {
            BC_holder.clear();
            BC_holder.resize(_imgui->BC_holder.size());
            for (int i = 0; i < _imgui->BC_holder.size(); i++)
                BC_holder[i] = _imgui->BC_holder[i];
            _imgui->bc_updated = false;
            show_bc = _imgui->show_bc;
            send_BC_to_frontend();
        }
    }
    // CONFIGURATIONS
    {
        /*############
        CONFIGURATIONS
        ############*/
        if (_imgui->load_configuration)
        {
            _imgui->load_configuration = false;
            load_configuration(_imgui->config_file);
            std::cout << _imgui->config_file << "is loaded as a config.\n";
        }
        if (_imgui->save_configuration)
        {
            _imgui->save_configuration = false;
            save_configuration(_imgui->config_save_name);
            std::cout << _imgui->config_file << "is saved as a config.\n";
        }
    }
    /* CHECK SLICES */
    if (_imgui->update_slice)
    {
        slices.clear();
        for (auto s : _imgui->slices)
            slices.push_back(s);
        send_slices_to_frontend();

        _imgui->update_slice = false;
        showSlices->setData(_imgui->show_slice);
        showSlices->send();
    }
}
void MasterApp::setup_imgui()
{
    for (int i = 0; i < 3; i++)
    {
        _imgui->domain_bbox_min[i] = gConfs.domain_bbox_min[i];
        _imgui->domain_bbox_max[i] = gConfs.domain_bbox_max[i];
        _imgui->mesh_s[i] = gConfs.mesh_s[i];
        _imgui->mesh_r[i] = gConfs.mesh_r[i];
        _imgui->mesh_b[i] = gConfs.mesh_b[i];
        sw_master.sw_limits[2 * i] = gConfs.domain_bbox_min[i];
        sw_master.sw_limits[2 * i + 1] = gConfs.domain_bbox_max[i];
        _imgui->sw_box[2 * i] = gConfs.domain_bbox_min[i];
        _imgui->sw_box[2 * i + 1] = gConfs.domain_bbox_max[i];
    }
    _imgui->d = gConfs.mesh_depth;

    // Geometries
    _imgui->number_of_geometries = number_of_geometries;
    _imgui->geometries.resize(geometries.size());
    for (int i = 0; i < geometries.size(); i++)
        _imgui->geometries[i] = geometries[i];
    // BC
    _imgui->BC_holder.resize(BC_holder.size());
    for (int i = 0; i < BC_holder.size(); i++)
        _imgui->BC_holder[i] = BC_holder[i];

    // Wall BC
    _imgui->Wall_BC_holder.clear();
    _imgui->Wall_BC_holder.resize(Wall_BC_holder.size());
    for (int i = 0; i < Wall_BC_holder.size(); i++)
        _imgui->Wall_BC_holder[i] = Wall_BC_holder[i];
}
void MasterApp::update_imgui(int x)
{
    switch (x)
    {
    case 0:
        // Geometries
        _imgui->number_of_geometries = number_of_geometries;
        _imgui->geometries.resize(geometries.size());
        for (int i = 0; i < geometries.size(); i++)
            _imgui->geometries[i] = geometries[i];
        break;
    case 1:
        _imgui->BC_holder.resize(BC_holder.size());
        for (int i = 0; i < BC_holder.size(); i++)
            _imgui->BC_holder[i] = BC_holder[i];
        break;
    case 2:
        _imgui->Wall_BC_holder.clear();
        _imgui->Wall_BC_holder.resize(Wall_BC_holder.size());
        for (int i = 0; i < Wall_BC_holder.size(); i++)
            _imgui->Wall_BC_holder[i] = Wall_BC_holder[i];
        break;
    }
}
void MasterApp::load_configuration(std::string configration_name)
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

    read_xml(paths.configuration + configration_name, pt);

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
}

void MasterApp::save_configuration(std::string configuration_filaname)
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
    write_xml(paths.configuration+configuration_filaname+".xml", pt);
    log_message(configuration_filaname+".xml is saved.");
    return; 
}

int MasterApp::add_geometry(std::string filename)
{
    try
    {
        geometries.push_back(new geometry_master(filename));
        number_of_geometries++;
        std::string dummy = get_serialized_data(geometries.back());
        update_imgui(0);
        return 0;
    }
    catch (const std::exception &e)
    {
        log_message(e.what());
        return 1;
    }
}
void MasterApp::delete_geometry(int geometry_delete_num)
{
    geometries.erase(geometries.begin() + geometry_delete_num);
    std::cout << geometry_delete_num << geometries[geometry_delete_num]->name << " will be deleted";
    std::cout << geometries.size() << " geometries left.";
    number_of_geometries--;
}

void MasterApp::send_scene_to_backend()
{
    if (!(controller->is_backend_simulation_running))
    {
        if (controller->send_initial_configuration(geometries, BC_holder, Wall_BC_holder, gConfs) == 0)
        {
            for (auto g : geometries)
                log_message(g->printSelf());
            for (auto bc : BC_holder)
                log_message(bc.printSelf());
            for (auto wbc : Wall_BC_holder)
                log_message(wbc.printSelf());
            log_message(gConfs.printSelf());
        }
        else
        {
            log_message("Initial configuration could not be sent");
        }
    }
    else
    {
        controller->register_and_send_frontend_geometries(geometries);
        controller->register_and_send_boundary_conditions(BC_holder);
    }
}

void MasterApp::create_streamlines()
{
    log_message("Rendering streamlines is started.");

    controller->set_sliding_window(sw_master.sw_limits);
    if (controller->update() != 0)
    {
        log_message("Connection problem might have occured. No streamline will be created.");
        return;
    }
    visdataSyncher->setData(0);
    visdataSyncher->send();
}
void MasterApp::create_temperature_field()
{
    log_message("Rendering Temperature field is started.");
    controller->set_sliding_window(sw_master.sw_limits);
    if (controller->update(0, 4) != 0)
    {
        log_message("Connection problem might have occured. No temperature field will be created.");
        return;
    }
    visdataSyncher->setData(1);
    visdataSyncher->send();
}

void MasterApp::send_BC_to_frontend()
{
    auto bc = bcSyncher->getData();
    bc.show_flag = show_bc;
    strcpy(bc.message, get_serialized_data(BC_holder).c_str());
    bcSyncher->setData(bc);
    bcSyncher->send();
}

void MasterApp::viewportEvent(ViewportEvent &event)
{
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _imgui->_imgui.relayout(Vector2{event.windowSize()} / event.dpiScaling(),
                            event.windowSize(), event.framebufferSize());
}
void MasterApp::send_slices_to_frontend()
{
    slice_syncher dummy;

    int c = 0; // counter
    dummy.number_of_slices = slices.size();
    for (auto ss : slices)
    {
        for (int i = 0; i < 3; i++)
        {
            dummy.origins[3 * c + i] = ss->origin[i];
            dummy.normals[3 * c + i] = ss->normal[i];
        }
        c++;
    }
    Debug{} << dummy.number_of_slices << " NUM SLICE";
    sliceSyncher->setData(dummy);
    sliceSyncher->send();
}
void MasterApp::process_wand()
{
    /*
    cursor_directions = [-x +x +y -y]
        0 -> Trigger
        1 -> Right
        2 -> Middle
        3 -> Left
     */
    bool changed = false;
    // analog
    glm::vec2 analogvalue = rServer->getAnalogValue();
    // Debug{} << analogvalue[0] << analogvalue[1];
    glm::ivec2 cursor_pos = analogsyncher->getData();
    float analog_tolerance = 0.3;

    if (analogvalue[0] > analog_tolerance)
    {
        changed = true;
        //Debug{}<<"True";
        //   cursor_directions[1] = true;
        virtual_cursor->setTranslation(Vector2(virtual_cursor->translation().x(), virtual_cursor->translation().y() + gui_cursor_speed_y));
    }
    if (analogvalue[0] < -analog_tolerance)
    {
        changed = true;
        // cursor_directions[0] = true;
        virtual_cursor->setTranslation(Vector2(virtual_cursor->translation().x(), virtual_cursor->translation().y() - gui_cursor_speed_y));
    }
    if (analogvalue[1] > analog_tolerance)
    {
        changed = true;
        //  cursor_directions[2] = true;
        virtual_cursor->setTranslation(Vector2(virtual_cursor->translation().x() - gui_cursor_speed_x, virtual_cursor->translation().y()));
    }
    if (analogvalue[1] < -analog_tolerance)
    {
        changed = true;
        // cursor_directions[3] = true;
        virtual_cursor->setTranslation(Vector2(virtual_cursor->translation().x() + gui_cursor_speed_x, virtual_cursor->translation().y()));
    }
    //Debug{} << cursor_directions[0] << cursor_directions[1] << cursor_directions[2] << cursor_directions[3];
    if (changed)
    {
        Vector2 Pos = virtual_cursor->absoluteTransformation().translation();
        cursor_pos[0] = Pos.y();
        cursor_pos[1] = Pos.x();
        analogsyncher->setData(cursor_pos);
        analogsyncher->send();
        changed = false;
    }

    // Buttons
    std::list<glm::ivec2> buttonQueue;
    rServer->getButtonQueue(buttonQueue);
    glm::bvec4 buttonPressed = buttonSyncher->getData();

    for (auto it = buttonQueue.begin(); it != buttonQueue.end(); ++it)
    {
        changed = true;
        buttonPressed[it->x] = static_cast<bool>(it->y);
        virtual_mouse_commands[it->x] = static_cast<bool>(it->y);
    }
    if (changed)
    {
        Debug{} << virtual_mouse_commands[0] << virtual_mouse_commands[1] << virtual_mouse_commands[2] << virtual_mouse_commands[3];
        buttonSyncher->setData(buttonPressed);
        buttonSyncher->send();
    }
}
void MasterApp::log_message(std::string log)
{
    auto LogMessage = logSyncher->getData();
    log.append("\n");
    std::string Owner = "[Server]:";
    strcpy(LogMessage.message, (Owner + log).c_str());
    logSyncher->setData(LogMessage);
    logSyncher->send();
    _imgui->log.AddLog(LogMessage.message);
}

} // namespace Magnum
MAGNUM_APPLICATION_MAIN(Magnum::MasterApp)