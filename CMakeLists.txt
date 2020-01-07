###############################################################################
# Ugurcan Sari
###############################################################################

# minimal version of cmake
cmake_minimum_required (VERSION 2.8)

project(Sceneography)
set(synch_DIR "${CMAKE_CURRENT_SOURCE_DIR}/home/ugurcan/Desktop/vorlesung_vr_projekte-master/libs/synchInstalled/cmake" CACHE PATH "Installation directory of synch library")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/modules/")
# include the general cmake settings
include (../general.cmake)

option (CAVE  "Using CAVE configurations" OFF )  # Also forcing it to be always on
if(CAVE)
  add_definitions(-DCAVE)   
endif()
option (BUILD_SCENEGRAPH_TESTS "Build SceneGraph Tests" OFF)

# FIND LIBRARIES ! 
message(STATUS "Using Glfw")
find_package(Magnum REQUIRED
    GL
    GlfwApplication        
    MeshTools
    Primitives
    Shaders
    DebugTools
    SceneGraph
    Trade)


find_package(synch REQUIRED)
find_package(MagnumIntegration REQUIRED ImGui)
# VTK
find_package(VTK)
# BOOST
FIND_PACKAGE( Boost 1.40 REQUIRED serialization system  )


# INCLUDE STUFF
include(${VTK_USE_FILE})
#INCLUDE DIRECTORIES
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/tinyxml2)
include_directories(${CMAKE_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
include_directories(${CMAKE_SOURCE_DIR}/../pv_plugin/eafdata)
# SET STUFF
set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)
corrade_add_resource(Texture_Floor  src/node/resources.conf)
# FILE STUFF
FILE(GLOB_RECURSE TINY_XML2
    #3rdparty/tinyxml2/*.h
    3rdparty/tinyxml2/*.cpp)

FILE(GLOB SOURCE_FILES 
    src/*.cpp)
FILE(GLOB SOURCE_MASTER 
    src/master/*.cpp)
FILE(GLOB SOURCE_NODE
    src/node/*.cpp)
###################################################################
# Master Executable
###################################################################
set(EXECUTABLE_OUTPUT_PATH ../bin/) 
add_executable(scene_master
            ${SOURCE_MASTER}
            src/GUI/ImGui_interface.cpp
            )
target_link_libraries(scene_master PRIVATE
    Magnum::Application
    Magnum::GL
    Magnum::GlfwApplication
    Magnum::DebugTools
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::Trade
    MagnumIntegration::ImGui # bunlari 18 nisanda ekledim
    synch
    ${FLUID_BASENAME}Lib ${ADDITIONAL_FLUID_LIBRARIES} 
    ${Boost_LIBRARIES} )
###################################################################
# Node Executable
###################################################################
set(EXECUTABLE_OUTPUT_PATH ../bin/)
add_executable(scene
            ${SOURCE_NODE}
            ${Texture_Floor}
            ../pv_plugin/eafdata/vtkEAFWriter.cxx
            src/GUI/ImGui_interface.cpp
            )
target_link_libraries(scene PRIVATE
    Magnum::Application
    Magnum::GL
    Magnum::GlfwApplication
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::DebugTools
    Magnum::Primitives
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::Trade
    MagnumIntegration::ImGui
    synch
    ${FLUID_BASENAME}Lib ${ADDITIONAL_FLUID_LIBRARIES} ${Boost_LIBRARIES} )


install(TARGETS scene DESTINATION bin/scenograph)
install(TARGETS scene_master DESTINATION bin/scenograph)
FILE(GLOB scenenograph_files "${CMAKE_CURRENT_SOURCE_DIR}/scenograph_starters/*")
install(FILES ${scenenograph_files} PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ  DESTINATION bin)

####################################################################
# Build Tests
####################################################################
if(BUILD_SCENEGRAPH_TESTS)
  # BOOST
FIND_PACKAGE( Boost 1.40 REQUIRED  unit_test_framework)

#INCLUDE DIRECTORIES
include_directories(${CMAKE_SOURCE_DIR}/../include)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

FILE(GLOB TESTSOURCES tests/*.cpp)

set(EXECUTABLE_OUTPUT_PATH ../bin/)
add_executable(tests
            ${TESTSOURCES}
            )
target_link_libraries(tests PRIVATE
    ${FLUID_BASENAME}Lib ${ADDITIONAL_FLUID_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

endif(BUILD_SCENEGRAPH_TESTS)

FILE(GLOB geofiles "${CMAKE_CURRENT_SOURCE_DIR}/geometry_files/*.eaf")
install(FILES ${geofiles} DESTINATION geo_examples)

#FILE(GLOB configurations "${CMAKE_CURRENT_SOURCE_DIR}/geometry_configurations/*")
#install(FILES ${configurations} DESTINATION geometry_configurations)

