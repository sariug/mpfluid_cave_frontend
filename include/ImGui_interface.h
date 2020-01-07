#ifndef IMGUI_GUI_H
#define IMGUI_GUI_H

#include "basic_defs.h"
#include "magnum_defs.h"

#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
// Magnum Imgui
#include <imgui.h>
#include <Magnum/ImGuiIntegration/Context.hpp>

const int X_PADDLE = 0;
const int Y_PADDLE = 0;
struct Log
{
    ImGuiTextBuffer Buf;
    ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls, allowing us to have a random access on lines
    bool AutoScroll;           // Keep scrolling if already at the bottom

    Log()
    {
        AutoScroll = true;
        Clear();
    }

    void Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    void AddLog(const char *fmt, ...) IM_FMTARGS(2)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void Draw(const char *title, bool *p_open = NULL)
    {
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char *buf = Buf.begin();
        const char *buf_end = Buf.end();

        ImGuiListClipper clipper;
        clipper.Begin(LineOffsets.Size);
        while (clipper.Step())
        {
            for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
            {
                const char *line_start = buf + LineOffsets[line_no];
                const char *line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                ImGui::TextUnformatted(line_start, line_end);
            }
        }
        clipper.End();
        ImGui::PopStyleVar();

        if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }
};
namespace Magnum
{
using namespace Math::Literals;
static const char *keys[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "q", "w", "e", "r", "t", "z", "u", "i", "o", "p", "a", "s", "d", "f", "g", "h", "j", "k", "l", "*", "y", "x", "c", "v", "b", "n", "m", ",", ".", "-", " "};

class ImGui_gui
{

public:
    Log log;

    ImGui_gui();
    void draw_interface(bool *wand_button_commands, int x = 0, int y = 0);

    folder_paths paths;
    bool update_node_gui = false;
    bool is_server = false;
    bool show_u = true;
    bool show_t = true;
    /*############
            BACKEND ORDERS
          ############*/
    bool simulation_backend_running = false;
    int get_update = false;
    bool connect_to_backend = false;
    bool disconnect_from_backend = false;
    bool start_back_end_simulation = false;
    bool restart = false;
    bool pause = false;
    bool cont = false;
    bool send_scene = false;
    bool checkpoint = false;
    bool kill = false;
    /*############
         SW SETTINGS
          ############*/
    bool _showSW = false;
    float sw_box[6] = {0, 0, 0, 0, 0, 0};
    bool _updateimgui = false;
    bool sw_immediate_update=false;
    /*
      CAMERA
       */
    CameraView imgui_camera = CameraView::UNDEF;
    /*############
         GEOMETRY SETTINGS
          ############*/
    int number_of_geometries = 0;
    std::vector<std::string> geo_names;

    // Highlight settings
    int highlight_geometry_id;
    bool highlight_geometry_modify = false;
    // Set primary geometry
    bool set_primary_geo_modify = false;
    int set_primary_geometry_id;
    // Set primary geometry
    bool scale_to_the_bbox = false;
    int scale_to_the_bbox_id;
    // Set General Configurations
    bool domain_bbox_changed = false;
    float domain_bbox_min[3] = {0.0, 0.0, 0.0};
    float domain_bbox_max[3] = {18.0, 25.0, 36.0};
    int mesh_s[3] = {2, 2, 2};
    int mesh_r[3] = {2, 2, 2};
    int mesh_b[3] = {8, 8, 8};
    int d = 3; // Mesh
    // Add Geo
    std::vector<geometry_master *> geometries;
    std::vector<std::string> geo_files;
    bool add_geometry = false;
    std::string geometry_add_name;
    // Geometry Modify
    bool ask_for_modify_data = false;
    int current_modify_geo = 0;
    float current_translate[3] = {0, 0, 0}; // for modification of geometry
    float current_rotate[3] = {0, 0, 0};    // for modfication of geometry
    bool modify_geo = false;
    float _temperature = 150;
    int _gidattribute = 150;
    onevectorfloat change;
    float _translate[3] = {0, 0, 0}; // for modification of geometry
    float _rotate[3] = {0, 0, 0};    // for modfication of geometry
    // Geometry -> Delete
    bool delete_geo = false;
    int geometry_delete_num;
    std::string geometry_delete_name;

    /*############
         BOUNDARY CONDITIONS
          ############*/
    std::vector<imgui_BC> BC_holder;
    bool bc_updated = false;
    bool show_bc = false;
    /*############
         WALL BOUNDARY CONDITIONS
          ############*/
    std::vector<imgui_BC> Wall_BC_holder;
    bool wall_bc_updated = false;

    /*############
         CONFIGURATIONS
          ############*/
    // save
    bool save_configuration = false;
    std::string config_save_name = "";
    // load
    bool load_configuration = false;
    std::string config_file = "";

    // Slices
    std::vector<Slice *> slices;
    bool update_slice = false;
    bool show_slice = true;
    std::vector<std::string> slice_names;
    ImGuiIntegration::Context _imgui{NoCreate};
    int current_bc_ = 0;

private:
    enum
    {
        add_bc_focus,
        modify_bc_focus,
        save_load_focus,
        slice_focus,
    };
    void draw_general_steering();
    void draw_sliding_window_controll();
    void draw_geometry_window();
    void draw_boundary_conditions();
    void draw_wall_boundary_conditions();
    void draw_save_load_config();
    void draw_general_configurations();
    void draw_change_view();
    void draw_slice_field();
    void draw_logger();
    void draw_basic_signals();
    void draw_keyboard();
    // Virtual Keyboard
    std::vector<bool> key_booleans;
    int num_of_keys = 42;
    char current_key;
    int focus = -1;
    bool _show_geo_add = false;
    bool _show_geo_modify = false;
    bool _show_geo_delete = false;
    bool _show_bc_add = false;
    bool _show_bc_modify = false;
    std::vector<std::string> bc_names;
    bool _apply_to_solids = false;
    Float bc_limits_min[3] = {0, 0, 0};
    Float bc_limits_max[3] = {1, 1, 1};
    const char *boundary_conditions = "INFLOW_TEMPNM\0OUTFLOW\0Solid\0\0";
};

} // namespace Magnum
#endif