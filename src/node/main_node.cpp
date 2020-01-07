#include "main_node.hpp"
MainNode::~MainNode()
{
    rNode->stopSynching();
}
MainNode::MainNode(const Arguments &arguments) : Platform::Application{
                                                     arguments,

#ifdef CAVE
                                                     Configuration{}.setTitle("MainNode").setCursorMode(Configuration::CursorMode::Disabled).setSize(Magnum::Vector2i(1920, 1200)),
                                                     GLConfiguration{}.setFlags(GLConfiguration::Flag::Stereo).setSampleCount(4)

#else
                                                     Configuration{}
                                                         .setTitle("MainNode")
                                                         .setWindowFlags(Configuration::WindowFlag::Resizable)
                                                         .setSize(Magnum::Vector2i(IMGUI_WINDOW_X, IMGUI_WINDOW_Y)),
                                                     GLConfiguration{}.setSampleCount(4)
#endif
                                                 },
                                                 _sliding_window(&_scene),
#ifdef CONSTRUCT_CUBEMAP
                                                 _cubemap_object(&_scene),
#endif
                                                 _slice_objects(&_scene), gui_object(&_scene), _static_objects(&_scene), _bc_objects(&_scene), _streamline_objects(&_scene), _temperature_objects(&_scene), _cave_geo(&_scene)

{
#ifdef CAVE
    Magnum::GL::Renderer::setLineWidth(10);
#else
    Magnum::GL::Renderer::setLineWidth(3);
#endif

    // More synchincg stuff
    rNode = std::make_shared<synchlib::renderNode>(arguments.argv[2], arguments.argc, arguments.argv);

    timeSyncher = synchlib::udpSynchObject<long long>::create();
    swsyncher = synchlib::udpSynchObject<sliding_window>::create();
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

    rNode->addSynchObject(timeSyncher, synchlib::renderNode::RECEIVER, 0);
    rNode->addSynchObject(swsyncher, synchlib::renderNode::RECEIVER, 0);
    rNode->addSynchObject(visdataSyncher, synchlib::renderNode::RECEIVER, 0);
    rNode->addSynchObject(configureSyncher, synchlib::renderNode::RECEIVER, 0);
    rNode->addSynchObject(bcSyncher, synchlib::renderNode::RECEIVER, 0);
    rNode->addSynchObject(cameraSyncher, synchlib::renderNode::RECEIVER, 0);
    rNode->addSynchObject(sliceSyncher, synchlib::renderNode::RECEIVER, 0);
    rNode->addSynchObject(showSlices, synchlib::renderNode::RECEIVER, 0);
    rNode->addSynchObject(analogsyncher, synchlib::renderNode::RECEIVER, 0);
    rNode->addSynchObject(buttonSyncher, synchlib::renderNode::RECEIVER, 0);
    rNode->addSynchObject(logSyncher, synchlib::renderNode::RECEIVER, 0);
    rNode->addSynchObject(backendSyncher, synchlib::renderNode::RECEIVER, 0);

    rNode->init();
    rNode->startSynching();
    // synchlib stuff end

    /* Every scene needs a camera */
    _cameraObject
        .setParent(&_scene);
    //

#ifdef CAVE
    change_camera_view(CameraView::XY);
#else
    _cameraObject
        .translate(Vector3::zAxis(200.0f))
        .translate(Vector3::xAxis(200.0f));
#endif
    (*(_camera = new SceneGraph::Camera3D{_cameraObject}))
#ifndef CAVE
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.001f, 10000.0f))
#endif
        //.setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 1.0f, 5000.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());

    // Set paths
    set_working_directories();
    // Geo and SW outline
    SLIDING_WINDOW = new Rectengle{cave_geo, sliding_window_color, *(new Object3D{&_sliding_window}), &_sliding_window_drawable};
    SLIDING_WINDOW->setLengths(_sliding_x, _sliding_y, _sliding_z);

#ifndef CAVE
    // Add Origin
    new Origin{_static_objects, &_drawables};

    // Add CAVE
    Color3 cave_color = 0xff00ff_rgbf;
    CAVE_GEOMETRY = new Rectengle{cave_geo, cave_color, *(new Object3D{&_cave_geo}), &_drawables};
#endif
#ifdef CONSTRUCT_CUBEMAP
    construct_cubemap();
#endif
    update_scaling_factors();

    // Start Processor is
    Processor = std::make_shared<vtkProcessor>();
    framebuffer.attachTexture(GL::Framebuffer::ColorAttachment{0}, color, 0);
    _imgui->_imgui = ImGuiIntegration::Context(Vector2{IMGUI_WINDOW_X, IMGUI_WINDOW_Y} / dpiScaling(),
                                               Vector2i{IMGUI_WINDOW_X, IMGUI_WINDOW_Y}, Vector2i{IMGUI_WINDOW_X, IMGUI_WINDOW_Y});

    /* CAVE GUI */
    _cameraObject_gui = new Object2D{&_scene_gui};
    _cameraObject_gui->rotate(90.0_degf);
    _camera_gui = new SceneGraph::Camera2D{*_cameraObject_gui};
    _camera_gui->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix3::projection({IMGUI_WINDOW_X, IMGUI_WINDOW_Y}))
        .setViewport(framebuffer.viewport().size());
    _cameraObject_gui->translate(Vector2(IMGUI_WINDOW_Y / 2, IMGUI_WINDOW_X / 2));
    virtual_cursor = new VirtualCursor{_scene_gui, _drawables_gui};
    virtual_cursor->setScaling(Vector2(10.0f, 10.0f));

    _plane = MeshTools::compile(Magnum::Primitives::planeSolid(Magnum::Primitives::PlaneTextureCoords::Generate));
    gui_plane = new Object3D{&gui_object};
    gui_plane->scale(Vector3(CAVE_DIMENSION / 4.0f, CAVE_DIMENSION / 4.0f, CAVE_DIMENSION / 4.0f));
#ifdef CAVE
    gui_plane->rotateXLocal(-70.0_degf);
    gui_plane->rotateZLocal(180.0_degf);
#endif
    gui_plane->translate(Vector3(CAVE_DIMENSION / 2.0f, 0.0f, CAVE_DIMENSION / 2.0f));

    new GUIPLANE(*gui_plane, Color4(1.0f, 0.0f, 0.0f, 0.0f), std::move(_plane), &_gui_drawables);
}

void MainNode::drawEvent()
{
    GL::Renderer::setClearColor(Color4(0.0f, 0.0f, 0.0f, 0.0f));

    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
    framebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add);

    update_synchers();
    process_wand();
    check_imgui();
    framebuffer.bind();
#ifndef CAVE
    gui_plane->translate(_cameraObject.transformation().translation() + Vector3(0, 0, -250) - gui_plane->transformation().translation());
#endif

    _imgui->draw_interface(virtual_mouse_commands, imgui_pos[0], imgui_pos[1]);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    _camera_gui->draw(_drawables_gui);
    render_gui();

    GL::defaultFramebuffer.bind();

    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add);

    if (show_scaled)
        this->scale_drawables_for_cave();
    auto timeNow = std::chrono::high_resolution_clock::now();
    long long dur = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - time_start).count();
    timeSyncher->setData(dur);
    timeSyncher->send();
    long long curTime = timeSyncher->getData();
    glm::mat4 projMatL, projMatR, viewMat;
    rNode->getProjectionMatrices(projMatL, projMatR);
    rNode->getSceneTrafo(viewMat);
    Matrix4 mMatL = glm2magnum(projMatL);
    Matrix4 mMatR = glm2magnum(projMatR);
    Matrix4 mview = glm2magnum(viewMat);

// Draw Left
#ifdef CAVE
    Magnum::GL::defaultFramebuffer.mapForDraw(Magnum::GL::DefaultFramebuffer::DrawAttachment::BackLeft);
    Magnum::GL::defaultFramebuffer.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth);
    _camera->setProjectionMatrix(mMatL * mview);
#endif

    _camera->draw(_drawables);
    if (show_streamlines)
        _camera->draw(_streamline_drawables);
    if (show_temperatures)
        _camera->draw(_temperature_drawables);
    if (_showSW)
        _camera->draw(_sliding_window_drawable);
    if (show_bc)
        _camera->draw(_bc_drawables);
    if (show_slices)
    {
        GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
        _camera->draw(_slices_drawable);
    }
    if (show_gui)
    {
        GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
        _camera->draw(_gui_drawables);
    }

    // Draw Right
#ifdef CAVE
#ifndef TAKE_PHOTO
    Magnum::GL::defaultFramebuffer.mapForDraw(Magnum::GL::DefaultFramebuffer::DrawAttachment::BackRight);
    Magnum::GL::defaultFramebuffer.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    _camera->setProjectionMatrix(mMatR * mview);

    _camera->draw(_drawables);
    if (show_streamlines)
        _camera->draw(_streamline_drawables);
    if (_showSW){
        _camera->draw(_sliding_window_drawable);}
    if (show_temperatures)
        _camera->draw(_temperature_drawables);
    if (show_bc)
        _camera->draw(_bc_drawables);
    if (show_slices)
    {
        GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
        _camera->draw(_slices_drawable);
    }
    if (show_gui)
    {
        GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
        _camera->draw(_gui_drawables);
    }
#endif
#endif
    // Scale them back !
    if (show_scaled)
        scale_drawables_for_cave();
    if (!rNode->synchFrame())
    {
        std::cout << "Could not sync frame" << std::endl;
        this->exit();
    }
    swapBuffers();
    redraw();
}
void MainNode::change_camera_view(CameraView X)
{
    switch (X)
    {
        //Debug{} << cameraSyncher->getData();
    case CameraView::XY:
        _cameraObject
            .resetTransformation()
            .translate(Vector3::xAxis(CAVE_DIMENSION / 2.0))
            .translate(Vector3::yAxis(CAVE_DIMENSION / 2.0))
            .rotateZLocal(180.0_degf)
            .rotateXLocal(90.0_degf);
        //(*gui_plane).resetTransformation().translate(Vector3(1, 1, 0)).scale(Vector3(CAVE_DIMENSION / 2, CAVE_DIMENSION / 2, CAVE_DIMENSION / 2));

        break;
    case CameraView::YX:
        _cameraObject
            .resetTransformation()
            .translate(Vector3::xAxis(CAVE_DIMENSION / 2.0))
            .translate(Vector3::yAxis(CAVE_DIMENSION / 2.0))
            .rotateXLocal(90.0_degf);
        //  (*gui_plane).resetTransformation().translate(Vector3(1, 1, 0)).scale(Vector3(CAVE_DIMENSION / 2, CAVE_DIMENSION / 2, CAVE_DIMENSION / 2));
        break;
    case CameraView::XZ:
        _cameraObject
            .resetTransformation()
            .translate(Vector3::xAxis(CAVE_DIMENSION / 2.0))
            .translate(Vector3::zAxis(CAVE_DIMENSION / 2.0));
        //  (*gui_plane).resetTransformation().rotateXLocal(-90.0_degf).translate(Vector3(1, 0, 1)).scale(Vector3(CAVE_DIMENSION / 2, CAVE_DIMENSION / 2, CAVE_DIMENSION / 2));

        break;
    case CameraView::ZX:
        _cameraObject
            .resetTransformation()
            .rotateYLocal(180.0_degf)
            .translate(Vector3::xAxis(CAVE_DIMENSION / 2.0))
            .translate(Vector3::zAxis(CAVE_DIMENSION / 2.0));
        //   (*gui_plane).resetTransformation().rotateXLocal(-90.0_degf).translate(Vector3(1, 0, 1)).scale(Vector3(CAVE_DIMENSION / 2, CAVE_DIMENSION / 2, CAVE_DIMENSION / 2));

        break;
    case CameraView::YZ:
        _cameraObject
            .resetTransformation()
            .rotateZLocal(90.0_degf)
            .translate(Vector3::yAxis(CAVE_DIMENSION) / 2.0f)
            .translate(Vector3::zAxis(CAVE_DIMENSION) / 2)
            .translate(Vector3::xAxis(CAVE_DIMENSION));
        //   (*gui_plane).resetTransformation().rotateYLocal(90.0_degf).translate(Vector3(0, 1, 1)).scale(Vector3(CAVE_DIMENSION / 2, CAVE_DIMENSION / 2, CAVE_DIMENSION / 2));

        break;
    case CameraView::ZY:
        _cameraObject
            .resetTransformation()
            .rotateZLocal(-90.0_degf)
            .translate(Vector3::yAxis(CAVE_DIMENSION) / 2)
            .translate(Vector3::zAxis(CAVE_DIMENSION) / 2);
        //   (*gui_plane).resetTransformation().rotateYLocal(90.0_degf).translate(Vector3(0, 1, 1)).scale(Vector3(CAVE_DIMENSION / 2, CAVE_DIMENSION / 2, CAVE_DIMENSION / 2));

        break;
    case CameraView::RESET:
        // A todo here. For now just XY
        _cameraObject
            .resetTransformation()
            .translate(Vector3::xAxis(CAVE_DIMENSION / 2.0))
            .translate(Vector3::yAxis(CAVE_DIMENSION / 2.0))
            .rotateZLocal(180.0_degf)
            .rotateXLocal(90.0_degf);
        break;
    default:
        break;
    }
}
void MainNode::viewportEvent(ViewportEvent &event)
{
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
    _camera->setViewport(event.windowSize());

    /* Reload fonts if pixel density changed */
    const Vector2 size = Vector2{event.windowSize()} / event.dpiScaling();
    const Float supersamplingRatio = event.framebufferSize().x() / size.x();
    _imgui->_imgui.relayout(Vector2{event.windowSize()} / event.dpiScaling(),
                            event.windowSize(), event.framebufferSize());
}
// Keyboard and mouse events

void MainNode::mouseScrollEvent(MouseScrollEvent &event)
{

    if (!event.offset().y())
        return;

    /* Distance to origin */
    const Float distance = _cameraObject.transformation().translation().z();
    /* Move 15% of the distance back or forward */
    _cameraObject.translate(distance * 0.15f *
                            (event.offset().y() > 0.0f ? -_cameraObject.transformation().backward() : _cameraObject.transformation().backward()));
    redraw();
}

void MainNode::mousePressEvent(MouseEvent &event)
{
    _previousMousePosition = _mousePressPosition = event.position();
}

void MainNode::mouseReleaseEvent(MouseEvent &event)
{

    if (event.button() == MouseEvent::Button::Left || event.button() == MouseEvent::Button::Middle)
        _previousPosition = Vector3();
}
void MainNode::mouseMoveEvent(MouseMoveEvent &event)
{

    if ((event.buttons() & MouseMoveEvent::Button::Left))
    {

        const Vector2 delta = 3.0f *
                              Vector2{event.position() - _previousMousePosition} /
                              Vector2{GL::defaultFramebuffer.viewport().size()};

        _cameraObject
            .rotate(Rad{-delta.y()}, _cameraObject.transformation().right().normalized())
            .rotateY(Rad{-delta.x()});

        _previousMousePosition = event.position();
        redraw();
    }
    else if ((event.buttons() & MouseMoveEvent::Button::Middle))
    {
        // todo
        return;
    }
    else
    {
        return;
    }
}
void MainNode::update_scaling_factors()
{
    SLIDING_WINDOW->setLengths(_sliding_x, _sliding_y, _sliding_z);
    Vector3 cave_cog = {CAVE_DIMENSION / 2, CAVE_DIMENSION / 2, CAVE_DIMENSION / 2};
    sw_cog = SLIDING_WINDOW->get_center_of_gravity();

    scaling_factorx = CAVE_DIMENSION / (_sliding_x);
    scaling_factory = CAVE_DIMENSION / (_sliding_y);
    scaling_factorz = CAVE_DIMENSION / (_sliding_z);
    Vector3 sw_mini_cog = {
        sw_cog[0] * scaling_factorx,
        sw_cog[1] * scaling_factory,
        sw_cog[2] * scaling_factorz};

    scaling_translate = cave_cog - sw_mini_cog; //cave_cog-SLIDING_WINDOW->get_center_of_gravity();//
}

void MainNode::scale_drawables_for_cave()
{
    auto cave_cog = CAVE_GEOMETRY->get_center_of_gravity();

    if (scale_up)
    {
        _sliding_window.scale(Vector3(scaling_factorx, scaling_factory, scaling_factorz));
        _sliding_window.translate(scaling_translate);
        _streamline_objects.scale(Vector3(scaling_factorx, scaling_factory, scaling_factorz));
        _streamline_objects.translate(scaling_translate);
        _temperature_objects.scale(Vector3(scaling_factorx, scaling_factory, scaling_factorz));
        _temperature_objects.translate(scaling_translate);
        _bc_objects.scale(Vector3(scaling_factorx, scaling_factory, scaling_factorz));
        _bc_objects.translate(scaling_translate);
        _slice_objects.scale(Vector3(scaling_factorx, scaling_factory, scaling_factorz));
        _slice_objects.translate(scaling_translate);
        //_static_objects
        _cubemap_object.scale(Vector3(scaling_factorx, scaling_factory, scaling_factorz));
        _cubemap_object.translate(scaling_translate);

        _static_objects.scale(Vector3(scaling_factorx, scaling_factory, scaling_factorz));
        _static_objects.translate(scaling_translate);
        scale_up = false;
    }
    else
    {
        _sliding_window.translate(-scaling_translate);
        _sliding_window.scale(Vector3(1.0f / scaling_factorx, 1.0f / scaling_factory, 1.0f / scaling_factorz));

        //sliding_window->translate(-scaling_translate);
        //SLIDING_WINDOW->scaleAndTranslate(1 / scaling_factorx, 1 / scaling_factory, 1 / scaling_factorz);
        //for (std::size_t i = 0; i < GEOMETRY_HOLDER.size(); i++)
        //{
        //    Geometries[i]->translate(-scaling_translate);
        //    Geometries[i]->scale(Vector3(1.0 / scaling_factorx, 1.0 / scaling_factory, 1.0 / scaling_factorz));
        //}
        _streamline_objects.translate(-scaling_translate);
        _streamline_objects.scale(Vector3(1.0f / scaling_factorx, 1.0f / scaling_factory, 1.0f / scaling_factorz));
        _temperature_objects.translate(-scaling_translate);
        _temperature_objects.scale(Vector3(1.0f / scaling_factorx, 1.0f / scaling_factory, 1.0f / scaling_factorz));
        _bc_objects.translate(-scaling_translate);
        _bc_objects.scale(Vector3(1.0f / scaling_factorx, 1.0f / scaling_factory, 1.0f / scaling_factorz));
        _slice_objects.translate(-scaling_translate);
        _slice_objects.scale(Vector3(1.0f / scaling_factorx, 1.0f / scaling_factory, 1.0f / scaling_factorz));

        _cubemap_object.translate(-scaling_translate);
        _cubemap_object.scale(Vector3(1.0f / scaling_factorx, 1.0f / scaling_factory, 1.0f / scaling_factorz));

        _static_objects.translate(-scaling_translate);
        _static_objects.scale(Vector3(1.0f / scaling_factorx, 1.0f / scaling_factory, 1.0f / scaling_factorz));

        scale_up = true;
    }
}

void MainNode::keyPressEvent(KeyEvent &event)
{

    if (event.key() == KeyEvent::Key::Up || event.key() == KeyEvent::Key::W)
    {

        const Float distance = _cameraObject.transformation().translation().z();
        /* Move 15% of the distance back or forward */
        _cameraObject.translate(distance * 0.15f *
                                (-_cameraObject.transformation().backward()));
    }
    else if (event.key() == KeyEvent::Key::Down || event.key() == KeyEvent::Key::S)
    {
        const Float distance = _cameraObject.transformation().translation().z();
        /* Move 15% of the distance back or forward */
        _cameraObject.translate(distance * 0.15f *
                                (_cameraObject.transformation().backward()));
    }
    else if (event.key() == KeyEvent::Key::Left || event.key() == KeyEvent::Key::A)
    {
        const Float distance = _cameraObject.transformation().translation().z();
        /* Move 15% of the distance back or forward */
        _cameraObject.translate(distance * 0.15f *
                                (-_cameraObject.transformation().right()));
    }
    else if (event.key() == KeyEvent::Key::Right || event.key() == KeyEvent::Key::D)
    {
        const Float distance = _cameraObject.transformation().translation().z();
        /* Move 15% of the distance back or forward */
        _cameraObject.translate(distance * 0.15f *
                                (_cameraObject.transformation().right()));
    }
    else if (event.key() == KeyEvent::Key::PageUp || event.key() == KeyEvent::Key::Y)
    {
        const Float distance = _cameraObject.transformation().translation().z();
        /* Move 15% of the distance back or forward */
        _cameraObject.translate(distance * 0.15f *
                                (_cameraObject.transformation().up()));
    }
    else if (event.key() == KeyEvent::Key::PageDown || event.key() == KeyEvent::Key::X)
    {

        const Float distance = _cameraObject.transformation().translation().z();
        /* Move 15% of the distance back or forward */
        _cameraObject.translate(distance * 0.15f *
                                (-_cameraObject.transformation().up()));
    }

    else if (event.key() == KeyEvent::Key::K)
    {
        if (show_scaled)
            show_scaled = false;
        else
        {
            show_scaled = true;
        }
    }
    else if (event.key() == KeyEvent::Key::P)
    {
        Magnum::DebugTools::screenshot(GL::defaultFramebuffer, "Screenshot.jpg");
    }

    else if (event.key() == KeyEvent::Key::Space)
    {
        virtual_mouse_commands[2] = true;
    }
    else if (event.key() == KeyEvent::Key::G)
    {
        show_gui = (!show_gui);
    }
    redraw();
}
//SLIDING_WINDOW->x_min = SLIDING_WINDOW->x_min +1.0f;
void MainNode::keyReleaseEvent(KeyEvent &event)
{
    if (event.key() == KeyEvent::Key::Space)
    {
        virtual_mouse_commands[2] = false;
    }
}

void MainNode::textInputEvent(TextInputEvent &event)
{
}

void MainNode::load_configuration()
{
    // Reset Current items such as Geometry HOLDER !
    for (std::size_t i = 0; i < number_of_geometries; i++)
    {
        auto obj = GEOMETRY_HOLDER[i];
        delete obj;
        if (Geometries[i])
            delete Geometries[i];
    }
    GEOMETRY_HOLDER.clear();
    number_of_geometries = 0;
    BC_holder.clear();
    Wall_BC_holder.clear();
    for (std::size_t i = 0; i < STREAMTRACERHOLDER.size(); i++)
        delete STREAMTRACERHOLDER[i];
    STREAMTRACERHOLDER.clear();
    delete PointHolder;
    // Getting Geometry
    auto conf_data = configureSyncher->getData();

    // turn vector into string
    std::string geo_string(conf_data.geometries);
    std::string bc_string(conf_data.bcs);
    std::string wall_bc_string(conf_data.wall_bcs);
    std::string conf_string(conf_data.general_configs);

    std::istringstream iss(geo_string);
    boost::archive::text_iarchive ia(iss);
    ia >> geometries;

    // Set configurations
    std::istringstream i1ss(conf_string);
    boost::archive::text_iarchive i1a(i1ss);
    i1a >> gConfs;
    gConfs.printSelf();

    // Set BC
    std::istringstream i2ss(bc_string);
    boost::archive::text_iarchive i2a(i2ss);
    i2a >> BC_holder;
    for (auto bc : BC_holder)
        bc.printSelf();
    Debug{} << BC_holder.size();

    // Set Wall BC
    std::istringstream i3ss(wall_bc_string);
    boost::archive::text_iarchive i3a(i3ss);
    i3a >> Wall_BC_holder;
    Debug{} << Wall_BC_holder.size();
    for (auto bc : Wall_BC_holder)
        bc.printSelf();

    for (auto g : geometries)
    {
        std::string geometry_file_path = paths.geometry + g->file_name;
        if (geometry_file_path.substr(geometry_file_path.length() - 3) == "stl")
        {
            Processor->make_eaf_out_of_stl(geometry_file_path);
        }
        EAF_LOADER eaf_importer(geometry_file_path.substr(0, geometry_file_path.length() - 3) + "eaf");

        if (eaf_importer.load_eaf() == 0)
        {

            Containers::Optional<Trade::MeshData3D> meshData = eaf_importer.get_mesh();
            auto vertexes = meshData->positions(0);

            float minx = 1e9, miny = 1e9, minz = 1e9, maxx = -1e9, maxy = -1e9, maxz = -1e9;
            for (std::size_t i = 0; i < vertexes.size(); i++)
            {
                if (vertexes[i][0] > maxx)
                    maxx = vertexes[i][0];
                if (vertexes[i][1] > maxy)
                    maxy = vertexes[i][1];
                if (vertexes[i][2] > maxz)
                    maxz = vertexes[i][2];
                if (vertexes[i][0] < minx)
                    minx = vertexes[i][0];
                if (vertexes[i][1] < miny)
                    miny = vertexes[i][1];
                if (vertexes[i][2] < minz)
                    minz = vertexes[i][2];
            }
            Float bounding_box[6] = {minx, maxx, miny, maxy, minz, maxz};
            auto obj = new Object3D(&_static_objects);
            GEOMETRY_HOLDER.push_back(new Imported_Geometry{g->file_name, obj,
                                                            bounding_box, MeshTools::compile(*meshData), _drawables}); // White color only                                                                                                                       // All below is to make the seen good. Back end unrelated.

            auto previous = GEOMETRY_HOLDER.back()->get_center_of_gravity();

            // Modify position
            obj->translate(-previous)
                .rotateX(Deg(-g->position_modification[3]))
                .rotateY(Deg(-g->position_modification[4]))
                .rotateZ(Deg(-g->position_modification[5]))
                .translate(previous)
                .translate(Vector3::xAxis(g->position_modification[0]))
                .translate(Vector3::yAxis(g->position_modification[1]))
                .translate(Vector3::zAxis(g->position_modification[2]));
            GEOMETRY_HOLDER.back()->set_position(g->position_modification);
            Geometries[number_of_geometries] = obj;
            number_of_geometries++;
            StaticCounter::geometry_number++;
        }
        else
        {
            throw std::runtime_error("Geometry could not loaded form file !");
        }
    }

    Float bbox[6] = {gConfs.domain_bbox_min[0], gConfs.domain_bbox_max[0],
                     gConfs.domain_bbox_min[1], gConfs.domain_bbox_max[1],
                     gConfs.domain_bbox_min[2], gConfs.domain_bbox_max[2]};
    SLIDING_WINDOW->update_sw_bbox(bbox);

#ifdef CONSTRUCT_CUBEMAP

    std::string cubemap(conf_data.cubemap);
    std::string cubefaces[6];
    std::string delimiter = " ";
    size_t pos = 0;
    std::string token;
    std::size_t i = 0;
    while ((pos = cubemap.find(delimiter)) != std::string::npos)
    {
        token = cubemap.substr(0, pos);
        cubefaces[i] = paths.assets + token; //+x
        std::cout << token << std::endl;
        cubemap.erase(0, pos + delimiter.length());
        i++;
    }
    cubefaces[5] = paths.assets + cubemap; //+x

    Debug{} << cubemap[0] << i;
    Debug{} << cubemap[1] << i;
    Debug{} << cubemap[2] << i;
    Debug{} << cubemap[3] << i;
    Debug{} << cubemap[4] << i;
    Debug{} << cubemap[5] << i;
    /* 
         paths.assets + std::string(walls[1]),  //-x
         paths.assets + std::string(walls[2]),  //+y
         paths.assets + std::string(walls[3]),  //-y
         paths.assets + std::string(walls[4]),  //+z,           
         paths.assets + std::string(walls[5])}; //-z
*/
    if (i < 5)
        construct_cubemap();
    else
        construct_cubemap(cubefaces);

#endif
    update_scaling_factors();
    setup_imgui();
    return;
}

void MainNode::set_working_directories()
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
    paths.CWD = cwd;
    if (paths.CWD.substr(paths.CWD.length() - 5) == "build")
    {
        paths.geometry = paths.CWD.substr(0, paths.CWD.length() - 5) + "src/scenograph/geometry_files/";
        paths.configuration = paths.CWD.substr(0, paths.CWD.length() - 5) + "src/scenograph/geometry_configurations/";
        paths.assets = paths.CWD.substr(0, paths.CWD.length() - 5) + "src/scenograph/geometry_configurations/assets/";
        std::cout << "Geometry folder : " << paths.geometry << std::endl;
        std::cout << "Configuration folder : " << paths.configuration << std::endl;
    }
    else if (paths.CWD.substr(paths.CWD.length() - 3) == "bin")
    {
        paths.geometry = paths.CWD.substr(0, paths.CWD.length() - 3) + "geo_examples/";
        paths.configuration = paths.CWD.substr(0, paths.CWD.length() - 3) + "geometry_configurations/";
        paths.assets = paths.CWD.substr(0, paths.CWD.length() - 3) + "geometry_configurations/assets";
    }
    else
        throw "In which folders  I am ?";
}

void MainNode::render_boundary_conditions()
{
    for (std::size_t i = 0; i < BC_HOLDER.size(); i++)
        delete BC_HOLDER[i];
    BC_HOLDER.clear();
    Color3 Geo_color = 0x0000ff_rgbf;

    std::vector<uint8_t> bcstring_vec;
    auto bc = bcSyncher->getData();
    std::string bc_string(bc.message);
    // Set BC
    std::istringstream i2ss(bc_string);
    boost::archive::text_iarchive i2a(i2ss);
    i2a >> BC_holder;
    show_bc = bc.show_flag;

    float bbox[6];
    for (auto bc : BC_holder)
    {
        for (std::size_t i = 0; i < 3; i++)
        {
            bbox[2 * i] = bc.domain_min[i];
            bbox[2 * i + 1] = bc.domain_max[i];
        }

        switch (bc.bc_type)
        {
        case 0: // Inlet
        {
            Geo_color = 0x33cccc_rgbf; // Turqusie
            break;
        }
        case 1: // Outlet
        {
            Geo_color = 0xff0000_rgbf; // Red
            break;
        }
        case 2: // Solid
        {
            Geo_color = 0x666633_rgbf; // Grey Green
            break;
        }
        default:
            printf("What type of bc is this ? \n");
            break;
        }
        BC_HOLDER.push_back(new Rectengle(bbox, Geo_color, _bc_objects, &_bc_drawables, false));
    }
    // Imgui :
    _imgui->BC_holder.resize(BC_holder.size());
    for (std::size_t i = 0; i < BC_holder.size(); i++)
        _imgui->BC_holder[i] = BC_holder[i];
}
void MainNode::construct_cubemap(std::string *cubemap)
{
    if (CUBEMAP)
    {
        delete CUBEMAP;
        _resourceManager.clear();
    }
    /* Load image importer plugin */
    Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("JpegImporter");
    if (!importer)
        std::exit(1);
    _resourceManager.set<Trade::AbstractImporter>("jpeg-importer",
                                                  importer.release(), ResourceDataState::Final, ResourcePolicy::Manual);

    if (!cubemap)
    {
        std::string cubefaces[6] = {paths.assets + "stars/left.JPG",  //+x  // LEFT CAVE
                                    paths.assets + "stars/right.JPG", //-x   // RIGHT CAVE
                                    paths.assets + "stars/back.JPG",  //+y   // Curtain Cave
                                    paths.assets + "stars/front.JPG", //-y       // Front cave
                                    paths.assets + "stars/up.JPG",    //"+y.jpg",           //+z
                                    paths.assets + "stars/down.JPG"}; //-z
        printf("Default Cubemap");

        /* Add objects to scene */
        _cubemap_object.resetTransformation();
        CUBEMAP = new Examples::CubeMap(_resourceManager, cubefaces, &_cubemap_object, &_drawables);
        _cubemap_object.translate(Vector3(1, 1, 1))
            .scale(Vector3(gConfs.domain_bbox_max[0] / 2, gConfs.domain_bbox_max[1] / 2, gConfs.domain_bbox_max[2] / 2));
        /* We don't need the importer anymore */
        _resourceManager.free<Trade::AbstractImporter>();
        return;
    }

    /* Add objects to scene */
    _cubemap_object.resetTransformation();
    CUBEMAP = new Examples::CubeMap(_resourceManager, cubemap, &_cubemap_object, &_drawables);
    _cubemap_object.translate(Vector3(1, 1, 1))
        .scale(Vector3(gConfs.domain_bbox_max[0] / 2, gConfs.domain_bbox_max[1] / 2, gConfs.domain_bbox_max[2] / 2));
    /* We don't need the importer anymore */
    _resourceManager.free<Trade::AbstractImporter>();
}

void MainNode::render_field_data()
{

    int vis_type = visdataSyncher->getData();
    if (vis_type == 0)
    {

        // If possible clean first the old timestep streamlines
        for (std::size_t i = 0; i < STREAMTRACERHOLDER.size(); i++)
            delete STREAMTRACERHOLDER[i];
        STREAMTRACERHOLDER.clear();

        // Render Streamlines
        std::cout << "Rendering streamlines are started." << std::endl;
        Processor->read_unstructered_vtk("Backend.vtk");
        Processor->set_seed_source(SLIDING_WINDOW->get_bbox(), 300);
        Processor->set_lookup_table(0);
        int number_of_streamlines = Processor->calculate_streamlines(*std::max_element(SLIDING_WINDOW->get_bbox(), SLIDING_WINDOW->get_bbox() + 6));
        std::vector<StreamPoint> streampoints;
        for (std::size_t i = 0; i < number_of_streamlines; i++)
        {
            streampoints.clear();
            Processor->get_streampoints(streampoints, i);
            STREAMTRACERHOLDER.push_back(new StreamTracer(streampoints, _streamline_objects, &_streamline_drawables));
        }
    }
    else if (vis_type == 1)
    {
        // If possible clean first the old timestep Temperatures
        delete PointHolder;

        std::cout << "Rendering the temperature field.\n";

        Processor->read_unstructered_vtk("Backend.vtk");
        Processor->set_lookup_table(1);
        std::vector<StreamPoint> TemperaturePoints;
        Processor->set_seed_source(SLIDING_WINDOW->get_bbox(), 1000000);
        Processor->get_coloured_points(SLIDING_WINDOW->get_bbox(), TemperaturePoints);
        PointHolder = new ScalarPoints(TemperaturePoints, _temperature_objects, &_temperature_drawables);
        Debug{} << "Rendered temperetarue balls = " << TEMPERATUREHOLDER.size();
        std::cout << "Rendering the Points field finished.\n";
    }
    else if (vis_type == -222)
    { // For temperature Balls !
        std::cout << "Rendering the temperature field.\n";
        // If possible clean first the old timestep Temperatures
        for (std::size_t i = 0; i < TEMPERATUREHOLDER.size(); i++)
            delete TEMPERATUREHOLDER[i];
        TEMPERATUREHOLDER.clear();

        Processor->read_unstructered_vtk("Backend.vtk");
        Processor->set_lookup_table(1);
        std::vector<StreamPoint> TemperaturePoints;
        Processor->get_temperature_points(TemperaturePoints, 100);
        for (auto &&tp : TemperaturePoints)
            TEMPERATUREHOLDER.push_back(new TemperatureSphere(tp.pos, tp.color, _temperature_objects, &_drawables));
        Debug{} << "Rendered temperetarue balls = " << TEMPERATUREHOLDER.size();
        std::cout << "Rendering the temperature field finished.\n";
    }
}

void MainNode::SliceField()
{

    for (std::size_t i = 0; i < SLICEHOLDER.size(); i++)
        delete SLICEHOLDER[i];
    SLICEHOLDER.clear();
    auto slices = sliceSyncher->getData();

    for (int b = 0; b < slices.number_of_slices; b++)
    {
        Slice slice;
        for (std::size_t i = 0; i < 3; i++)
        {
            slice.origin[i] = slices.origins[3 * b + i];
            slice.normal[i] = slices.normals[3 * b + i];
        }
        std::vector<StreamPoint> streampoints;
        std::vector<int> idxs;
        Processor->slice_data(streampoints, idxs, slice.origin, slice.normal);
        SLICEHOLDER.push_back(new SliceVR(streampoints, idxs, _slice_objects, &_slices_drawable));
    }
}

void MainNode::render_gui()
{
    /* Set texture data and parameters */
    /* Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    _texture.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::TextureFormat::RGB8, image->size())
        .setSubImage(0, {}, *image);*/

    // Plane !

    color.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::TextureFormat::RGB16, framebuffer.viewport().size());
    cave_gui->settTexture(color);
}
void MainNode::setup_imgui()
{
    _imgui->number_of_geometries = number_of_geometries;
    _imgui->geo_names.clear();
    for (std::size_t i = 0; i < GEOMETRY_HOLDER.size(); i++)
        _imgui->geo_names.push_back(GEOMETRY_HOLDER[i]->name);
    for (std::size_t i = 0; i < 3; i++)
    {
        _imgui->domain_bbox_min[i] = gConfs.domain_bbox_min[i];
        _imgui->domain_bbox_max[i] = gConfs.domain_bbox_max[i];
        _imgui->mesh_s[i] = gConfs.mesh_s[i];
        _imgui->mesh_r[i] = gConfs.mesh_r[i];
        _imgui->mesh_b[i] = gConfs.mesh_b[i];
        update_scaling_factors();
    }
    _imgui->d = gConfs.mesh_depth;
    // Geometries
    _imgui->number_of_geometries = number_of_geometries;
    _imgui->geometries.resize(geometries.size());
    for (std::size_t i = 0; i < geometries.size(); i++)
        _imgui->geometries[i] = geometries[i];
    // BC
    _imgui->BC_holder.resize(BC_holder.size());
    for (std::size_t i = 0; i < BC_holder.size(); i++)
        _imgui->BC_holder[i] = BC_holder[i];

    // Wall BC
    _imgui->Wall_BC_holder.clear();
    _imgui->Wall_BC_holder.resize(Wall_BC_holder.size());
    for (std::size_t i = 0; i < Wall_BC_holder.size(); i++)
        _imgui->Wall_BC_holder[i] = Wall_BC_holder[i];
}
void MainNode::update_imgui(int x)
{
    switch (x)
    {
    case 0:
        // Geometries
        _imgui->number_of_geometries = number_of_geometries;
        _imgui->geometries.resize(geometries.size());
        for (std::size_t i = 0; i < geometries.size(); i++)
            _imgui->geometries[i] = geometries[i];
        _imgui->log.AddLog("[Client]:Imgui Geometries updated.\n");

        break;
    case 1:
        _imgui->BC_holder.resize(BC_holder.size());
        for (std::size_t i = 0; i < BC_holder.size(); i++)
            _imgui->BC_holder[i] = BC_holder[i];
        _imgui->log.AddLog("[Client]:Imgui Boundary conditions updated.\n");

        break;
    case 2:
        _imgui->Wall_BC_holder.clear();
        _imgui->Wall_BC_holder.resize(Wall_BC_holder.size());
        for (std::size_t i = 0; i < Wall_BC_holder.size(); i++)
            _imgui->Wall_BC_holder[i] = Wall_BC_holder[i];
        _imgui->log.AddLog("[Client]:Imgui Wall Boundary conditions updated.\n");

        break;
    }
}
void MainNode::update_synchers()
{
    if (backendSyncher->hasChanged())
    {
        _imgui->simulation_backend_running = backendSyncher->getData();
    }
    if (logSyncher->hasChanged())
    {
        auto logMessage = logSyncher->getData();
        _imgui->log.AddLog(logMessage.message);
    }
    if (showSlices->hasChanged())
    {
        show_slices = showSlices->getData();
    }
    if (bcSyncher->hasChanged())
    {
        render_boundary_conditions();
    }
    if (sliceSyncher->hasChanged())
    {
        SliceField();
    }
    if (cameraSyncher->hasChanged())
    {
        change_camera_view(static_cast<CameraView>(cameraSyncher->getData()));
    }

    if (configureSyncher->hasChanged())
    {
        load_configuration();
    }

    if (visdataSyncher->hasChanged())
    {
        this->update_scaling_factors();
        this->render_field_data();
    }
    if (showSlices->hasChanged())
    {
        show_slices = showSlices->getData();
    }
    if (swsyncher->hasChanged())
    {

        auto swUpdate = swsyncher->getData();
        SLIDING_WINDOW->update_sw_bbox(swUpdate.sw_limits);
        _showSW = swUpdate.show_sw;
        for (std::size_t i = 0; i < 6; i++)
            _imgui->sw_box[i] = SLIDING_WINDOW->get_bbox()[i];
        if (_imgui->sw_immediate_update)
            this->update_scaling_factors();
    }
}
void MainNode::check_imgui()
{
    if (_imgui->modify_geo)
    {
        auto a = _imgui->change;
        Debug{} << GEOMETRY_HOLDER[_imgui->current_modify_geo]->name << " change: " << a << " mod_geo num = " << _imgui->current_modify_geo;
        geometries[_imgui->current_modify_geo]->printSelf();
        auto new_pos = geometries[_imgui->current_modify_geo]->position_modification;
        auto current_pos = GEOMETRY_HOLDER[_imgui->current_modify_geo]->get_position();
        auto cog = GEOMETRY_HOLDER[_imgui->current_modify_geo]->get_center_of_gravity();
        Debug{} << "current pos :" << current_pos;
        Debug{} << "new pos :" << new_pos;
        // Zerofy:
        Geometries[_imgui->current_modify_geo]->translate(Vector3::xAxis(-current_pos[0])).translate(Vector3::yAxis(-current_pos[1])).translate(Vector3::zAxis(-current_pos[2])).translate(-cog).rotateX(Deg(current_pos[3])).rotateY(Deg(current_pos[4])).rotateZ(Deg(current_pos[5])).translate(cog)
            // To new pos
            .translate(-cog)
            .rotateX(Deg(-new_pos[3]))
            .rotateY(Deg(-new_pos[4]))
            .rotateZ(Deg(-new_pos[5]))
            .translate(cog)
            .translate(Vector3::xAxis(new_pos[0]))
            .translate(Vector3::yAxis(new_pos[1]))
            .translate(Vector3::zAxis(new_pos[2]));
        GEOMETRY_HOLDER[_imgui->current_modify_geo]->set_position(new_pos);

        // Geometries[_imgui->current_modify_geo]->translate(Vector3::xAxis(a[0]))
        // .translate(Vector3::yAxis(a[1]))
        // .translate(Vector3::zAxis(a[2]));

        // auto previous = GEOMETRY_HOLDER[_imgui->current_modify_geo]->get_center_of_gravity();
        // Geometries[_imgui->current_modify_geo]->translate(-previous)
        // .rotateX(Deg(-a[3]))
        // .rotateY(Deg(-a[4]))
        // .rotateZ(Deg(-a[5]))
        // .translate(previous);
        _imgui->modify_geo = false;
    }
    if (_imgui->add_geometry)
    {
        load_geometry(_imgui->geometry_add_name);
        _imgui->add_geometry = false;
    }
    if (_imgui->delete_geo)
    {
        _imgui->delete_geo = false;
        auto obj = GEOMETRY_HOLDER[_imgui->geometry_delete_num];
        GEOMETRY_HOLDER.erase(GEOMETRY_HOLDER.begin() + _imgui->geometry_delete_num);
        delete obj;
        number_of_geometries--;
        log_message(_imgui->geometry_delete_name + "  is deleted.");
        geometries.erase(geometries.begin() + _imgui->geometry_delete_num);
        for (auto gg : geometries)
            gg->printSelf();
        for (auto gg : GEOMETRY_HOLDER)
            Debug{} << gg->name;
        _imgui->geometries = geometries;
    }
    if (_imgui->highlight_geometry_modify)
    {
        _imgui->highlight_geometry_modify = false;
        GEOMETRY_HOLDER[_imgui->highlight_geometry_id]->modify_selected();
    }
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
    }
    show_streamlines = _imgui->show_u;
    show_temperatures = _imgui->show_t;
}
void MainNode::load_geometry(std::string filename)
{
    std::string filepath = paths.geometry + filename;

    // Getting Geometry
    if (filepath.substr(filepath.length() - 3) == "stl")
    {
        Processor->make_eaf_out_of_stl(filepath);
    }
    EAF_LOADER eaf_importer(filepath.substr(0, filepath.length() - 3) + "eaf");

    if (eaf_importer.load_eaf() == 0)
    {

        Containers::Optional<Trade::MeshData3D> meshData = eaf_importer.get_mesh();
        auto vertexes = meshData->positions(0);

        float minx = 1e9, miny = 1e9, minz = 1e9, maxx = -1e9, maxy = -1e9, maxz = -1e9;
        for (std::size_t i = 0; i < vertexes.size(); i++)
        {
            if (vertexes[i][0] > maxx)
                maxx = vertexes[i][0];
            if (vertexes[i][1] > maxy)
                maxy = vertexes[i][1];
            if (vertexes[i][2] > maxz)
                maxz = vertexes[i][2];
            if (vertexes[i][0] < minx)
                minx = vertexes[i][0];
            if (vertexes[i][1] < miny)
                miny = vertexes[i][1];
            if (vertexes[i][2] < minz)
                minz = vertexes[i][2];
        }

        Float bounding_box[6] = {minx, miny, minz, maxx, maxy, maxz};

        Geometries[number_of_geometries] = new Object3D(&_static_objects);
        GEOMETRY_HOLDER.push_back(new Imported_Geometry{filename, Geometries[number_of_geometries], bounding_box, MeshTools::compile(*meshData), _drawables}); // White color only
        number_of_geometries++;
        geometries.push_back(new geometry_master(filename));
    }
    else
    {
        std::cout << "File could not be opened. Continueing without it.\n";
    }
    update_imgui(0);
}
void MainNode::move_geometry(int id, onevectorfloat &change) // for
{
    Debug{} << change;
    GEOMETRY_HOLDER[id]->set_position(change); // To save info about how much change gonna happen in backend
    // All below is to make the seen good. Back end unrelated.
    // Translate

    Debug{} << "geometry holde id " << GEOMETRY_HOLDER[id]->_id << " " << GEOMETRY_HOLDER.size();

    Geometries[id]->translate(Vector3{change[0], change[1], change[2]});
    Debug{} << "translate compelete";
    // Then rotate. It find center of gravity. Then rotates. Then translated it back to the location of where
    // Center of gravity was in the first place.
    Vector3 center = GEOMETRY_HOLDER[id]->get_center_of_gravity();
    // Rotate X
    if (change[3] != 0)
    {
        Geometries[id]->translate(-center);
        Geometries[id]->rotateX(Deg(-change[3]));
        Geometries[id]->translate(center);
    }
    if (change[4] != 0)
    {
        // Rotate Y
        Geometries[id]->translate(-center);
        Geometries[id]->rotateY(Deg(-change[4]));
        Geometries[id]->translate(center);
    }
    // Rotate Z
    if (change[5] != 0)
    {
        Geometries[id]->translate(-center);
        Geometries[id]->rotateZ(Deg(-change[5]));
        Geometries[id]->translate(center);
    }
}
void MainNode::process_wand()
{ /*
    cursor_directions = [-x +x +y -y]
        0 -> Trigger
        1 -> Right
        2 -> Middle
        3 -> Left
     */

    if (analogsyncher->hasChanged())
    {

        glm::ivec2 cursor_pos = analogsyncher->getData();
        imgui_pos = cursor_pos;

        //VIEWPORT_X * (CAVE_DIMENSION - Pos.x()) / CAVE_DIMENSION, VIEWPORT_Y * (CAVE_DIMENSION - Pos.z()) / CAVE_DIMENSION);
        // TODO !
        virtual_cursor->setTranslation(Vector2(cursor_pos[1], cursor_pos[0]));
    }
    if (buttonSyncher->hasChanged())
    {
        glm::bvec4 buttonPressed = buttonSyncher->getData();
        for (std::size_t i = 0; i < 4; i++)
            virtual_mouse_commands[i] = buttonPressed[i];
        if (buttonPressed[0])
            show_gui = (!show_gui);
        if (buttonPressed[1])
            gui_plane->translate(Vector3(0, 2.0f, 0.0f));
        if (buttonPressed[3])
            gui_plane->translate(Vector3(0, -2.0f, 0.0f));
    }
}
void MainNode::log_message(std::string log)
{
    log.append("\n");
    std::string Owner = "[CaveNode]:";
    _imgui->log.AddLog((Owner + log).c_str());
}
} // namespace Magnum

MAGNUM_APPLICATION_MAIN(Magnum::MainNode)
