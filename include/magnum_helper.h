#pragma once
#ifndef _MAGNUM_HELPER_H_
#define _MAGNUM_HELPER_H_
#include "basic_defs.h"
#include "magnum_defs.h"
#include "CubeMap.h"

namespace Magnum
{

using namespace Math::Literals;
Float NUMTOL = 1e-5;
Float INF = 1e8;
// Typedefs
typedef std::vector<Magnum::Float> magnumvector;
typedef std::vector<std::vector<std::vector<float>>> trivectorfloat;
typedef ResourceManager<GL::Mesh> Import_Mesh_Manager;

std::vector<std::string> split(std::string str, char delimiter)
{
    std::vector<std::string> internal;
    std::stringstream ss(str); // Turn the string into a stream.
    std::string tok;

    while (getline(ss, tok, delimiter))
    {
        internal.push_back(tok);
    }

    return internal;
}

class FloorTextureShader : public GL::AbstractShaderProgram
{
public:
    typedef GL::Attribute<0, Vector3> Position;
    typedef GL::Attribute<1, Vector2> TextureCoordinates;

    explicit FloorTextureShader();

    FloorTextureShader &setColor(const Color4 &color)
    {
        setUniform(_colorUniform, color);
        return *this;
    }

    FloorTextureShader &setTransformationProjectionMatrix(const Matrix4 &matrix)
    {
        setUniform(_transfor1mationProjectionMatrixUniform, matrix);
        return *this;
    }
    FloorTextureShader &bindTexture(GL::Texture2D &texture)
    {
        texture.bind(TextureLayer);
        return *this;
    }

private:
    enum : Int
    {
        TextureLayer = 0
    };
    Int _transfor1mationProjectionMatrixUniform;
    Int _colorUniform;
};

FloorTextureShader::FloorTextureShader()
{
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

    const Utility::Resource rs{"data"};

    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};

    vert.addSource(rs.get("Floor_texture.vert"));
    frag.addSource(rs.get("Floor_texture.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, frag}));

    GL::AbstractShaderProgram::attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());
    _transfor1mationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix");
    _colorUniform = uniformLocation("color");

    setUniform(uniformLocation("textureData"), TextureLayer);
}

class GUIPLANE : public SceneGraph::Drawable3D
{
public:
    explicit GUIPLANE(Object3D &object, const Color4 &color, GL::Mesh &&mesh, SceneGraph::DrawableGroup3D *drawables) : SceneGraph::Drawable3D{object, drawables}, _color{color}, _mesh{std::move(mesh)} {}
    void settTexture(Magnum::GL::Texture2D &tatata) { _shader.bindTexture(tatata); }

private:
    void draw(const Matrix4 &transformation, SceneGraph::Camera3D &camera) override
    {
        _shader //.setColor(_color)
            .setTransformationProjectionMatrix(camera.projectionMatrix() * transformation);
        _mesh.draw(_shader);
    }

    Color4 _color;
    FloorTextureShader _shader;
    //    Shaders::Flat3D _shader{Shaders::Flat3D::Flag::Textured};
    Magnum::GL::Mesh _mesh;
};
/* Temperature Balls */
void sphere_mesh(GL::Mesh &mesh, Vector3 &_pos)
{
    GL::Buffer buffer;
    GL::Buffer indexBuffer;
    Trade::MeshData3D data = Primitives::icosphereSolid(3);

    Matrix4 transformation = Matrix4::translation(_pos) * Matrix4::scaling(Vector3(0.01f)); //  Matrix4::rotationX(0.0_degf)* Matrix4::scaling(Vector3(0.01f));

    MeshTools::transformPointsInPlace(transformation, data.positions(0));

    buffer.setData(MeshTools::interleave(data.positions(0), data.normals(0)), GL::BufferUsage::StaticDraw);

    Containers::Array<char> indexData;
    MeshIndexType indexType;
    UnsignedInt indexStart, indexEnd;
    std::tie(indexData, indexType, indexStart, indexEnd) = MeshTools::compressIndices(data.indices());
    indexBuffer.setData(indexData, GL::BufferUsage::StaticDraw);

    mesh.setPrimitive(data.primitive())
        .setCount(data.indices().size())
        .addVertexBuffer(buffer, 0, Shaders::Phong::Position{}, Shaders::Phong::Normal{})
        .setIndexBuffer(indexBuffer, 0, indexType, indexStart, indexEnd);
}
// Sphere
class TemperatureSphere : public SceneGraph::Drawable3D
{
public:
    explicit TemperatureSphere(Vector3 _pos, Color4 _color, Object3D &object, SceneGraph::DrawableGroup3D *group) : SceneGraph::Drawable3D{object, group}
    {
        color = _color;
        sphere_mesh(_mesh, _pos);
        //Debug{} << "color" << color;
    }

private:
    void draw(const Matrix4 &transformationMatrix, SceneGraph::Camera3D &camera) override
    {
        using namespace Math::Literals;
        _shader
            //.setAmbientColor(Color4(1.0f,0.0f,0.0f,0.0f))
            .setAmbientColor(color)
            //.setDiffuseColor(0x00ff00_rgbf)
            //.setDiffuseColor(0x2f83cc_rgbf)
            //  .setShininess(200.0f)
            // .setLightPosition(camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}))
            .setTransformationMatrix(transformationMatrix)
            .setNormalMatrix(transformationMatrix.rotationScaling())
            .setProjectionMatrix(camera.projectionMatrix());
        _mesh.draw(_shader);
    }

    Color4 color;
    GL::Mesh _mesh;
    Shaders::Phong _shader;
};
/* Stream Tracer */
class StreamTracer : public SceneGraph::Drawable3D
{
public:
    explicit StreamTracer(std::vector<StreamPoint> streampoints, Object3D &object, SceneGraph::DrawableGroup3D *group) : SceneGraph::Drawable3D{object, group}
    {
        render_lines(streampoints);
    }
    void render_lines(std::vector<StreamPoint> streampoints)
    {

        GL::Buffer vertexBuffer;
        vertexBuffer.setData(streampoints);
        Containers::Array<UnsignedByte> indices{streampoints.size()};
        for (int i = 0; i < streampoints.size(); i++)
            indices[i] = i;
        GL::Buffer indexBuffer;
        indexBuffer.setData(indices, GL::BufferUsage::StaticDraw);
        _mesh.setPrimitive(MeshPrimitive::LineStrip)
            .addVertexBuffer(vertexBuffer, 0, Shaders::VertexColor3D::Position{},
                             Shaders::VertexColor3D::Color4{})
            .setIndexBuffer(indexBuffer, 0, GL::MeshIndexType::UnsignedByte)
            .setCount(streampoints.size());
    }

private:
    void draw(const Matrix4 &transformationMatrix, SceneGraph::Camera3D &camera) override
    {
        using namespace Math::Literals;
        _shader.setTransformationProjectionMatrix(camera.projectionMatrix() * transformationMatrix);
        _mesh.draw(_shader);
    }
    GL::Mesh _mesh;
    Shaders::VertexColor3D _shader;
};
/* ColourfulPoints */
/* Slice */
class ScalarPoints : public SceneGraph::Drawable3D
{
public:
    explicit ScalarPoints(std::vector<StreamPoint> streampoints, Object3D &object, SceneGraph::DrawableGroup3D *group) : SceneGraph::Drawable3D{object, group}
    {
        GL::Buffer vertexBuffer;
        vertexBuffer.setData(streampoints, GL::BufferUsage::StaticDraw);
        _mesh.setPrimitive(MeshPrimitive::Points)
            .addVertexBuffer(vertexBuffer, 0, Shaders::VertexColor3D::Position{},
                             Shaders::VertexColor3D::Color4{})
            .setCount(streampoints.size());
    }

private:
    void draw(const Matrix4 &transformationMatrix, SceneGraph::Camera3D &camera) override
    {
        using namespace Math::Literals;
        _shader.setTransformationProjectionMatrix(camera.projectionMatrix() * transformationMatrix);
        _mesh.draw(_shader);
    }
    GL::Mesh _mesh;
    Shaders::VertexColor3D _shader;
};

/* Slice */
class SliceVR : public SceneGraph::Drawable3D
{
public:
    explicit SliceVR(std::vector<StreamPoint> streampoints, std::vector<int> idxs, Object3D &object, SceneGraph::DrawableGroup3D *group) : SceneGraph::Drawable3D{object, group}
    {
        render_plane(streampoints, idxs);
    }
    void render_plane(std::vector<StreamPoint> streampoints, std::vector<int> idxs)
    {
        GL::Buffer vertexBuffer, indexBuffer;
        vertexBuffer.setData(streampoints, GL::BufferUsage::StaticDraw);
        indexBuffer.setData(idxs, GL::BufferUsage::StaticDraw);
        _mesh.setPrimitive(MeshPrimitive::Triangles)
            .addVertexBuffer(vertexBuffer, 0, Shaders::VertexColor3D::Position{},
                             Shaders::VertexColor3D::Color4{})
            .setIndexBuffer(indexBuffer, 0, GL::MeshIndexType::UnsignedInt)
            .setCount(idxs.size());
    }

private:
    void draw(const Matrix4 &transformationMatrix, SceneGraph::Camera3D &camera) override
    {
        using namespace Math::Literals;
        _shader.setTransformationProjectionMatrix(camera.projectionMatrix() * transformationMatrix);
        _mesh.draw(_shader);
    }
    GL::Mesh _mesh;
    Shaders::VertexColor3D _shader;
};

// Rectangle prism
class Rectengle : public SceneGraph::Drawable3D
{
public:
    explicit Rectengle(Float *Vertices, Color3 &_color, Object3D &object, SceneGraph::DrawableGroup3D *group, bool WireFrame = true) : SceneGraph::Drawable3D{object, group}
    {
        is_wireframe = WireFrame;
        if (is_wireframe)
        {
            _cube_color = Color4(_color, (1.0f));
        }
        else
        {
            _cube_color = Color4(_color, .5f);
        }

        for (int i = 0; i < 6; i++)
            bbox[i] = Vertices[i];
        for (int i = 0; i < 6; i++)
            limits[i] = Vertices[i];

        for (int i = 0; i < 6; i++)
            std::cout << bbox[i] << " ";
        std::cout << std::endl;
        cog = {(bbox[1] + bbox[0]) / 2.0f, (bbox[3] + bbox[2]) / 2.0f, (bbox[5] + bbox[4]) / 2.0f};
    }
    void update_mesh_filled()
    {
        Vector3 Position[] = {
            {bbox[0], bbox[2], bbox[4]}, // min min min
            {bbox[1], bbox[2], bbox[4]}, // max min min
            {bbox[1], bbox[3], bbox[4]}, // max mmax min
            {bbox[0], bbox[3], bbox[4]}, // min mmax min
            {bbox[0], bbox[2], bbox[5]}, // min min max
            {bbox[1], bbox[2], bbox[5]}, // max min max
            {bbox[1], bbox[3], bbox[5]}, // max max max
            {bbox[0], bbox[3], bbox[5]}, // min max max
        };
        GL::Buffer vertexBuffer;
        vertexBuffer.setData(Position, GL::BufferUsage::StaticDraw);
        UnsignedByte indices[]{

            2, 1, 0, 3, 2, 0, // back
            0, 7, 3, 7, 0, 4, // left
            2, 3, 7, 2, 7, 6, // up
            1, 2, 6, 1, 6, 5, // right
            4, 5, 6, 4, 6, 7, // front
            0, 1, 4, 1, 5, 4  // bottom
        };
        GL::Buffer indexBuffer;
        indexBuffer.setData(indices, GL::BufferUsage::StaticDraw);
        _mesh.setPrimitive(MeshPrimitive::Triangles)
            .addVertexBuffer(vertexBuffer, 0, Shaders::Flat3D::Position{})
            .setIndexBuffer(indexBuffer, 0, GL::MeshIndexType::UnsignedByte)
            .setCount(36);
        cog = {(bbox[1] + bbox[0]) / 2.0f, (bbox[3] + bbox[2]) / 2.0f, (bbox[5] + bbox[4]) / 2.0f};
    }
    void update_mesh()
    {
        Vector3 Position[] = {
            {bbox[0], bbox[2], bbox[4]},
            {bbox[1], bbox[2], bbox[4]},
            {bbox[1], bbox[3], bbox[4]},
            {bbox[0], bbox[3], bbox[4]},
            {bbox[0], bbox[2], bbox[5]},
            {bbox[1], bbox[2], bbox[5]},
            {bbox[1], bbox[3], bbox[5]},
            {bbox[0], bbox[3], bbox[5]},
        };
        GL::Buffer vertexBuffer;
        vertexBuffer.setData(Position, GL::BufferUsage::StaticDraw);
        UnsignedByte indices[]{
            // Front
            0, 1, 1, 2, 2, 3, 3, 0,
            // Back
            4, 5, 5, 6, 6, 7, 7, 4,
            // Connect with sides
            0, 4, 1, 5, 2, 6, 3, 7};
        GL::Buffer indexBuffer;
        indexBuffer.setData(indices, GL::BufferUsage::StaticDraw);
        _mesh.setPrimitive(MeshPrimitive::Lines)
            .addVertexBuffer(vertexBuffer, 0, Shaders::Flat3D::Position{})
            .setIndexBuffer(indexBuffer, 0, GL::MeshIndexType::UnsignedByte)
            .setCount(24);
        cog = {(bbox[1] + bbox[0]) / 2.0f, (bbox[3] + bbox[2]) / 2.0f, (bbox[5] + bbox[4]) / 2.0f};
    }
    void update_sw_bbox(float *new_bbox)
    {
        for (int i = 0; i < 6; i++)
            bbox[i] = new_bbox[i];
        update_mesh();
    }
    void setLengths(Float &X, Float &Y, Float &Z)
    {
        X = (bbox[1] - bbox[0]);
        Y = (bbox[3] - bbox[2]);
        Z = (bbox[5] - bbox[4]);
    };
    Vector3 getLengths()
    {
        Vector3 Lengths = {
            (bbox[1] - bbox[0]),
            (bbox[3] - bbox[2]),
            (bbox[5] - bbox[4])};
        return Lengths;
    };

    // Visual Translate
    void translate_vis(Float x, Float y, Float z)
    {
        bbox[0] = bbox[0] + x;
        bbox[1] = bbox[1] + x;

        bbox[2] = bbox[2] + y;
        bbox[3] = bbox[3] + y;

        bbox[4] = bbox[4] + z;
        bbox[5] = bbox[5] + z;
        cog = {(bbox[1] + bbox[0]) / 2.0f, (bbox[3] + bbox[2]) / 2.0f, (bbox[5] + bbox[4]) / 2.0f};
        update_mesh();
    }
    // Visual Translate
    void translate_to_pos(Float x, Float y, Float z)
    {
        float xlength = bbox[1] - bbox[0];
        float ylength = bbox[3] - bbox[2];
        float zlength = bbox[5] - bbox[4];
        bbox[0] = xlength - x;
        bbox[1] = xlength + x;

        bbox[2] = ylength - y;
        bbox[3] = ylength + y;

        bbox[4] = zlength - z;
        bbox[5] = zlength + z;
        //  cog = {(bbox[1]+bbox[0])/2.0f, (bbox[3]+bbox[2])/2.0f,(bbox[5]+bbox[4])/2.0f };
        update_mesh();
    }
    // to keep the sliding window in the limits of the geometry. todo there a bug somewhere !

    void keep_in_geometry_limits(float x, float y, float z)
    {
        // Keep in limits
        if (bbox[0] < limits[0])
        {
            bbox[0] = bbox[0] - x;
            bbox[1] = bbox[1] - x;
        }
        if (bbox[1] > limits[1])
        {
            bbox[0] = bbox[0] - x;
            bbox[1] = bbox[1] - x;
        }
        if (bbox[2] < limits[2])
        {
            bbox[2] = bbox[2] - y;
            bbox[3] = bbox[3] - y;
        }
        if (bbox[3] > limits[3])
        {
            bbox[2] = bbox[2] - y;
            bbox[3] = bbox[3] - y;
        }
        if (bbox[4] < limits[4])
        {
            bbox[4] = bbox[4] - z;
            bbox[5] = bbox[5] - z;
        }
        if (bbox[5] > limits[5])
        {
            bbox[4] = bbox[4] - z;
            bbox[5] = bbox[5] - z;
        }
        cog = {(bbox[1] + bbox[0]) / 2.0f, (bbox[3] + bbox[2]) / 2.0f, (bbox[5] + bbox[4]) / 2.0f};
    }
    Vector3 get_center_of_gravity() { return cog; }
    void set_center_of_gravity(Vector3 cog_) { cog = cog_; }
    void scaleThroughCoG(float factorx, float factory, float factorz)
    {

        bbox[0] = bbox[0] * factorx;
        bbox[1] = bbox[1] * factorx;
        bbox[2] = bbox[2] * factory;
        bbox[3] = bbox[3] * factory;
        bbox[4] = bbox[4] * factorz;
        bbox[5] = bbox[5] * factorz;
        update_mesh();
    }
    void scaleRectangle(float factorx, float factory, float factorz)
    { // This function is written for visualization purpose. Dont use
        // it to really scale the geometries ! ! !
        float xlength = bbox[1] - bbox[0];
        float ylength = bbox[3] - bbox[2];
        float zlength = bbox[5] - bbox[4];

        bbox[0] = cog[0] - (xlength * factorx) / 2;
        bbox[1] = cog[0] + (xlength * factorx) / 2;

        bbox[2] = cog[1] - (ylength * factory) / 2;
        bbox[3] = cog[1] + (ylength * factory) / 2;

        bbox[4] = cog[2] - (zlength * factorz) / 2;
        bbox[5] = cog[2] + (zlength * factorz) / 2;
        update_mesh();
    }
    void scaleAndTranslate(float factorx, float factory, float factorz)
    { // This function is written for visualization purpose. Dont use
        // it to really scale the geometries ! ! !
        float xlength = bbox[1] - bbox[0];
        float ylength = bbox[3] - bbox[2];
        float zlength = bbox[5] - bbox[4];
        //Translate
        bbox[0] = bbox[0] * factorx;
        bbox[1] = bbox[1] * factorx;
        bbox[2] = bbox[2] * factory;
        bbox[3] = bbox[3] * factory;
        bbox[4] = bbox[4] * factorz;
        bbox[5] = bbox[5] * factorz;
        update_mesh();
    }
    void scaleX(float factorx)
    { // This function is written for visualization purpose. Dont use
        // it to really scale the geometries ! ! !
        float xlength = bbox[1] - bbox[0];
        float midx = (bbox[1] + bbox[0]) / 2;
        bbox[0] = midx - (xlength * factorx) / 2;
        bbox[1] = midx + (xlength * factorx) / 2;
        update_mesh();
    }
    void scaleY(float factory)
    { // This function is written for visualization purpose. Dont use
        // it to really scale the geometries ! ! !
        float ylength = bbox[3] - bbox[2];
        float midy = (bbox[3] + bbox[2]) / 2;
        bbox[2] = midy - (ylength * factory) / 2;
        bbox[3] = midy + (ylength * factory) / 2;
        update_mesh();
    }
    void scaleZ(float factorz)
    { // This function is written for visualization purpose. Dont use
        // it to really scale the geometries ! ! !
        float zlength = bbox[5] - bbox[4];
        float midz = (bbox[5] + bbox[4]) / 2;
        bbox[4] = midz - (zlength * factorz) / 2;
        bbox[5] = midz + (zlength * factorz) / 2;
        update_mesh();
    }
    Float *get_bbox() { return bbox; }

private:
    void draw(const Matrix4 &transformationMatrix, SceneGraph::Camera3D &camera) override
    {
        if (is_wireframe)
            update_mesh();
        else
            update_mesh_filled();
        using namespace Math::Literals;
        _shader.setColor(_cube_color)
            .setTransformationProjectionMatrix(camera.projectionMatrix() * transformationMatrix);
        _mesh.draw(_shader);
        //  Debug{}<<x_min<<y_min<<z_min<<x_max<<y_max<<z_max;
    }

    // Bounding box limits.
    Float bbox[6];                              // bounding box limit
    Float limits[6] = {0, 270, 0, 270, 0, 270}; // limit to keep sliding window in bbox
    Vector3 cog;                                //center of gravity
    Color4 _cube_color;
    GL::Mesh _mesh;
    Shaders::Flat3D _shader;
    bool is_wireframe = false;
};
class Origin : public SceneGraph::Drawable3D
{
public:
    explicit Origin(Object3D &object, SceneGraph::DrawableGroup3D *group) : SceneGraph::Drawable3D{object, group}
    {
        Vector3 Position[] = {
            {.0f, 0.0f, 0.0f},
            {0.3f, 0.0f, 0.0f},
            {.0f, 0.0f, 0.0f},
            {0.0f, .3f, 0.0f},
            {.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, .3f}};
        GL::Buffer vertexBuffer;
        vertexBuffer.setData(Position, GL::BufferUsage::StaticDraw);
        UnsignedByte indicesz[]{4, 5};
        UnsignedByte indicesy[]{2, 3};
        UnsignedByte indicesx[]{0, 1};
        GL::Buffer indexBufferx;
        GL::Buffer indexBuffery;
        GL::Buffer indexBufferz;
        indexBufferx.setData(indicesx, GL::BufferUsage::StaticDraw);
        indexBuffery.setData(indicesy, GL::BufferUsage::StaticDraw);
        indexBufferz.setData(indicesz, GL::BufferUsage::StaticDraw);

        x.setPrimitive(MeshPrimitive::Lines)
            .addVertexBuffer(vertexBuffer, 0, Shaders::Flat3D::Position{})
            .setIndexBuffer(indexBufferx, 0, GL::MeshIndexType::UnsignedByte)
            .setCount(2);
        y.setPrimitive(MeshPrimitive::Lines)
            .addVertexBuffer(vertexBuffer, 0, Shaders::Flat3D::Position{})
            .setIndexBuffer(indexBuffery, 0, GL::MeshIndexType::UnsignedByte)
            .setCount(2);
        z.setPrimitive(MeshPrimitive::Lines)
            .addVertexBuffer(vertexBuffer, 0, Shaders::Flat3D::Position{})
            .setIndexBuffer(indexBufferz, 0, GL::MeshIndexType::UnsignedByte)
            .setCount(2);
    }

private:
    void draw(const Matrix4 &transformationMatrix, SceneGraph::Camera3D &camera) override
    {
        using namespace Math::Literals;
        _shader.setColor(colorx);
        _shader.setTransformationProjectionMatrix(camera.projectionMatrix() * transformationMatrix);
        x.draw(_shader);
        _shader.setColor(colory);
        y.draw(_shader);
        _shader.setColor(colorz);
        z.draw(_shader);
    }
    // x = red, y = green, z = blue
    Color3 colorx = 0xff0000_rgbf, colory = 0x00ff00_rgbf, colorz = 0x0000ff_rgbf;
    GL::Mesh x, y, z;
    Shaders::Flat3D _shader;
};

class WireframeDrawable : public SceneGraph::Drawable3D
{
public:
    explicit WireframeDrawable(Object3D &object, const Color4 &color, GL::Mesh &&mesh, SceneGraph::DrawableGroup3D *drawables) : SceneGraph::Drawable3D{object, drawables}, _color{color}, _mesh{std::move(mesh)} {}

private:
    void draw(const Matrix4 &transformation, SceneGraph::Camera3D &camera) override
    {
        _shader.setColor(_color)
            .setTransformationProjectionMatrix(camera.projectionMatrix() * transformation);
        _mesh.draw(_shader);
    }

    Color4 _color;
    Shaders::Flat3D _shader;
    GL::Mesh _mesh;
};

class Imported_Geometry : public Object3D, SceneGraph::Drawable3D
{
public:
    explicit Imported_Geometry(std::string filename, Object3D *object,
                               Float *_bbox, GL::Mesh &&mesh,
                               SceneGraph::DrawableGroup3D &group) : SceneGraph::Drawable3D{*object, &group}, bbox(_bbox), _mesh{std::move(mesh)}
    {
        static int geometry_number = 0;
        static Color4 color = Color4::fromHsv({20.0_degf, 1.0f, .5f}, 1.0f);
        _color = color;
        _id = geometry_number; // set id
        name = strdup((filename.substr(0, filename.length() - 4) + "-" + std::to_string(geometry_number)).c_str());
        if (_id == 0)
            _color = Color4::fromHsv({color.hue() + 20.0_degf, 0.0f, 1.0f});

        color = Color4::fromHsv({color.hue() + 60.0_degf, 1.0f, .6f});
        geometry_number++;

        // Center of BBOX
        cog[0] = (bbox[0] + bbox[1]) / 2.0;
        cog[1] = (bbox[2] + bbox[3]) / 2.0;
        cog[2] = (bbox[4] + bbox[5]) / 2.0;
    }

    // Functions for frontend
    void modify_selected()
    {
        if (is_selected)
            is_selected = false;
        else
            is_selected = true;
    }
    onevectorfloat get_position() { return position_modification; }
    void set_position(onevectorfloat _position_modification)
    {
        position_modification = _position_modification;
        //cog[0] = cog[0] + position_modification[0];
        //cog[1] = cog[1] + position_modification[1];
        //cog[2] = cog[2] + position_modification[2];
    }
    Vector3 get_center_of_gravity()
    {
        return cog;
    }
    void set_center_of_gravity(Vector3 cog_) { cog = cog_; }

    void set_primary_geometry(int decision)
    {
        is_primary_geometry = decision;
    }
    // Public atributes
    const char *name = "geo";
    int _id = 0;

private:
    void draw(const Matrix4 &transformationMatrix, SceneGraph::Camera3D &camera) override;
    bool is_selected = false;
    int is_primary_geometry = 0;
    Float *bbox;
    Vector3 cog, _light;
#ifdef CAVE
    Shaders::Phong _shader{{}, 4};
#else
    Shaders::Phong _shader;
#endif
    //Shaders::MeshVisualizer _shader{Shaders::MeshVisualizer::Flag::Wireframe};
    onevectorfloat position_modification = onevectorfloat(6, 0.0f); // translate(x,y,z) rotate(x,y,z) meters / degrees
    bool is_scale = false;
    GL::Mesh _mesh;
    Color4 _color = 0xffffff_rgbf;   // White
    std::vector<Vector3> cave_lights{//{0,0,0},{0,250,0},
                                     //{250,0,0},{250,250,0},
                                     {270, 270, 0},
                                     {0, 270, 270},
                                     {270, 270, 270},
                                     {0, 270, 0}};
    std::vector<Vector3> cave_lights2{
         {0, 0, 0},
    //     {270, 0, 0},
         {0,  0, 270},
        // {270, 0, 270},
         {0, 270, 0},
        //  {270, 270, 0},
    //   {0, 270, 270},
          {270, 270, 270}
       };
};
void Imported_Geometry::draw(const Matrix4 &transformationMatrix, SceneGraph::Camera3D &camera)
{
    if (is_selected)
        _shader.setDiffuseColor(0xff00ff_rgbf); // purple
    else
        _shader.setDiffuseColor(_color); // white

    _shader
    .setAmbientColor(0x000000_rgbf)
//.setDiffuseColor(0x2f83cc_rgbf)
.setShininess(1000.0f)
#ifdef CAVE
        .setLightPositions(cave_lights2)
#else
        .setLightPosition({135, 135, 135})
#endif
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotationScaling())
        .setProjectionMatrix(camera.projectionMatrix());
    // _shader.setColor(_color)
    // .setWireframeColor(0x000000_rgbf)
    // .setViewportSize(Vector2{GL::defaultFramebuffer.viewport().size()})
    // .setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix);

    _mesh.draw(_shader);
}

class FlyStick : public Object3D, SceneGraph::Drawable3D
{
public:
    explicit FlyStick(Object3D &parent, SceneGraph::DrawableGroup3D *group) : SceneGraph::Drawable3D{parent, group}
    {
        Vector3 Position[] = {
            {-5.5f, -14.0f, -4.35f},
            {5.5f, -14.0f, -4.35f},
            {5.5f, 14.0f, -4.35f},
            {-5.5f, 14.0f, -4.35f},
            {-5.5f, -14.0f, 4.35f},
            {5.5f, -14.0f, 4.35f},
            {5.5f, 14.0f, 4.35f},
            {-5.5f, 14.0f, 4.35f},
        };
        GL::Buffer vertexBuffer;
        vertexBuffer.setData(Position, GL::BufferUsage::StaticDraw);
        UnsignedByte indices[]{
            // Front
            0, 1, 1, 2, 2, 3, 3, 0,
            // Back
            4, 5, 5, 6, 6, 7, 7, 4,
            // Connect with sides
            0, 4, 1, 5, 2, 6, 3, 7};
        GL::Buffer indexBuffer;
        indexBuffer.setData(indices, GL::BufferUsage::StaticDraw);
        FlyStick_Outline.setPrimitive(MeshPrimitive::Lines)
            .addVertexBuffer(vertexBuffer, 0, Shaders::Flat3D::Position{})
            .setIndexBuffer(indexBuffer, 0, GL::MeshIndexType::UnsignedByte)
            .setCount(24);

        //    tip_on_gui = new TemperatureSphere({0.0f, 0.0f, 1.0f},0xff0000_rgbf, parent, group);

        update_line({.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}); // {270.0f, 270.0f, 270.0f});
    }
    void calculate_second_point(Vector3 Point_Wand)
    {
    }
    void update_line(Vector3 Point_Wand, Vector3 Point_Plane)
    {
        Debug{} << Point_Wand << Point_Plane;
        Position[0] = Point_Wand;
        Position[1] = Point_Plane;
        vertexBuffer.setData(Position, GL::BufferUsage::StaticDraw);
        UnsignedByte indicesx[]{0, 1};
        indexBufferx.setData(indicesx, GL::BufferUsage::StaticDraw);
        Line.setPrimitive(MeshPrimitive::Lines)
            .addVertexBuffer(vertexBuffer, 0, Shaders::Flat3D::Position{})
            .setIndexBuffer(indexBufferx, 0, GL::MeshIndexType::UnsignedByte)
            .setCount(2);
    }

private:
    void draw(const Matrix4 &transformationMatrix, SceneGraph::Camera3D &camera) override
    {
        using namespace Math::Literals;
        _shader.setColor(colorx);
        _shader.setTransformationProjectionMatrix(camera.projectionMatrix() * transformationMatrix);
        Line.draw(_shader);
        FlyStick_Outline.draw(_shader);
    }
    Color3 colorx = 0xffff00_rgbf;
    GL::Mesh FlyStick_Outline;
    GL::Mesh Line;
    Vector3 Position[2];
    GL::Buffer indexBufferx, vertexBuffer;
    Shaders::Flat3D _shader;
    GL::Mesh *tip_on_gui;
}; // namespace Magnum
Matrix4x4 glm2magnum(const glm::mat4 &input)
{

    Matrix4 tmp(Vector4(input[0][0], input[0][1], input[0][2], input[0][3]),
                Vector4(input[1][0], input[1][1], input[1][2], input[1][3]),
                Vector4(input[2][0], input[2][1], input[2][2], input[2][3]),
                Vector4(input[3][0], input[3][1], input[3][2], input[3][3]));
    return tmp;
}

class VirtualCursor : public Object2D, SceneGraph::Drawable2D
{
public:
    explicit VirtualCursor(Object2D &parent, SceneGraph::DrawableGroup2D &drawables) : Object2D{&parent}, SceneGraph::Drawable2D{*this, &drawables}
    {
        struct TriangleVertex
        {
            Vector2 position;
            Color3 color;
        };
        const TriangleVertex data[]{
            {{-0.5f, -0.5f}, 0xff0000_rgbf}, /* Left vertex, red color */
            {{0.5f, -0.5f}, 0x00ff00_rgbf},  /* Right vertex, green color */
            {{0.0f, 0.5f}, 0x0000ff_rgbf}    /* Top vertex, blue color */
        };

        GL::Buffer buffer;
        buffer.setData(data);

        _mesh.setCount(3)
            .addVertexBuffer(std::move(buffer), 0,
                             Shaders::VertexColor2D::Position{},
                             Shaders::VertexColor2D::Color3{});
    }

private:
    void draw(const Matrix3 &transformationMatrix, SceneGraph::Camera2D &camera) override
    {
        _shader
            .setTransformationProjectionMatrix(camera.projectionMatrix() * transformationMatrix);
        _mesh.draw(_shader);
    }

    GL::Mesh _mesh;
    Shaders::VertexColor2D _shader;
};
} // namespace Magnum
#endif
