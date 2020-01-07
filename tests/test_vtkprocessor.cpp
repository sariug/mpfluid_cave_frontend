#define BOOST_TEST_DYN_LINK
#ifdef STAND_ALONE
#define BOOST_TEST_MODULE Main
#endif
#include <boost/test/unit_test.hpp>

#include "../include/vtkProcessor.h"


BOOST_AUTO_TEST_SUITE(test2_suite)

vtkProcessor Processor;

/*
BOOST_AUTO_TEST_CASE(VTK_READING_TEST)
{
    BOOST_CHECK(Processor.read_unstructered_vtk("/home/ugurcan/Desktop/mpfluid_vis_sari/src/scenograph/tests/test_vtk.vtk")==0);
    
}
BOOST_AUTO_TEST_CASE(VTK_WRITTING_TEST)
{
    if(Processor.read_unstructered_vtk("/home/ugurcan/Desktop/mpfluid_vis_sari/src/scenograph/tests/test_vtk.vtk")==0)
        BOOST_CHECK(Processor.write_unstructered_vtk("/home/usari/Desktop/mpfluid_vis_sari/src/scenograph/tests/write_sample_vtk.vtk")==0);
}

BOOST_AUTO_TEST_CASE(RENDER_STREAMLINES)
{
    //BOOST_CHECK(Processor.read_unstructered_vtk("/home/usari/Schreibtisch/mpfluid_vis_sari/src/scenograph/tests/test_vtk.vtk")==0);
    Processor.read_unstructered_vtk("/home/ugurcan/Desktop/mpfluid_vis_sari/src/scenograph/tests/test_vtk.vtk");
    float bbox[6]={0, 6 ,0,6,0,3};
    Processor.set_seed_source(bbox);
    Processor.calculate_streamlines(5);
}

BOOST_AUTO_TEST_CASE(RENDER_STREAMLINES)
{
    //BOOST_CHECK(Processor.read_unstructered_vtk("/home/usari/Schreibtisch/mpfluid_vis_sari/src/scenograph/tests/test_vtk.vtk")==0);
    Processor.read_unstructered_vtk("/home/ugurcan/Desktop/mpfluid_vis_sari/src/scenograph/tests/test_vtk.vtk");
    Processor.slice_data();

}
*/

BOOST_AUTO_TEST_SUITE_END()