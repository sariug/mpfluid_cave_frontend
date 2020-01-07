#ifndef connect_fluconnector_H
#define connect_fluconnector_H

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkCell.h>
#include <vtkLookupTable.h>
#include <vtkPolyData.h>
#include <vtkCellDataToPointData.h>
#include <vtkPlaneSource.h>
#include <vtkUnstructuredGridWriter.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkStreamTracer.h>
#include <vtkPointSource.h>

#include "basic_defs.h"
#include "../../kernel/fluid.h"

#ifdef VTK_DEBUG_VISUAL
#include <vtkActor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkStructuredGridOutlineFilter.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
#endif

// forward declaration
class SWConnector;
class VisCGrid;
class control_backend
{
public:
    control_backend();
    ~control_backend();

    int connect_to_server();
    int disconnect_from_server();
    int update(int order = 0, unsigned short data_type = 0);
    int checkSocketState();

    // Continue And resume
    int pause_simulation();
    int continue_simulation();
    int restart_simulation();
    int kill_simulation();
    int dump_checkpoint_simulation();
    int start_backend_simulation();
    void send_geometry_information(std::string geo_serial_string);
    int send_initial_configuration(std::vector<geometry_master *> frontend_geometries, std::vector<BC_struct> boundaries, std::vector<BC_struct> wall_boundaries, general_configurations conf);
    int register_and_send_frontend_geometries(std::vector<geometry_master *> frontend_geometries, std::string domain = "");
    void register_and_send_boundary_conditions(std::vector<BC_struct> &boundaries);
    void delete_backend_geometry(geometry_master *geometry);
    void add_backend_geometry(geometry_master *geometry);
    void delete_backend_boundary_condion(BC_struct bc);
    void add_backend_boundary_condion(BC_struct bc);

    // Setter
    void set_sliding_window(float *sliding_window_bbox);

    bool is_backend_simulation_running = false;
    bool is_initial_geometry_sent = false;

private:
    // The connector class for connecting to a server
    SWConnector *swc;

    // This is the converter class from CGrid to VGrid (i.e. unstructured VTK grid)
    VisCGrid *vcg;

    int Order; /* Ugurcan*/
    unsigned short SWPort;
    vtkStdString SWServerName;
    unsigned short VisDataType;
    float VisBBox[6];
    int VisMaxCells;
    bool ToggleDataUpdate;
    double CurrentTime;
    std::vector<geometry_master *> backend_geometries;
    std::vector<BC_struct> backend_boundaries;

    // Main Data Container
    vtkUnstructuredGrid *data;

    int port;
};

#endif
