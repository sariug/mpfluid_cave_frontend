#include <iostream>
#include <fstream>
#include <sstream>      // std::stringstream, std::stringbuf
#include <string>
#include <vector>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include "Magnum/Mesh.h"
#include "Magnum/Trade/MeshData3D.h"
// --- definition of attribute types -------------------------------------------
#define ATTR_UNDEF    0
#define ATTR_INTEGER  1
#define ATTR_DOUBLE   2
#define ATTR_FLOAT    3
#define ATTR_STRING   4

typedef double DATA_TYPE_COORD;
typedef struct Face {
	int v1, v2, v3;				// vertex index list

	int nx, ny, nz;			// face normal
} Face;

namespace Magnum { 
using namespace Math::Literals;
using std::vector;
class EAF_LOADER
{
    public:
        EAF_LOADER(std::string Filename):filename(Filename){};
        int load_eaf()
        {
            	using namespace std;

            // opening instream
            ifstream is(filename, ios::in | ios::binary);
            if(!is) {
                cerr << "ERROR: GeoModel::load_model(): Unable to open input filename : " << filename << "\n";
                return 1;
            }
            is.precision(15);

            string line, dummy;
            int ct_line = 0;
            int n_pts = 0, n_trs = 0, n_eds = 0, f_pts = 0, f_trs = 0, f_eds = 0, f_norm = 0;
            // read keyword and amount of points
            {
                if(!getline(is,line,'\n')) {
                    cerr << "ERROR: GeoModel::load_model(): File ended before starting at all!\n";
                    return 1;
                } else {
                    ct_line++;
                }
   
                stringstream iss(line);
                string key;
                iss >> key;
                if(key!="EAF" && key!="eaf" && ct_line==1) {
                    cerr << "ERROR: GeoModel::load_model(): EAF File Format is supposed to start with the keyword 'EAF' or 'eaf' !\n";
                    cerr << "Line >>" << line << "<<\n";
                    return 1;
                }
            }

            // reading in and ignoring comment line (line 2)
            if(!getline(is,line,'\n')) {cerr << "ERROR: GeoModel::load_model(): File ended too early\n"; return 1;}
            ct_line++;

            // reading in format of file (binary | ascii)
            bool is_binary = false;
            if(!getline(is,line,'\n')) {cerr << "ERROR: GeoModel::load_model(): File ended too early\n"; return 1;}
            ct_line++;
            if(line=="binary") is_binary = true;

        // reading in amount of points etc
            {
                if(!getline(is,line,'\n')) {cerr << "ERROR: GeoModel::load_model(): File ended too early\n"; return 1;}
                ct_line++;
                istringstream isn(line);
                isn >> n_pts >> n_trs >> n_eds >> f_pts >> f_trs >> f_eds >> f_norm;
                if(n_pts<=0 || n_trs<=0 || n_eds<0) {
                    cerr << "ERROR: GeoModel::load_model(): (Line " << ct_line << ") Amount of points or triangles or edges are not matching!\n";
                    return 1;
                }
            }
            /* face list */


            // initialize fields
            nvertex = n_pts;
            nface = n_trs;
            //vlist.resize(nvertex);
            //flist.resize(nvertex)
            // Containers::Array<Vector3> vlist{nvertex};
            // Containers::Array<Face> flist{nface};
            vlist.resize(nvertex);
            flist.resize(nface);
            normals.resize(nvertex, Vector3());//, Vector3::Vector3(0.0f,.0f,.0f));
            //vlist = new BMGeoVertex [nvertex];
            //flist = new BMGeoFace [nface];

        if(is_binary) {
            cerr << "ERROR: Binary reading is not implemented !\n";
            return 1;
        } else {// read in ascii format

            // read in points
            for(int ct_pt=0;ct_pt<n_pts;++ct_pt)
            {
                if(!getline(is,line,'\n')) {
                    cerr << "ERROR: GeoModel::load_model(): File ended before all nodes were read in!\n";
                    return 1;
                } else {
                    ct_line++;
                }
                if(line.size()==0) {
                    ct_pt--;
                    continue;
                }
                stringstream iss(line);
                // point coordinates
                iss >> vlist[ct_pt][0] >> vlist[ct_pt][1] >> vlist[ct_pt][2];
            }

            // read in faces
            for(int ct_tr=0;ct_tr<n_trs;++ct_tr)
            {
                if(!getline(is,line,'\n')) {
                    cerr << "ERROR: GeoModel::load_model(): File ended before all tri elements were read in!\n";
                    return 1;
                } else {
                    ct_line++;
                }
                if(line.size()==0) {
                    ct_tr--;
                    continue;
                }

                stringstream iss(line);

                // tri elements
                int n[3];
                iss >> n[0] >> n[1] >> n[2];
                double dummy;
                if(f_norm!=0) {
                    // read normal components (ignored here)
                    iss >> dummy >> dummy >> dummy;
                }

                // node ids for tris
                flist[ct_tr].v1 = n[0];
                flist[ct_tr].v2 = n[1];
                flist[ct_tr].v3 = n[2];
                positionIndices.push_back(n[0]);
                positionIndices.push_back(n[1]);
                positionIndices.push_back(n[2]);
            }
        }
        
        is.close();
        return 0;
    }

    Containers::Optional<Trade::MeshData3D> get_mesh()
    {
		// calculate normals
		for(int i = 0; i<flist.size();i++)
		{
            Vector3 p1=vlist[flist[i].v1], p2=vlist[flist[i].v2], p3=vlist[flist[i].v3];
            Vector3 n = Math::cross( (p2 - p1).normalized(),(p3 - p1).normalized() );

            auto v21n = (p2 - p1).normalized();
            auto v31n = (p3 - p1).normalized();
            auto v32n = (p3 - p2).normalized();

            if(Math::isNan(v21n) || Math::isNan(v32n) || Math::isNan(v31n)) 
            n = Vector3::Vector();

            auto a1 = Math::angle(v21n,  v31n);    // p1 is the 'base' here
            auto a2 = Math::angle(v32n,  -v21n);    // p2 is the 'base' here
            auto a3 = Math::angle(-v31n, -v32n);    // p3 is the 'base' here
            normals[flist[i].v1] = n* float(a1) + normals[flist[i].v1];
            normals[flist[i].v2] = n* float(a2) + normals[flist[i].v2]; 
            normals[flist[i].v3] = n* float(a3) + normals[flist[i].v3];

            //	Set Normal.x to (multiply U.y by V.z) minus (multiply U.z by V.y)
            //	Set Normal.y to (multiply U.z by V.x) minus (multiply U.x by V.z)
            //	Set Normal.z to (multiply U.x by V.y) minus (multiply U.y by V.x)
		}
        for(auto &n:normals)
        {
            n = n.normalized();
        }
        positions.push_back(vlist);
        // return Trade::MeshData3D{*primitive, positionIndices, positions, {}, {}, {}, nullptr};
	    for(int i = 0;i<vlist.size();i++)colors.push_back(Color4(1.0f,1.0f,1.0f,1.0f));
        return Trade::MeshData3D{MeshPrimitive::Triangles, std::move(positionIndices), {std::move(vlist)}, {std::move(normals)}, {},{colors}};
    }


    private:
        std::string filename;
		Vector3 U,V;
        int nvertex;		// amount of vertices
        int nface;			// amount of faces
        //BMGeoVertex *vlist;	// vertex list
        vector<Face> flist;
        vector<vector<Vector3>> positions;
        vector<Vector3> vlist; // positions
        vector<Vector3> normals; // normals
        vector<UnsignedInt> positionIndices;
        vector<Color4> colors;

        Containers::Optional<MeshPrimitive> primitive =  MeshPrimitive::Triangles;

};


}