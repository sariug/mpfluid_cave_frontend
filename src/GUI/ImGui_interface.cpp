#include "../../include/ImGui_interface.h"

/* 
    IMGUI FUNCTIONS ONLY DOWN !
*/
// Below three is the vector to string converter for combobox and listbox
void get_filenames_in_directory(std::vector<std::string> &geo_files, const char *folder)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(folder)) != NULL)
    {
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL)
        {
            if (ent->d_name[0] != '.')
                geo_files.push_back(ent->d_name);
        }
        closedir(dir);
    }
    else
    {
        std::cout << " get filenames function failed." << std::endl;
    }
}
static auto vector_getter = [](void *vec, int idx, const char **out_text) {
    auto &vector = *static_cast<std::vector<std::string> *>(vec);
    if (idx < 0 || idx >= static_cast<int>(vector.size()))
    {
        return false;
    }
    *out_text = vector.at(idx).c_str();
    return true;
};

bool Combo(const char *label, int *currIndex, std::vector<std::string> &values)
{
    if (values.empty())
    {
        return false;
    }
    return ImGui::Combo(label, currIndex, vector_getter,
                        static_cast<void *>(&values), values.size());
}

bool ListBox(const char *label, int *currIndex, std::vector<std::string> &values)
{
    if (values.empty())
    {
        return false;
    }
    return ImGui::ListBox(label, currIndex, vector_getter,
                          static_cast<void *>(&values), values.size(),5);
}
namespace Magnum
{

ImGui_gui::ImGui_gui()
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
        //Debug{}<<"ImGui Geometry folder : "<< paths.geometry;
        //Debug{}<<"ImGui Configuration folder : "<< paths.configuration;
    }
    else if (cwd_str.substr(cwd_str.length() - 3) == "bin")
    {
        paths.geometry = cwd_str.substr(0, cwd_str.length() - 3) + "geo_examples/";
        paths.configuration = cwd_str.substr(0, cwd_str.length() - 3) + "geometry_configurations/";
        // Debug{}<<"ImGui Geometry folder : "<< paths.geometry;
        // Debug{}<<"ImGui Configuration folder : "<< paths.configuration;
    }
    else
        throw "Whats up ?";

    // Start Virtual keys !
    key_booleans.resize(num_of_keys, false);
    current_key = 0;
}

void ImGui_gui::draw_interface(bool *wand_button_commands, int x, int y)
{

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
                                   GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
                                   GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    // Middle button is ok.
    ImGuiIO &io = ImGui::GetIO();
#ifdef CAVE
    io.MousePos = ImVec2(Vector2(x, y));
    wand_button_commands[2] ? io.MouseDown[0] = true : io.MouseDown[0] = false;
    ImGui::IsAnyWindowHovered();
#else
    if (!is_server)
    {
        io.MousePos = ImVec2(Vector2(x, y));
        wand_button_commands[2] ? io.MouseDown[0] = true : io.MouseDown[0] = false;
        ImGui::IsAnyWindowHovered();
    }
#endif
    _imgui.newFrame();
    draw_keyboard();
    draw_general_steering();
    draw_general_configurations();
    draw_basic_signals();
    draw_sliding_window_controll();
    draw_geometry_window();
    draw_save_load_config();
    draw_boundary_conditions();
    draw_wall_boundary_conditions();
    draw_change_view();
    draw_slice_field();
    draw_logger();
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    _imgui.drawFrame();
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
    // Reset the keys
    for (auto key : key_booleans)key = false;
        
}
void ImGui_gui::draw_general_steering()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(540, 55), ImVec2(540, 55));
    ImGui::SetNextWindowPos(ImVec2(5 + X_PADDLE, 85 + Y_PADDLE));
    ImGui::Begin("General Steering"); //, NULL, ImGuiWindowFlags_NoMove);
    {
        if (start_back_end_simulation == false)
        {
        if (ImGui::Button("Start and Send_scene")){send_scene = true;ImGui::SameLine();}}
        else if (ImGui::Button("Modify backend"))send_scene = true;ImGui::SameLine();
        if (ImGui::Button("Pause"))pause = true;ImGui::SameLine();
        if (ImGui::Button("Continue"))cont = true;ImGui::SameLine();
        if (ImGui::Button("Restart"))restart = true;ImGui::SameLine();
        if (ImGui::Button("Checkpoint"))checkpoint = true;ImGui::SameLine();
        if (ImGui::Button("Kill"))kill = true;
    }
    ImGui::End();
}
void ImGui_gui::draw_slice_field()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(240, 180), ImVec2(240, 180));
    ImGui::SetNextWindowPos(ImVec2(550 + X_PADDLE, 305 + Y_PADDLE));
    ImGui::Begin("Slice Field");
    static int slice_id = 0;
    static int current_slice = 0;
    static char slice_name[128] = "New Slice";

    if (ImGui::Button("Add"))
    {
        slices.push_back(new Slice());
        slices.back()->id = slice_id;
        slices.back()->name = "New Slice";

        slice_names.clear();
        for (int i = 0; i < slices.size(); i++)
            slice_names.push_back(slices[i]->name);
        current_slice = slices.size() - 1;
        slice_id++;
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete"))
    {
        if (slices.size() > 0)
        {
            slices.erase(slices.begin() + current_slice);
            update_slice = true;
            slice_names.clear();
            for (int i = 0; i < slices.size(); i++)
                slice_names.push_back(slices[i]->name);
            current_slice = slices.size() - 1;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Apply"))
    {
        if (slices.size() > 0)
        {
            slices[current_slice]->name = slice_name;

            update_slice = true;
            slice_names.clear();
            for (int i = 0; i < slices.size(); i++)
                slice_names.push_back(slices[i]->name);
        }
    }
    ImGui::SameLine();

    if (show_slice)
    {
        if (ImGui::Button("Hide"))
        {
            show_slice = false;
            update_slice = true;
        }
    }
    else
    {
        if (ImGui::Button("Show"))
        {
            show_slice = true;
            update_slice = true;
        }
    }

    Combo("#", &current_slice, slice_names);
    if (slices.size() > 0)
    {
        ImGui::InputText("Name", slice_name, IM_ARRAYSIZE(slice_name));
        if (ImGui::IsItemActivated())
        {
            focus = slice_focus;
        }
        if (focus == slice_focus)
        {
            for (int i = 0; i < 41; i++)
            {
                if (key_booleans[i])
                    strcat(slice_name, keys[i]);
            }
            if (key_booleans[41])
            {
                size_t len = strlen(slice_name);
                slice_name[len - 1] = '\0';
            }
        }
        float step = 0.1;
        ImGui::PushItemWidth(70);
        ImGui::InputScalar("##originx", ImGuiDataType_Float, &(slices[current_slice]->origin[0]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##originy", ImGuiDataType_Float, &(slices[current_slice]->origin[1]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##originz", ImGuiDataType_Float, &(slices[current_slice]->origin[2]), &step);
        ImGui::PopItemWidth();

        ImGui::PushItemWidth(70);
        ImGui::InputScalar("##normalx", ImGuiDataType_Float, &(slices[current_slice]->normal[0]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##normaly", ImGuiDataType_Float, &(slices[current_slice]->normal[1]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##normalz", ImGuiDataType_Float, &(slices[current_slice]->normal[2]), &step);
        ImGui::PopItemWidth();
    }
    ImGui::End();
}
void ImGui_gui::draw_change_view()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(240, 50), ImVec2(240, 50));
    ImGui::SetNextWindowPos(ImVec2(550 + X_PADDLE, 250 + Y_PADDLE));
    ImGui::Begin("Camera View"); //, NULL, ImGuiWindowFlags_NoMove);
    if (ImGui::Button("<->"))
    {
        imgui_camera = CameraView::RESET;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Reset View");
    ImGui::SameLine();
    if (ImGui::Button("XY"))
    {
        imgui_camera = CameraView::XY;
    }
    ImGui::SameLine();
    if (ImGui::Button("YX"))
    {
        imgui_camera = CameraView::YX;
    }
    ImGui::SameLine();
    if (ImGui::Button("XZ"))
    {
        imgui_camera = CameraView::XZ;
    }
    ImGui::SameLine();
    if (ImGui::Button("ZX"))
    {
        imgui_camera = CameraView::ZX;
    }
    ImGui::SameLine();
    if (ImGui::Button("YZ"))
    {
        imgui_camera = CameraView::YZ;
    }
    ImGui::SameLine();
    if (ImGui::Button("ZY"))
    {
        imgui_camera = CameraView::ZY;
    }
    ImGui::SameLine();
    ImGui::End();
}
void ImGui_gui::draw_sliding_window_controll()
{
    static int imgui_counter = 0;
    ImGui::SetNextWindowSizeConstraints(ImVec2(240, 245), ImVec2(240, 245));
    ImGui::SetNextWindowPos(ImVec2(550 + X_PADDLE, 1 + Y_PADDLE));
    ImGui::Begin("Sliding Window"); //, NULL, ImGuiWindowFlags_NoMove);
    ImGui::Text("Sliding Window Min-Max");
    ImGui::Text("x: %.3f , y: %.3f , z: %.3f", sw_box[0], sw_box[2], sw_box[4]);
    ImGui::Text("x: %.3f , y: %.3f , z: %.3f", sw_box[1], sw_box[3], sw_box[5]);
    ImGui::Separator();
    if (ImGui::Button("Show SW"))
    {
        if (_showSW)
        {
            _showSW = false; // Debug()<<"sw off";
            if (imgui_counter % 2 == 0)
            {
                _updateimgui = true;
                imgui_counter + 1;
            }
            else
            {
                _updateimgui = false;
            }
        }
        else
        {
            imgui_counter = (imgui_counter + 2) % 2;
            _showSW = true; // Debug()<<"sw on";
            _updateimgui = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset SW"))
    {
        for (int i = 0; i < 3; i++)
        {
            sw_box[2 * i] = domain_bbox_min[i];
            sw_box[2 * i + 1] = domain_bbox_max[i];
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Scale NOW"))
    {
            sw_immediate_update = !(sw_immediate_update);
    }
    
    const char *items[] = {"Xmin", "Xmax", "Ymin", "Ymax", "Zmin", "Zmax"};
    for (int i = 0; i < 3; i++)
    {
        ImGui::PushID(2 * i);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(i / 3.0f, 0.5f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(i / 3.0f, 0.6f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(i / 3.0f, 0.7f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(i / 3.0f, 0.9f, 0.9f));
        ImGui::SliderFloat(items[2 * i], &(sw_box[2 * i]), domain_bbox_min[i], sw_box[2 * i + 1]);
        ImGui::PopStyleColor(4);
        ImGui::PopID();
        // Mix
        ImGui::PushID(2 * i + 1);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(i / 3.0f, 0.5f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(i / 3.0f, 0.6f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(i / 3.0f, 0.7f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(i / 3.0f, 0.9f, 0.9f));
        ImGui::SliderFloat(items[2 * i + 1], &(sw_box[2 * i + 1]), sw_box[2 * i], domain_bbox_max[i]);
        ImGui::PopStyleColor(4);
        ImGui::PopID();
    }
    ImGui::End();
}
void ImGui_gui::draw_geometry_window()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 235), ImVec2(ImVec2(250, 235)));
    ImGui::SetNextWindowPos(ImVec2(295 + X_PADDLE, 145 + Y_PADDLE));
    ImGui::Begin("Geometries"); //, NULL, ImGuiWindowFlags_NoMove);
    ImGui::Text("Number of geometries : %d", number_of_geometries);
    ImGui::Text("Current Geometries:");
    geo_names.clear();
    for (int i = 0; i < geometries.size(); i++)
        geo_names.push_back(geometries[i]->name);
    static int current_geometry = 0;
    ListBox("", &current_geometry, geo_names);
    ImGui::BeginGroup();
    {
        if (ImGui::Button("Add"))
        {
            _show_geo_add = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Modify"))
        {
            if (number_of_geometries > 0)
                _show_geo_modify = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete"))
        {
            if (number_of_geometries > 0)
                _show_geo_delete = true;
            _show_geo_modify = false;
            _show_geo_add = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Highlight"))
        {
            highlight_geometry_id = current_geometry;
            highlight_geometry_modify = true;
        }
        if (ImGui::Button("Set primary geometry"))
        {
            set_primary_geo_modify = true;
            set_primary_geometry_id = current_geometry;
        }
        if (ImGui::Button("Set scale to the bbox"))
        {
            scale_to_the_bbox = true;
            scale_to_the_bbox_id = current_geometry;
        }
    }
    ImGui::EndGroup();

    // Geometry -> Add
    if (_show_geo_add)
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(250, 180), ImVec2(250, 180));
        ImGui::SetNextWindowPos(ImVec2(295 + X_PADDLE, 620 + Y_PADDLE));

        // ImGui::SetNextWindowSize(ImVec2(500, 100));
        ImGui::Begin("Add New Geometry", &_show_geo_add);
        geo_files.clear();

        get_filenames_in_directory(geo_files, paths.geometry.c_str());

        static int current = 0;
        ImGui::Text("Geometry files");
        ListBox("", &current, geo_files);
        if (ImGui::Button("Add"))
        {
            if (MAXIMUM_NUM_OF_GEO > number_of_geometries)
            {
                add_geometry = true;
                geometry_add_name = geo_files[current];
            }
            else
            {
                std::cout << " maximum geometry number ! \n";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Close"))
            _show_geo_add = false;
        ImGui::End();
    }
    // Geometry -> Modify
    if (_show_geo_modify)
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(395, 280), ImVec2(395,280));
        ImGui::SetNextWindowPos(ImVec2(795 + X_PADDLE, 50 + Y_PADDLE));

        //    ImGui::SetNextWindowSize(ImVec2(300, 100));
        ImGui::Begin("Modify Geometry", &_show_geo_modify);

        current_modify_geo = current_geometry;
        auto data = &(geometries[current_geometry]->position_modification);

        ImGui::Text("Current Translation[m]= x: %.3f , y: %.3f , z: %.3f", (*data)[0], (*data)[1], (*data)[2]);
        ImGui::Text("Current Rotation[deg] = x: %.3f , y: %.3f , z: %.3f", (*data)[3], (*data)[4], (*data)[5]);
        ImGui::Separator();
        float step_translate = 0.1f;
        float step_rotate = 2.0f;
        ImGui::Text("translate:[x,y,z] [m]");
        ImGui::PushItemWidth(86);
        ImGui::InputScalar("##_translatex", ImGuiDataType_Float, &(_translate[0]), &step_translate);
        ImGui::SameLine();
        ImGui::InputScalar("##_translatey", ImGuiDataType_Float, &(_translate[1]), &step_translate);
        ImGui::SameLine();
        ImGui::InputScalar("##_translatez", ImGuiDataType_Float, &(_translate[2]), &step_translate);
        ImGui::PopItemWidth();

        ImGui::Text("rotate:[x,y,z][deg]");
        ImGui::PushItemWidth(86);
        ImGui::InputScalar("##_rotatex", ImGuiDataType_Float, &(_rotate[0]), &step_rotate);
        ImGui::SameLine();
        ImGui::InputScalar("##_rotatey", ImGuiDataType_Float, &(_rotate[1]), &step_rotate);
        ImGui::SameLine();
        ImGui::InputScalar("##_rotatez", ImGuiDataType_Float, &(_rotate[2]), &step_rotate);
        ImGui::PopItemWidth();

        ImGui::Text("Attribute for GID");
        ImGui::InputInt("-", &(geometries[current_geometry]->gid_attribute));

        ImGui::Text("Temperature");
        ImGui::PushItemWidth(86);
        ImGui::InputScalar("K##geotempvelz", ImGuiDataType_Float, &(geometries[current_geometry]->temperature), &step_translate);
        ImGui::PopItemWidth();
        if (ImGui::Button("Apply"))
        {
            modify_geo = true;
            change.resize(6, 0.0);

            for (int i = 0; i < 3; i++)
            {
                (*data)[i] = (*data)[i] + _translate[i];
                change[i] = _translate[i];
                (*data)[i + 3] = (*data)[i + 3] + _rotate[i];
                change[i + 3] = _rotate[i];
            }

            // Reset translate and rotate
            for (int j = 0; j < 3; j++)
            {
                _translate[j] = .0f;
                _rotate[j] = .0f;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Close"))
        {
            _show_geo_modify = false;
        }
        ImGui::End();
    }
    // Geometry -> Delete
    if (_show_geo_delete)
    {
        ImGui::OpenPopup("Delete");
        if (ImGui::BeginPopupModal("Delete"))
        {
            ImGui::Text("Are you sure to delete **this** geometry!");
            if (ImGui::Button("Yes"))
            {
                _show_geo_delete = false;
                delete_geo = true;
                geometry_delete_name = geo_names[current_geometry];
                geometry_delete_num = current_geometry;
                geo_names.erase(geo_names.begin() + current_geometry);

                number_of_geometries--;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No"))
            {
                _show_geo_delete = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}
void ImGui_gui::draw_general_configurations()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(285, 235), ImVec2(285, 235));
    ImGui::SetNextWindowPos(ImVec2(5 + X_PADDLE, 145 + Y_PADDLE));
    ImGui::Begin("General Configurations"); //, NULL, ImGuiWindowFlags_NoMove);

    // if (ImGui::GetIO().MouseDown[0] && ImGui::IsWindowFocused())
    //log.AddLog("We are in General Cofs\n");
    ImGui::Text("Domain Bounding Box");
    {
        float step = 0.01;
        ImGui::PushItemWidth(78);
        ImGui::InputScalar("##Domainminx", ImGuiDataType_Float, &(domain_bbox_min[0]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##Domainminy", ImGuiDataType_Float, &(domain_bbox_min[1]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##Domainminz", ImGuiDataType_Float, &(domain_bbox_min[2]), &step);
        ImGui::InputScalar("##Domainmaxx", ImGuiDataType_Float, &(domain_bbox_max[0]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##Domainmaxy", ImGuiDataType_Float, &(domain_bbox_max[1]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##Domainmaxz", ImGuiDataType_Float, &(domain_bbox_max[2]), &step);
        ImGui::PopItemWidth();
    }
    ImGui::Separator();
    ImGui::Text("Mesh Parameters[X-Y-Z]");
    {
        ImGui::PushItemWidth(78);
        // mesh s
        ImGui::InputInt("##Sx", &mesh_s[0]);ImGui::SameLine();
        ImGui::InputInt("##Sy", &mesh_s[1]);ImGui::SameLine();
        ImGui::InputInt("##Sz", &mesh_s[2]);ImGui::SameLine();
        ImGui::Text("S");
        // mesh r
        ImGui::InputInt("##Rx", &mesh_r[0]);ImGui::SameLine();
        ImGui::InputInt("##Ry", &mesh_r[1]);ImGui::SameLine();
        ImGui::InputInt("##Rz", &mesh_r[2]);ImGui::SameLine();
        ImGui::Text("R");
        // mesh b
        ImGui::InputInt("##Bx", &mesh_b[0]);ImGui::SameLine();
        ImGui::InputInt("##By", &mesh_b[1]);ImGui::SameLine();
        ImGui::InputInt("##Bz", &mesh_b[2]);ImGui::SameLine();
        ImGui::Text("B");
        // mesh depth
        ImGui::InputInt("d", &d);
        ImGui::PopItemWidth();
        if (d < 1)d = 1;     
        if(mesh_s[0]<1)mesh_s[0]=1;
        if(mesh_s[1]<1)mesh_s[1]=1;
        if(mesh_s[2]<1)mesh_s[2]=1;
        if(mesh_r[0]<1)mesh_r[0]=1;
        if(mesh_r[1]<1)mesh_r[1]=1;
        if(mesh_r[2]<1)mesh_r[2]=1;
        if(mesh_b[0]<1)mesh_b[0]=1;
        if(mesh_b[1]<1)mesh_b[1]=1;
        if(mesh_b[2]<1)mesh_b[2]=1;
    }
    if (ImGui::Button("Apply"))
    {
        domain_bbox_changed = true;
    }
    ImGui::End();
}
void ImGui_gui::draw_boundary_conditions()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(285, 225), ImVec2(285, 225));
    ImGui::SetNextWindowPos(ImVec2(5 + X_PADDLE, 390 + Y_PADDLE));
    ImGui::Begin("Boundary Conditions"); //, NULL, ImGuiWindowFlags_NoMove);

    static int current_bc = 0;
    current_bc = current_bc_;
    ImGui::Text("Boundary Conditions");
    bc_names.clear();
    for (auto bcc : BC_holder)
        bc_names.push_back(bcc.name);
    ListBox("", &current_bc, bc_names);
    if (ImGui::Button("Add"))
    {
        _show_bc_add = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Modify"))
    {
        _show_bc_modify = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete"))
    {
        if (BC_holder.size() < 1)
            return;
        BC_holder.erase(BC_holder.begin() + current_bc);
        bc_updated = true;
        _show_bc_add = false;
        _show_bc_modify = false;
    }
    if (show_bc)
    {
        if (ImGui::Button("Hide"))
        {
            show_bc = false;
            bc_updated = true;
        }
    }
    else
    {
        if (ImGui::Button("Show"))
        {
            show_bc = true;
            bc_updated = true;
        }
    }
    if (_show_bc_add)
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(395, 225), ImVec2(395, 225));
        ImGui::SetNextWindowPos(ImVec2(795 + X_PADDLE, 330 + Y_PADDLE));
        ImGui::Begin("Add New BC", &_show_bc_add);
        static int bc_type = 0;
        static char bc_name[128] = "bc.name";
        ImGui::InputText("## BC name", bc_name, IM_ARRAYSIZE(bc_name));
        if (ImGui::IsItemActivated())
        {
            focus = add_bc_focus;
        }
        if (focus == add_bc_focus)
        {
            for (int i = 0; i < 41; i++)
            {
                if (key_booleans[i])
                    strcat(bc_name, keys[i]);
            }
            if (key_booleans[41])
            {
                size_t len = strlen(bc_name);
                bc_name[len - 1] = '\0';
            }
        }
        ImGui::Combo("BC type", &(bc_type), boundary_conditions);
        ImGui::Text("BC limits");
        float step = 0.01;
        ImGui::PushItemWidth(78);
        ImGui::InputScalar("##BCminx", ImGuiDataType_Float, &(bc_limits_min[0]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##BCminy", ImGuiDataType_Float, &(bc_limits_min[1]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##BCminz", ImGuiDataType_Float, &(bc_limits_min[2]), &step);
        ImGui::InputScalar("##BCmaxx", ImGuiDataType_Float, &(bc_limits_max[0]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##BCmaxy", ImGuiDataType_Float, &(bc_limits_max[1]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##BCmaxz", ImGuiDataType_Float, &(bc_limits_max[2]), &step);
        ImGui::PopItemWidth();

        ImGui::Checkbox("Apply to solids", &(_apply_to_solids));

        if (ImGui::Button("Add"))
        {
            imgui_BC bc;
            //bc.name=bc_name;
            strcpy(bc.name, bc_name);
            for (int a = 0; a < 3; a++)
            {
                bc.domain_min[a] = bc_limits_min[a];
                bc.domain_max[a] = bc_limits_max[a];
            }
            bc.bc_type = bc_type;
            bc._apply_to_solids = _apply_to_solids;
            BC_holder.push_back(bc);

            _show_bc_add = false;
            bc_updated = true;
        }
        ImGui::End();
    }
    if (_show_bc_modify && BC_holder.size()>0)
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(395, 225), ImVec2(395, 225));
        ImGui::SetNextWindowPos(ImVec2(795 + X_PADDLE, 570 + Y_PADDLE));
        ImGui::Begin("Modify BC", &_show_bc_modify);
        // Theres a bug here " !!!!"
        static char bc_name[128];
        float temp_step = 0.3;

        strcpy(bc_name, BC_holder[current_bc].name);
        ImGui::InputText("BC name", bc_name, IM_ARRAYSIZE(bc_name));
        if (ImGui::IsItemActivated())
        {
            focus = modify_bc_focus;
        }
        if (focus == modify_bc_focus)
        {
            for (int i = 0; i < 41; i++)
            {
                if (key_booleans[i])
                    strcat(bc_name, keys[i]);
            }
            if (key_booleans[41])
            {
                size_t len = strlen(bc_name);
                bc_name[len - 1] = '\0';
            }
        }
        strcpy(BC_holder[current_bc].name, bc_name);
        for (int a = 0; a < 3; a++)
        {
            bc_limits_min[a] = BC_holder[current_bc].domain_min[a];
            bc_limits_max[a] = BC_holder[current_bc].domain_max[a];
        }
        ImGui::Combo("BC type", &(BC_holder[current_bc].bc_type), boundary_conditions);
        ImGui::Text("BC limits");
        float step = 0.01;
        ImGui::PushItemWidth(78);
        ImGui::InputScalar("##_show_bc_modifyDomainminx", ImGuiDataType_Float, &(BC_holder[current_bc].domain_min[0]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##_show_bc_modifyDomainminy", ImGuiDataType_Float, &(BC_holder[current_bc].domain_min[1]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##_show_bc_modifyDomainminz", ImGuiDataType_Float, &(BC_holder[current_bc].domain_min[2]), &step);
        ImGui::InputScalar("##_show_bc_modifyDomainmaxx", ImGuiDataType_Float, &(BC_holder[current_bc].domain_max[0]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##_show_bc_modifyDomainmaxy", ImGuiDataType_Float, &(BC_holder[current_bc].domain_max[1]), &step);
        ImGui::SameLine();
        ImGui::InputScalar("##_show_bc_modifyDomainmaxz", ImGuiDataType_Float, &(BC_holder[current_bc].domain_max[2]), &step);
        ImGui::PopItemWidth();
        ImGui::Text("Temperature");
        ImGui::InputScalar("K##waltestmpvelz", ImGuiDataType_Float, &(BC_holder[current_bc].temperature), &temp_step);

        // _apply_to_solids = BC_holder[current_bc]._apply_to_solids;
        ImGui::Checkbox("Apply to solids", &(BC_holder[current_bc]._apply_to_solids));

        if (ImGui::Button("Apply"))
        {
            bc_updated = true;
            _show_bc_modify = false;
        }

        ImGui::End();
    }
    ImGui::End();
    current_bc_ = current_bc;
}
void ImGui_gui::draw_wall_boundary_conditions()
{

    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 225), ImVec2(ImVec2(250, 225)));
    ImGui::SetNextWindowPos(ImVec2(295 + X_PADDLE, 390 + Y_PADDLE));
    ImGui::Begin("Wall Boundary Conditions"); //, NULL, ImGuiWindowFlags_NoMove);

    const char *items[] = {"EAST", "WEST", "NORTH", "SOUTH", "TOP", "BOTTOM"};
    static const char *current_item = items[0];
    static int selected;

    if (ImGui::BeginCombo("##combo", current_item)) // The second parameter is the label previewed before opening the combo.
    {
        for (int n = 0; n < IM_ARRAYSIZE(items); n++)
        {
            bool is_selected = (current_item == items[n]); // You can store your selection however you want, outside or inside your objects
            if (ImGui::Selectable(items[n], is_selected))
            {
                current_item = items[n];
                selected = n;
            }
            ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
        }
        ImGui::EndCombo();
    }
    if (this->Wall_BC_holder.size() > 0)
    {
        float step = 0.01;
        ImGui::Combo("BC type", &(Wall_BC_holder[selected].bc_type), boundary_conditions);
        ImGui::Text("Velocity");
        ImGui::PushItemWidth(86);
        ImGui::InputScalar("m/s##walvelx", ImGuiDataType_Float, &(Wall_BC_holder[selected].U[0]), &step);
        ImGui::InputScalar("m/s##walvely", ImGuiDataType_Float, &(Wall_BC_holder[selected].U[1]), &step);
        ImGui::InputScalar("m/s##walvelz", ImGuiDataType_Float, &(Wall_BC_holder[selected].U[2]), &step);
        ImGui::PopItemWidth();
        ImGui::PushItemWidth(95);

        ImGui::Text("Temperature");
        ImGui::InputScalar("K##waltempvelz", ImGuiDataType_Float, &(Wall_BC_holder[selected].temperature), &step);
        ImGui::PopItemWidth();
    }
    if (ImGui::Button("Apply Wall Boundary Conditions"))
    {
        wall_bc_updated = true;
    }
    ImGui::End();
}
void ImGui_gui::draw_save_load_config()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(240, 125), ImVec2(240, 125));
    ImGui::SetNextWindowPos(ImVec2(550 + X_PADDLE, 490 + Y_PADDLE));
    ImGui::Begin("Save/Load Configurations"); //, NULL, ImGuiWindowFlags_NoMove);
    static char str0[128] = "filename";
    ImGui::InputText("Filename", str0, IM_ARRAYSIZE(str0));
    if (ImGui::IsItemActivated())
    {
        focus = save_load_focus;
    }
    if (focus == save_load_focus)
    {
        Debug{} << "focus buradaa";

        for (int i = 0; i < 41; i++)
        {
            if (key_booleans[i])
                strcat(str0, keys[i]);
        }
        if (key_booleans[41])
        {
            size_t len = strlen(str0);
            str0[len - 1] = '\0';
        }
    }
    std::vector<std::string> configuration_files;
    static int current_configuration = 0;
    get_filenames_in_directory(configuration_files, paths.configuration.c_str());
    Combo("File", &current_configuration, configuration_files);

    if (ImGui::Button("Save"))
    {
        save_configuration = true;
        config_save_name = str0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        load_configuration = true;
        config_file = configuration_files[current_configuration];
    }

    ImGui::End();
}

void ImGui_gui::draw_basic_signals()
{

    ImGui::SetNextWindowSizeConstraints(ImVec2(540, 80), ImVec2(540, 80));
    ImGui::SetNextWindowPos(ImVec2(5 + X_PADDLE, 1 + Y_PADDLE));
    ImGui::Begin("Basic Signals"); //, NULL, ImGuiWindowFlags_NoMove);
    if (simulation_backend_running)
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Backend Running");
    else
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Backend Not Running");

    ImGui::Separator();
    // Get simulation controls to top
    ImGui::BeginGroup();
    {

        if (ImGui::Button("CONNECT"))
        {
            connect_to_backend = true;
        }        
        ImGui::SameLine();
        if (ImGui::Button("DISCONNECT"))
        {
            disconnect_from_backend = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Update U"))
        {
            get_update = 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Update T"))
        {
            get_update = 2;
        }
        ImGui::SameLine();
        if(!show_u)
        {
            if (ImGui::Button("Show U"))
            {
                show_u = true;
            }
        }else{
            if (ImGui::Button("Hide U"))
            {
                show_u = false;
            }
        }
        ImGui::SameLine();
        if(!show_t)
        {
            if (ImGui::Button("Show T"))
            {
                show_t = true;                
            }
        }else{
            if (ImGui::Button("Hide T"))
            {
                show_t = false;                
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("About"))
        {
            log.AddLog("-------------------------------------------------------\n");
            log.AddLog("This work is a part of the masters thesis named\n");
            log.AddLog("\t'Interactive Exploration and Computational Steering \n ");
            log.AddLog("\tin  CAVE-like  Environments  for High-Performance\n");
            log.AddLog("\t Fluid Simulations'\n");
            log.AddLog("at the Chair of Computation in Engineering.It is \n");
            log.AddLog("prepared under the supervision of Dr.Ralf-Peter Mundani \n");
            log.AddLog("and Christoph Ertl M.Sc. \n");
            log.AddLog("For questions you can mail : ugurcansari93@gmail.com \n");
            log.AddLog("\t\t\t\t\t\t\t\t\tUgurcan Sari\n\t\t\t\t\t\t\t\t\tMunich/Germany\n\t\t\t\t\t\t\t\t\tNovember 2019\n");
            log.AddLog("-------------------------------------------------------\n");
        }
    }
    ImGui::EndGroup();
    ImGui::End();
}
void ImGui_gui::draw_logger()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(400, 800), ImVec2(400, 800));
    ImGui::SetNextWindowPos(ImVec2(795 + X_PADDLE, 1 + Y_PADDLE));
    ImGui::Begin("Log");
    ImGui::End();
    log.Draw("Log");
}
void ImGui_gui::draw_keyboard()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(290, 180), ImVec2(290, 180));
    ImGui::SetNextWindowPos(ImVec2(5 + X_PADDLE, 620 + Y_PADDLE));
    ImGui::Begin("Virtual Keyboard"); //, NULL, ImGuiWindowFlags_NoMove);

    // First Line
    current_key = 0;

    for (int n = 0; n < 40; n++)
    {
        if (n % 10 > 0)
            ImGui::SameLine();
        ImGui::PushID(n * 1000);
        float hue = n * 0.01f;
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue, 0.8f, 0.8f));
        if (ImGui::Button(keys[current_key], ImVec2(20.0f, 20.0f)))
        {
            key_booleans[current_key] = true;
        }
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        current_key++;
    }
    ImGui::PushID(41 * 1000);
    float hue = 41 * 0.01f;
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue, 0.8f, 0.8f));
    if (ImGui::Button("SPACE", ImVec2(270.0f, 20.0f)))
    {
        key_booleans[current_key] = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::PopID();

    current_key++;

    ImGui::PushID(42 * 1000);
    hue = 42 * 0.01f;
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue, 0.8f, 0.8f));
    if (ImGui::Button("BACKSPACE", ImVec2(270.0f, 20.0f)))
    {
        key_booleans[current_key] = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::PopID();

    ImGui::End();
    ImGui::SetKeyboardFocusHere(-1);
}
} // namespace Magnum
