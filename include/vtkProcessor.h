#ifndef vtkProcessor_H
#define vtkProcessor_H

#include "magnum_defs.h"
#include "basic_defs.h"

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
#include <vtkProbeFilter.h>
#include <vtkStreamTracer.h>
#include <vtkPlaneSource.h>
#include <vtkCutter.h>
#include <vtkDoubleArray.h>
#include <vtkPlane.h>
#include <vtkSTLReader.h>
#include <vtkPointSource.h>
#include <vtkNamedColors.h>
#include <vtkDataSetMapper.h>
#include <vtkAppendFilter.h>
#include <vtkExtractUnstructuredGrid.h>

#include "../../pv_plugin/eafdata/vtkEAFWriter.h"
//#define DebugVtkProcessor
#ifdef VTK_DEBUG_VISUAL
#include <vtkActor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkStructuredGridOutlineFilter.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
#endif

typedef struct
{
    double o[3];
    double n[3];
} cutplane;

class vtkProcessor
{
public:
    vtkProcessor() { std::cout << "vtkPRocessor created.\n"; }
    ~vtkProcessor() {}

    // IO
    int read_unstructered_vtk(std::string filename)
    {
        try
        {
            vtkSmartPointer<vtkUnstructuredGridReader> gridReader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
            gridReader->SetFileName(filename.c_str());
            gridReader->Update();
            data = gridReader->GetOutput();
        }
        catch (std::exception e)
        {
            std::cout << e.what();
            return 1;
        }
        return 0;
    }
    int write_unstructered_vtk(std::string filename)
    {
        try
        {
            vtkSmartPointer<vtkUnstructuredGridWriter> gridWriter = vtkSmartPointer<vtkUnstructuredGridWriter>::New();
            gridWriter->SetInputData(data);
            gridWriter->SetFileName(filename.c_str());
            gridWriter->SetFileTypeToBinary();
            gridWriter->Write();
        }
        catch (std::exception e)
        {
            std::cout << e.what();
            return 1;
        }
        return 0;
    }

    void set_seed_source(float *bbox, int numberOfPoints = 100, int type = 0)
    {
        // High Resolution Point Source
        seeds = vtkSmartPointer<vtkPointSource>::New();
        // Sliding windows centers
        float sw_x = (bbox[0] + bbox[1]) / 2.0;
        float sw_y = (bbox[2] + bbox[3]) / 2.0;
        float sw_z = (bbox[4] + bbox[5]) / 2.0;
        // Sw diagonal
        //float dia = std::max({(bbox[1] - bbox[0]), (bbox[3] - bbox[2]), (bbox[5] - bbox[4])});
        float dia = pow((pow((bbox[1] - bbox[0]),2)+ pow((bbox[3] - bbox[2]),2)+pow((bbox[5] - bbox[4]),2)),0.5);

        seeds->SetCenter(sw_x, sw_y, sw_z);
        seeds->SetRadius(dia / 2.0);
        seeds->SetNumberOfPoints(numberOfPoints);

#ifdef DebugVtkProcessor
        seeds->Print(std::cout);
#endif
    }
    void set_lookup_table(int vis_type)
    {
        if (data == nullptr)
        {
            std::cout << "No grid exist in VTK data" << std::endl;
            return;
        }
        // Set look-up table
        // ** I believe this is cheap, therefore no need to save it**
        lu = vtkSmartPointer<vtkLookupTable>::New();

        lu->SetNumberOfTableValues(256);
        //lu->SetVectorModeToRGBColors();
        //lu->SetVectorModeToMagnitude();
        lu->SetVectorMode(vtkScalarsToColors::MAGNITUDE);

        lu->SetHueRange(0.6667, 0.0);
        switch (vis_type)
        {
        case 0:
            lu->SetTableRange(0, (data->GetCellData()->GetArray("velocity_mag")->GetMaxNorm())*0.9);
            break;
        case 1:
            lu->SetTableRange(295, data->GetCellData()->GetArray("temperature")->GetMaxNorm());
            break;
        default:
            std::cout << "What is this visualization type ? " << vis_type << std::endl;
            break;
        }
        //lu->SetTableRange(0, 1);
        lu->Build();
    }
    int calculate_streamlines(double max_length)
    {
        active_data_type = 0;
        /*
        Return : Number of Streamlines
        */

        streamer = vtkStreamTracer::New();
        std::cout << "    Calculation of  streamlines are started." << std::endl;
        // Interpolate cell values to point values
        vtkSmartPointer<vtkCellDataToPointData> c2p = vtkSmartPointer<vtkCellDataToPointData>::New();
        c2p->SetInputData(data);
        c2p->Update();
        // Calculate streamline

        streamer->SetInputConnection(c2p->GetOutputPort());
        streamer->SetSourceConnection(seeds->GetOutputPort());
        streamer->SetMaximumPropagation(max_length);
        streamer->SetInitialIntegrationStep(0.2);
        streamer->SetMinimumIntegrationStep(.01);
        streamer->SetMaximumIntegrationStep(.5);
        streamer->SetMaximumNumberOfSteps(2000);
        streamer->SetInterpolatorTypeToCellLocator();

        streamer->SetIntegrationDirectionToBoth();
        streamer->SetIntegratorTypeToRungeKutta45();
        //streamer->SetComputeVorticity(true);

        streamer->Update();
#ifdef DebugVtkProcessor
        std::cout << "Number of Streamlines : " << streamer->GetOutput()->GetNumberOfCells() << std::endl;
        streamer->Print(std::cout);
#endif
#ifdef VTK_DEBUG_VISUAL
        auto streamLine = streamer;

        vtkSmartPointer<vtkPolyDataMapper> streamLineMapper =
            vtkSmartPointer<vtkPolyDataMapper>::New();
        streamLineMapper->SetInputConnection(streamLine->GetOutputPort());

        vtkSmartPointer<vtkActor> streamLineActor =
            vtkSmartPointer<vtkActor>::New();
        streamLineActor->SetMapper(streamLineMapper);
        streamLineActor->VisibilityOn();

        // Outline-Filter for the grid
        vtkSmartPointer<vtkStructuredGridOutlineFilter> outline =
            vtkSmartPointer<vtkStructuredGridOutlineFilter>::New();

        outline->SetInputData(streamLine->GetOutput());

        vtkSmartPointer<vtkPolyDataMapper> outlineMapper =
            vtkSmartPointer<vtkPolyDataMapper>::New();
        outlineMapper->SetInputConnection(outline->GetOutputPort());

        vtkSmartPointer<vtkActor> outlineActor =
            vtkSmartPointer<vtkActor>::New();
        outlineActor->SetMapper(outlineMapper);
        outlineActor->GetProperty()->SetColor(1, 1, 1);

        // Create the RenderWindow, Renderer and Actors
        vtkSmartPointer<vtkRenderer> renderer =
            vtkSmartPointer<vtkRenderer>::New();
        vtkSmartPointer<vtkRenderWindow> renderWindow =
            vtkSmartPointer<vtkRenderWindow>::New();
        renderWindow->AddRenderer(renderer);

        vtkSmartPointer<vtkRenderWindowInteractor> interactor =
            vtkSmartPointer<vtkRenderWindowInteractor>::New();
        interactor->SetRenderWindow(renderWindow);

        vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
            vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
        interactor->SetInteractorStyle(style);

        renderer->AddActor(streamLineActor);
        renderer->AddActor(outlineActor);

        // Add the actors to the renderer, set the background and size
        renderer->SetBackground(0.1, 0.2, 0.4);
        renderWindow->SetSize(300, 300);
        interactor->Initialize();
        renderWindow->Render();

        interactor->Start();
#endif
        return streamer->GetOutput()->GetNumberOfCells();
    }
    void get_streampoints(std::vector<StreamPoint> &SP, int i) // i here is the streamline number
    {
        // Apply
        vtkSmartPointer<vtkIdList> cellPointIds = vtkSmartPointer<vtkIdList>::New();
        double vel_x, vel_y, vel_z, vel_mag;
        double rgba[4];
        int number_of_points = streamer->GetOutput()->GetCell(i)->GetNumberOfPoints();
        SP.resize(number_of_points, StreamPoint());
        // XYZ Position
        auto cell = streamer->GetOutput()->GetCell(i);
        for (int j = 0; j < number_of_points; j++)
        {

            for (int k = 0; k < 3; k++)
            {
                SP[j].pos[k] = cell->GetPoints()->GetPoint(j)[k]; // Points xyz
            }
        }

        // Colors
        streamer->GetOutput()->GetCellPoints(i, cellPointIds); // Streampoints
        assert(number_of_points == cellPointIds->GetNumberOfIds());
#ifdef DebugVtkProcessor
        std::cout << "For streamline ; Num of points are  equal to num of colors : " << number_of_points << std::endl;
#endif
        for (int j = 0; j < cellPointIds->GetNumberOfIds(); j++)
        {
            vel_x = streamer->GetOutput()->GetPointData()->GetArray(4)->GetTuple3(cellPointIds->GetId(j))[0];
            vel_y = streamer->GetOutput()->GetPointData()->GetArray(4)->GetTuple3(cellPointIds->GetId(j))[1];
            vel_z = streamer->GetOutput()->GetPointData()->GetArray(4)->GetTuple3(cellPointIds->GetId(j))[2];
            vel_mag = sqrt(vel_x * vel_x + vel_y * vel_y + vel_z * vel_z);
            lu->GetColor(vel_mag, rgba);
            for (int k = 0; k < 3; k++)
            {
                SP[j].color[k] = rgba[k]; // color xyz
            }
            SP[j].color[3] = 1.0f;
        }
    }

    void get_temperature_points(std::vector<StreamPoint> &TemperaturePoints, int skip_number = 6)
    {
        active_data_type = 1;

        // In order to increase the number of data we have.
        vtkSmartPointer<vtkCellDataToPointData> c2p = vtkSmartPointer<vtkCellDataToPointData>::New();
        c2p->SetInputData(data);
        c2p->Update();

        auto dataset = c2p->GetOutput();
        // Apply
        double rgba[4];

        for (int i = 0; i < dataset->GetNumberOfPoints(); i = i + skip_number)
        {
            StreamPoint tp;
            // XYZ
            tp.pos[0] = dataset->GetPoint(i)[0];
            tp.pos[1] = dataset->GetPoint(i)[1];
            tp.pos[2] = dataset->GetPoint(i)[2];

            lu->GetColor(dataset->GetPointData()->GetArray("temperature")->GetTuple1(i), rgba);

            // Color
            tp.color[0] = rgba[0];
            tp.color[1] = rgba[1];
            tp.color[2] = rgba[2];
            tp.color[3] = 1.0f;
            TemperaturePoints.push_back(tp);
        }
        std::cout << "Data extraction for Temperature balls is finished." << std::endl;
        std::cout << "Number of temperature balls : " << TemperaturePoints.size() << std::endl;
    }

    void get_coloured_points(float *bbox, std::vector<StreamPoint> &Points)
    {
     active_data_type = 1;
        // In order to increase the number of data we have.
        vtkSmartPointer<vtkCellDataToPointData> c2p = vtkSmartPointer<vtkCellDataToPointData>::New();
        c2p->SetInputData(data);
        c2p->Update();

        std::cout << c2p->GetOutput()->GetNumberOfPoints() << std::endl;

        vtkSmartPointer<vtkProbeFilter> probFilter = vtkSmartPointer<vtkProbeFilter>::New();
        probFilter->SetSourceConnection(c2p->GetOutputPort());
        probFilter->SetInputConnection(this->seeds->GetOutputPort());
        probFilter->Update();

        auto dataset = probFilter->GetOutput();
        // Apply
        double rgba[4];
        for (int i = 0; i < dataset->GetNumberOfPoints(); i++)
        {
            if (!((bbox[0] <= dataset->GetPoint(i)[0] &&
                  bbox[1] >= dataset->GetPoint(i)[0] &&
                  bbox[2] <= dataset->GetPoint(i)[1] &&
                  bbox[3] >= dataset->GetPoint(i)[1] &&
                  bbox[4] <= dataset->GetPoint(i)[2] &&
                  bbox[5] >= dataset->GetPoint(i)[2])))
                  continue;
                StreamPoint tp;

            // XYZ
            tp.pos[0] = dataset->GetPoint(i)[0];
            tp.pos[1] = dataset->GetPoint(i)[1];
            tp.pos[2] = dataset->GetPoint(i)[2];

            lu->GetColor(dataset->GetPointData()->GetArray("temperature")->GetTuple1(i), rgba);

            // Color
            tp.color[0] = rgba[0];
            tp.color[1] = rgba[1];
            tp.color[2] = rgba[2];
            tp.color[3] = .8f;
            Points.push_back(tp);
        }
        std::cout << "Data extraction for Temperature balls is finished." << std::endl;
        std::cout << "Number of temperature balls : " << Points.size() << std::endl;
    }

    void make_eaf_out_of_stl(std::string filepath)
    {
        std::cout << filepath << " is converted to eaf" << std::endl;
        vtkSmartPointer<vtkSTLReader> reader =
            vtkSmartPointer<vtkSTLReader>::New();
        reader->SetFileName(filepath.c_str());
        reader->Update();
        vtkSmartPointer<vtkEAFWriter> gridWriter = vtkSmartPointer<vtkEAFWriter>::New();
        gridWriter->SetInputConnection(reader->GetOutputPort());
        gridWriter->SetFileName((filepath.substr(0, filepath.length() - 3) + "eaf").c_str());
        //gridWriter->SetFileTypeToBinary();
        gridWriter->Write();
        std::cout << filepath << " is converted to eaf" << std::endl;
    }

    void slice_data(std::vector<StreamPoint> &SP, std::vector<int> &ID_ARRAY, float *origin, float *normal)
    {
        double scalar =0.0;
        double rgba[4];
        vtkSmartPointer<vtkCellDataToPointData> pointData = vtkSmartPointer<vtkCellDataToPointData>::New();
        pointData->SetInputData(data);
        pointData->Update();
        // creating vtk append filter
        vtkSmartPointer<vtkAppendFilter> appendFilter = vtkSmartPointer<vtkAppendFilter>::New();

        cutplane a;
        vtkSmartPointer<vtkIdList> cellPointIds = vtkSmartPointer<vtkIdList>::New();
        this->set_lookup_table(active_data_type);
        // creating cutplanes
        vtkNew<vtkCutter> cut;
        vtkNew<vtkPlane> plane;
        plane->SetOrigin(origin[0], origin[1], origin[2]);
        plane->SetNormal(normal[0], normal[1], normal[2]);
        cut->SetInputData(pointData->GetOutput());
        cut->SetCutFunction(plane.GetPointer());
        cut->Update();
        // add an array identifying the slice (for later thresholding)
        vtkIdType number_of_points = cut->GetOutput()->GetNumberOfPoints();

        auto output = cut->GetOutput();
        SP.resize(number_of_points, StreamPoint());
        for (std::size_t  j = 0; j < number_of_points; j++)
        {

            for (int k = 0; k < 3; k++)
            {
                SP[j].pos[k] = output->GetPoints()->GetPoint(j)[k]; // Points xyz
            }
        }

        for (std::size_t i=0; i < output->GetNumberOfPolys(); i++)
        {
            output->GetCellPoints(i, cellPointIds);
            for (std::size_t j = 0; j < cellPointIds->GetNumberOfIds(); j++)
            {
                if (active_data_type == 0)
                    scalar = output->GetPointData()->GetArray(5)->GetTuple1(cellPointIds->GetId(j));
                else if (active_data_type == 1)
                    scalar = output->GetPointData()->GetArray(4)->GetTuple1(cellPointIds->GetId(j));
                lu->GetColor(scalar, rgba);
                for (std::size_t k = 0; k < 3; k++)           
                    SP[cellPointIds->GetId(j)].color[k] = rgba[k]; // color xyz

                SP[cellPointIds->GetId(j)].color[3] = 0.5f;
                ID_ARRAY.push_back(cellPointIds->GetId(j));
            }
        }

        return;
    }

private:
    // Main Data Container
    vtkSmartPointer<vtkUnstructuredGrid> data;
    vtkSmartPointer<vtkPointSource> seeds;
    vtkSmartPointer<vtkStreamTracer> streamer;
    vtkSmartPointer<vtkLookupTable> lu;
    int active_data_type = 0; //1 for temp 0 for velocity
};

#endif //vtkProcessor_H