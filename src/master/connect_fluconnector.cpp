#include "../../include/connect_fluconnector.h"

// ---Constructer and connection to backend and update-------
control_backend::control_backend()
{
    this->swc = new SWConnector();
    this->vcg = new VisCGrid();
    data = vtkUnstructuredGrid::New();

    //pvcs = vtkSmartPointer<vtkPVConnectorSource>::New();
    port = 12346;
}

int control_backend::connect_to_server()
{
    int connected = -1;
    this->SWPort = 12345;
    for (int i = 0; i < 10; i++)
    {
        this->SWPort = this->SWPort + 1;
        this->SWServerName = "localhost";

        this->VisDataType = 0; // 0 - Velocity; 3- Pressure 4- Temperature 5-Rho 6-Vof
        this->VisMaxCells = 1000000;

        this->ToggleDataUpdate = false;

        this->CurrentTime = -1.0;

        double bbox[6] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        for (int i = 0; i < 6; i++)
            this->VisBBox[i] = bbox[i];

        //this->SetNumberOfInputPorts( 0 );
        this->Order = 0;

        //pvcs->SetVisBBox(0.3,0.3,0.0,0.9,1.0,1.0);
        connected = this->swc->connectToCollectorServer(this->SWServerName.c_str(), this->SWPort);
        if (connected == 0)
            break;
    }
    return connected;
}
int control_backend::disconnect_from_server()
{
    swc->disconnectNowFromCollectorServer();
    is_backend_simulation_running = false;
    return 0;
}
control_backend::~control_backend()
{
    // disconnect very properly from server
    swc->taskSendQuit();
    swc->disconnectFromCollectorServer();

    // delete the instances
    delete this->vcg;
    delete this->swc;
}

int control_backend::start_backend_simulation()
{
    // check if the socket state is ok for working with it
    if (!swc->socketStateOK(false))
    {
        // try to connect
        int rep = this->swc->connectToCollectorServer(this->SWServerName.c_str(), this->SWPort);
        if (rep != 0)
        {
            std::cout << "Could not connect to collector. Returned Error " << rep << "." << std::endl;
            return 1;
        }
    }

    int is_started = this->swc->taskStartBackendSimumation();
    if (is_started != 0)
    {
        std::cout << "Simulation could not be started\n";
        return 1;
    }
    std::cout << "Simulation is started ! ";
    return 0;
}
void control_backend::delete_backend_geometry(geometry_master *geometry)
{
    if (this->checkSocketState() != 0)
        return;

    int is_delete_sent = this->swc->taskDeleteGeometry(get_serialized_data(geometry));
    if (is_delete_sent != 0)
    {
        std::cout << "Delete geometry could not be send\n";
    }
    std::cout << "Delete geometry " << get_serialized_data(geometry) << std::endl;
}
void control_backend::add_backend_geometry(geometry_master *geometry)
{
    if (this->checkSocketState() != 0)
        return;

    int is_add_sent = this->swc->taskAddGeometry(get_serialized_data(geometry));
    if (is_add_sent != 0)
    {
        std::cout << "Add geometry could not be send\n";
    }
    std::cout << "Add geometry " << get_serialized_data(geometry) << std::endl;
}

void control_backend::delete_backend_boundary_condion(BC_struct bc)
{
    if (this->checkSocketState() != 0)
        return;

    int is_delete_sent = this->swc->taskDeleteBC(get_serialized_data(bc));
    if (is_delete_sent != 0)
    {
        std::cout << "Delete boundary_condion could not be send\n";
    }
    std::cout << "Delete boundary_condion " << get_serialized_data(bc) << std::endl;
}
void control_backend::add_backend_boundary_condion(BC_struct bc)
{
    if (this->checkSocketState() != 0)
        return;

    int is_add_sent = this->swc->taskAddBC(get_serialized_data(bc));
    if (is_add_sent != 0)
    {
        std::cout << "Add boundary_condion could not be send\n";
    }
    std::cout << "Add boundary_condion " << get_serialized_data(bc) << std::endl;
}

int control_backend::update(int order, unsigned short data_type)
{
    if (!(this->is_backend_simulation_running))
    {
        std::cout << "There's no running simulation to update ! " << std::endl;
        return 1;
    }
    // OPEN HERE !
    if (this->checkSocketState() != 0)
        return 1;

    this->VisDataType = data_type;

    this->Order = order;
    // send the request for the data to the server
    int ret = this->swc->taskReceiveSlidingWindowVisData(this->VisDataType, this->VisBBox, this->VisMaxCells, this->Order);
    printf("Viscells = %d, BBox (%f,%f,%f,%f,%f,%f)\n", this->VisMaxCells, this->VisBBox[0], this->VisBBox[1], this->VisBBox[2], this->VisBBox[3], this->VisBBox[4], this->VisBBox[5]);
    if (ret != 0)
    {
        std::
                cout
            << "taskReceiveSlidingWindowVisData() returned " << ret << " as error. This is not good!\n";
        return 1;
    }

    // add grids to be visualised
    std::list<CGrid *>::const_iterator git;
    for (git = swc->getCurrentCGridList().begin(); git != swc->getCurrentCGridList().end(); ++git)
    {
        const CGrid *cg = *git;
        vcg->addCGrid(cg);
    }
    vcg->finishedAddingCGrids();

    bool isVectorData = false;
    std::string name_to_use = "";

    // copy desired data to this class
    vcg->getVGridUpdate(data, &name_to_use, &isVectorData, &this->CurrentTime);

    // set the current data active
    if (isVectorData)
    {
        data->GetCellData()->SetActiveVectors(name_to_use.c_str());
    }
    else
    {
        data->GetCellData()->SetActiveScalars(name_to_use.c_str());
    }

    vtkSmartPointer<vtkUnstructuredGridWriter> gridWriter = vtkSmartPointer<vtkUnstructuredGridWriter>::New();
    gridWriter->SetInputData(data);
    gridWriter->SetFileName("Backend.vtk");
    gridWriter->SetFileTypeToBinary();
    gridWriter->Write();
    return 0;
}

// ---Constructer and connection to backend and update-------
// ---Sliding window and geometry limits ---------------------------------
void control_backend::set_sliding_window(float *sliding_window_bbox)
{
    // Frontend is designed according to VTK
    // Backend = [xmin ymin zmin xmax ymax zmax]
    // Frontend = [xmin xmax ymin ymax zmin zmax]
    VisBBox[0] = sliding_window_bbox[0];
    VisBBox[1] = sliding_window_bbox[2];
    VisBBox[2] = sliding_window_bbox[4];
    VisBBox[3] = sliding_window_bbox[1];
    VisBBox[4] = sliding_window_bbox[3];
    VisBBox[5] = sliding_window_bbox[5];
}
// ---Sliding window limits ---------------------------------

// ---Simulation Pause/Continue---------------------------------
int control_backend::pause_simulation()
{
    std::cout << "Simulation is paused" << std::endl;
    return this->update(SWCOL_PAUSE_SIM);
}
int control_backend::kill_simulation()
{
    std::cout << "Simulation is killed" << std::endl;
    is_backend_simulation_running = false;
    return this->swc->taskKillSimulation();
}
int control_backend::dump_checkpoint_simulation()
{
    std::cout << "Checkpoint is dumped" << std::endl;
    return this->swc->taskCheckpointDataDump();
}
int control_backend::continue_simulation()
{
    std::cout << "Simulation is continuingg" << std::endl;
    return this->update(0x93);
}
int control_backend::restart_simulation()
{
    std::cout << "Simulation is restarted" << std::endl;
    if (this->update(0x91) == 0)
    {
        is_backend_simulation_running = false;
        is_initial_geometry_sent = false;
        return 0;
    }
    else
    {
        return 1;
    }
}

// Simulation geometry send_INITIAL
void control_backend::send_geometry_information(std::string geo_serial_string)
{

    int task_sent = this->swc->taskSendGeometryInformation(geo_serial_string);
    if (task_sent != 0)
    {
        std::cout << "Geometry information could not be sent\n";
        return;
    }
    std::cout << "Geometry initial information sent ! \n";
    std::cout << geo_serial_string << std::endl;
    return;
}
int control_backend::send_initial_configuration(std::vector<geometry_master *> frontend_geometries, std::vector<BC_struct> boundaries, std::vector<BC_struct> wall_boundaries, general_configurations conf)
{
    // Send initial conditions
    int task_sent = this->swc->taskSendInitialConfiguration(get_serialized_data(frontend_geometries), get_serialized_data(boundaries), get_serialized_data(wall_boundaries), get_serialized_data(conf));
    if (task_sent != 0)
    {
        std::cout << "Initial configuration could not be sent\n";
        return 1;
    }
    // Register Geometries
    for (auto i : frontend_geometries)
        backend_geometries.emplace_back(new geometry_master(*i));
    // Register BC's
    for (auto i : boundaries)
        backend_boundaries.push_back(i);

    // TODO register wall boundaries !

    // Update situation
    is_backend_simulation_running = true;
    is_initial_geometry_sent = true;
    return 0;
}

int control_backend::register_and_send_frontend_geometries(std::vector<geometry_master *> frontend_geometries, std::string domain)
{

    if (this->checkSocketState() != 0)
        return 1;

    if (!(is_backend_simulation_running))
    {
        for (auto i : frontend_geometries)
            backend_geometries.emplace_back(new geometry_master(*i));
        // Send it
        send_geometry_information(get_serialized_data(frontend_geometries) + "|" + domain);
    }
    else
    {
        // new that will be added and deleted
        std::vector<geometry_master *> new_add, new_delete;

        //for(auto&& i:tmp) i->printSelf();
        std::vector<int> backend_ids, frontend_ids;
        for (auto j : backend_geometries)
            backend_ids.push_back(j->_id);
        for (auto j : frontend_geometries)
            frontend_ids.push_back(j->_id);

        // Geometries to add
        for (auto i : frontend_geometries)
        {
            if (std::find(backend_ids.begin(), backend_ids.end(), i->_id) != backend_ids.end())
            {
                continue;
            }
            else
            {
                std::cout << "new add to backend : " << i->_id << std::endl;
                new_add.push_back(i);
            }
        }
        // Geometries to delete
        for (auto i : backend_geometries)
        {
            if (std::find(frontend_ids.begin(), frontend_ids.end(), i->_id) != frontend_ids.end())
            {
                continue;
            }
            else
            {
                std::cout << "new delete to backend : " << i->_id << std::endl;
                new_delete.push_back(i);
            }
        }
        // Modified Geometries
        for (auto i : frontend_geometries)
        {
            for (auto j : backend_geometries)
            {

                if (*i != *j && i->_id == j->_id)
                {
                    std::cout << "modified geometry: " << j->_id << std::endl;
                    new_delete.push_back(j);
                    new_add.push_back(i);
                }
            }
        }

        // Here -> Send first Delete !
        for (auto i : new_delete)
            delete_backend_geometry(i);
        // Here -> Then send Add !
        for (auto i : new_add)
            add_backend_geometry(i);
        // Update local backend_geometries variable (basicly backend = frontend should be enough !)
        backend_geometries.clear();
        for (auto i : frontend_geometries)
            backend_geometries.emplace_back(new geometry_master(*i));
    }
    return 0;
}
void control_backend::register_and_send_boundary_conditions(std::vector<BC_struct> &boundaries)
{
    if (this->checkSocketState() != 0)
        return;

    if (!(is_backend_simulation_running))
    {
        for (auto i : boundaries)
            backend_boundaries.push_back(i);
        // Send it
        //send_geometry_information(this->get_serialized_data(frontend_geometries)+ "|"+domain);
    }
    else
    {
        std::vector<BC_struct> new_add;
        std::vector<BC_struct> new_delete;
        // Geometries to add
        for (auto i : boundaries)
        {
            if (std::find(backend_boundaries.begin(), backend_boundaries.end(), i) != backend_boundaries.end())
            {
                continue;
            }
            else
            {
                std::cout << "new add to backend : " << i.name << std::endl;
                new_add.push_back(i);
            }
        }
        // Geometries to delete
        for (auto i : backend_boundaries)
        {
            if (std::find(boundaries.begin(), boundaries.end(), i) != boundaries.end())
            {
                continue;
            }
            else
            {
                std::cout << "new delete boundary condition to backend : " << i.name << std::endl;
                new_delete.push_back(i);
            }
        }

        // Here -> Send first Delete !
        for (auto i : new_delete)
            delete_backend_boundary_condion(i);
        // Here -> Then send Add !
        for (auto i : new_add)
            add_backend_boundary_condion(i);
        // Update local backend_geometries variable (basicly backend = frontend should be enough !)

        backend_boundaries.clear();
        for (auto i : boundaries)
            backend_boundaries.push_back(i);
    }
}
int control_backend::checkSocketState()
{
    // check if the socket state is ok for working with it
    if (!swc->socketStateOK(false))
    {
        // try to connect
        int rep = this->swc->connectToCollectorServer(this->SWServerName.c_str(), this->SWPort);
        if (rep != 0)
        {
            std::cout << "Could not connect to collector. Returned Error " << rep << "." << std::endl;
            return 1;
        }
    }
    return 0;
}
// Debug
//for(auto&& i:frontend_geometries) i->printSelf();
//for(auto&& i:backend_geometries) i->printSelf();