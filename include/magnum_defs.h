#ifndef MAGNUM_DEFS_H
#define MAGNUM_DEFS_H

// Magnum Defs
//Corrade 
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Assert.h>
#include <Corrade/Containers/ArrayViewStl.h>
#ifndef CAVE
#include <Corrade/Utility/DebugStl.h>
#endif
// Platform
#include <Magnum/Platform/GlfwApplication.h>
// Magnum GL
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/CubeMapTexture.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/GL/AbstractObject.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderbuffer.h>
// Magnum Trade
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshObjectData3D.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>
// Magnum Math
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Range.h>

// Magnum Mesh
#include <Magnum/Mesh.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/MeshTools/GenerateFlatNormals.h>
// Magnum Sceneography
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/MatrixTransformation2D.h>
#include <Magnum/SceneGraph/TranslationRotationScalingTransformation2D.h>
// Magnum Shaders
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Shaders/VertexColor.h>
#include<Magnum/Shaders/MeshVisualizer.h>
// Magnum Primitives
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Cone.h>
#include <Magnum/Primitives/Cylinder.h>
#include <Magnum/Primitives/Crosshair.h>
#include <Magnum/Primitives/Icosphere.h>
#include <Magnum/Primitives/Plane.h>
// Magnum Rest
#include <Magnum/DebugTools/Screenshot.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/ResourceManager.h>
#include <Magnum/ImageView.h>
#include <Magnum/Image.h>

typedef Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation3D> Object3D;
typedef Magnum::SceneGraph::Scene<Magnum::SceneGraph::MatrixTransformation3D> Scene3D;
typedef Magnum::SceneGraph::Object<Magnum::SceneGraph::TranslationRotationScalingTransformation2D> Object2D;
typedef Magnum::SceneGraph::Scene<Magnum::SceneGraph::TranslationRotationScalingTransformation2D> Scene2D;

/* Streamtreacer*/
struct StreamPoint
{
    StreamPoint(){};
    Magnum::Vector3 pos;
    Magnum::Color4 color;
    void printself(){Magnum::Debug{}<<pos<<color;}
};













#endif
