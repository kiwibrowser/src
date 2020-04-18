/*
 * Copyright © 2008 Ben Smith
 * Copyright © 2010-2011 Linaro Limited
 *
 * This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
 *
 * glmark2 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * glmark2.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Ben Smith (original glmark benchmark)
 *  Alexandros Frantzis (glmark2)
 */
#include "mesh.h"
#include "model.h"
#include "vec.h"
#include "log.h"
#include "options.h"
#include "util.h"
#include "float.h"
#include "math.h"
#include <fstream>
#include <sstream>
#include <memory>

using std::string;
using std::vector;
using LibMatrix::vec2;
using LibMatrix::vec3;
using LibMatrix::uvec3;

#define read_or_fail(file, dst, size) do { \
    file.read(reinterpret_cast<char *>((dst)), (size)); \
    if (file.gcount() < (std::streamsize)(size)) { \
        Log::error("%s: %d: Failed to read %zd bytes from 3ds file (read %zd)\n", \
                   __FUNCTION__, __LINE__, \
                   (size_t)(size), file.gcount()); \
        return false; \
    } \
} while(0);

/**
 * Computes the bounding box for a Model::Object.
 *
 * @param object the Model object
 */
void
Model::compute_bounding_box(const Object& object)
{
    float minX(FLT_MAX);
    float maxX(FLT_MIN);
    float minY(FLT_MAX);
    float maxY(FLT_MIN);
    float minZ(FLT_MAX);
    float maxZ(FLT_MIN);
    for (vector<Vertex>::const_iterator vIt = object.vertices.begin(); vIt != object.vertices.end(); vIt++)
    {
        const vec3& curVtx = vIt->v;
        if (curVtx.x() < minX)
        {
            minX = curVtx.x();
        }
        if (curVtx.x() > maxX)
        {
            maxX = curVtx.x();
        }
        if (curVtx.y() < minY)
        {
            minY = curVtx.y();
        }
        if (curVtx.y() > maxY)
        {
            maxY = curVtx.y();
        }
        if (curVtx.z() < minZ)
        {
            minZ = curVtx.z();
        }
        if (curVtx.z() > maxZ)
        {
            maxZ = curVtx.z();
        }
    }
    maxVec_ = vec3(maxX, maxY, maxZ);
    minVec_ = vec3(minX, minY, minZ);
}

/**
 * Appends the vertices of a Model::Object to a Mesh.
 *
 * @param object the object to append
 * @param mesh the mesh to append to
 * @param p_pos the attribute position to use for the 'position' attribute
 * @param n_pos the attribute position to use for the 'normal' attribute
 * @param t_pos the attribute position to use for the 'texcoord' attribute
 */
void
Model::append_object_to_mesh(const Object &object, Mesh &mesh,
                             int p_pos, int n_pos, int t_pos,
                             int nt_pos, int nb_pos)
{
    for (vector<Face>::const_iterator faceIt = object.faces.begin();
         faceIt != object.faces.end();
         faceIt++)
    {
        // In some model file formats (OBJ in particular), the face description
        // may contain separate indices per-attribute.  So, we need to allow
        // for this when adding each vertex attribute to the mesh.
        const Face &face = *faceIt;
        const Vertex &v1 = object.vertices[face.v.x()];
        const Vertex &v2 = object.vertices[face.v.y()];
        const Vertex &v3 = object.vertices[face.v.z()];

        bool separate_t(face.which & Face::OBJ_FACE_T);

        const Vertex &t1 = object.vertices[separate_t ? face.t.x() : face.v.x()];
        const Vertex &t2 = object.vertices[separate_t ? face.t.y() : face.v.y()];
        const Vertex &t3 = object.vertices[separate_t ? face.t.z() : face.v.z()];

        bool separate_n(face.which & Face::OBJ_FACE_N);

        const Vertex &n1 = object.vertices[separate_n ? face.n.x() : face.v.x()];
        const Vertex &n2 = object.vertices[separate_n ? face.n.y() : face.v.y()];
        const Vertex &n3 = object.vertices[separate_n ? face.n.z() : face.v.z()];

        mesh.next_vertex();
        if (p_pos >= 0)
            mesh.set_attrib(p_pos, v1.v);
        if (n_pos >= 0)
            mesh.set_attrib(n_pos, n1.n);
        if (t_pos >= 0)
            mesh.set_attrib(t_pos, t1.t);
        if (nt_pos >= 0)
            mesh.set_attrib(nt_pos, v1.nt);
        if (nb_pos >= 0)
            mesh.set_attrib(nb_pos, v1.nb);

        mesh.next_vertex();
        if (p_pos >= 0)
            mesh.set_attrib(p_pos, v2.v);
        if (n_pos >= 0)
            mesh.set_attrib(n_pos, n2.n);
        if (t_pos >= 0)
            mesh.set_attrib(t_pos, t2.t);
        if (nt_pos >= 0)
            mesh.set_attrib(nt_pos, v2.nt);
        if (nb_pos >= 0)
            mesh.set_attrib(nb_pos, v2.nb);

        mesh.next_vertex();
        if (p_pos >= 0)
            mesh.set_attrib(p_pos, v3.v);
        if (n_pos >= 0)
            mesh.set_attrib(n_pos, n3.n);
        if (t_pos >= 0)
            mesh.set_attrib(t_pos, t3.t);
        if (nt_pos >= 0)
            mesh.set_attrib(nt_pos, v3.nt);
        if (nb_pos >= 0)
            mesh.set_attrib(nb_pos, v3.nb);
    }
}

/**
 * Converts a model to a mesh using the default attributes bindings.
 *
 * The default attributes and their order is: Position, Normal, Texcoord
 *
 * @param mesh the mesh to populate
 */
void
Model::convert_to_mesh(Mesh &mesh)
{
    std::vector<std::pair<AttribType, int> > attribs;

    attribs.push_back(std::pair<AttribType, int>(AttribTypePosition, 3));
    attribs.push_back(std::pair<AttribType, int>(AttribTypeNormal, 3));
    attribs.push_back(std::pair<AttribType, int>(AttribTypeTexcoord, 2));

    convert_to_mesh(mesh, attribs);
}

/**
 * Converts a model to a mesh using custom attribute bindings.
 *
 * The attribute bindings are pairs of <AttribType, dimensionality>.
 *
 * @param mesh the mesh to populate
 * @param attribs the attribute bindings to use
 */
void
Model::convert_to_mesh(Mesh &mesh,
                       const std::vector<std::pair<AttribType, int> > &attribs)
{
    std::vector<int> format;
    int p_pos = -1;
    int n_pos = -1;
    int t_pos = -1;
    int nt_pos = -1;
    int nb_pos = -1;

    mesh.reset();

    for (std::vector<std::pair<AttribType, int> >::const_iterator ai = attribs.begin();
         ai != attribs.end();
         ai++)
    {
        format.push_back(ai->second);
        if (ai->first == AttribTypePosition)
            p_pos = ai - attribs.begin();
        else if (ai->first == AttribTypeNormal)
            n_pos = ai - attribs.begin();
        else if (ai->first == AttribTypeTexcoord)
            t_pos = ai - attribs.begin();
        else if (ai->first == AttribTypeTangent)
            nt_pos = ai - attribs.begin();
        else if (ai->first == AttribTypeBitangent)
            nb_pos = ai - attribs.begin();
    }

    mesh.set_vertex_format(format);

    for (std::vector<Object>::const_iterator iter = objects_.begin();
         iter != objects_.end();
         iter++)
    {
        append_object_to_mesh(*iter, mesh, p_pos, n_pos, t_pos, nt_pos, nb_pos);
    }
}

void
Model::calculate_texcoords()
{
    if (gotTexcoords_)
        return;

    // Since the model didn't come with texcoords, and we don't actually know
    // if it came with normals, either, we'll use positional spherical mapping
    // to generate texcoords for the model.  See:
    // http://www.mvps.org/directx/articles/spheremap.htm for more details.
    vec3 centerVec = maxVec_ + minVec_;
    centerVec *= 0.5;
    
    for (std::vector<Object>::iterator iter = objects_.begin();
         iter != objects_.end();
         iter++)
    {
        Object &object = *iter;
        for (vector<Vertex>::iterator vertexIt = object.vertices.begin();
             vertexIt != object.vertices.end();
             vertexIt++)
        {
            Vertex& curVertex = *vertexIt;
            vec3 vnorm(curVertex.v - centerVec);
            vnorm.normalize();
            curVertex.t.x(asinf(vnorm.x()) / M_PI + 0.5);
            curVertex.t.y(asinf(vnorm.y()) / M_PI + 0.5);
        }
    }

    gotTexcoords_ = true;
}

/**
 * Calculates the normal vectors of the model vertices.
 */
void
Model::calculate_normals()
{
    if (gotNormals_)
        return;

    LibMatrix::vec3 n;

    for (std::vector<Object>::iterator iter = objects_.begin();
         iter != objects_.end();
         iter++)
    {
        Object &object = *iter;

        for (vector<Face>::const_iterator f_iter = object.faces.begin();
             f_iter != object.faces.end();
             f_iter++)
        {
            const Face &face = *f_iter;
            Vertex &a = object.vertices[face.v.x()];
            Vertex &b = object.vertices[face.v.y()];
            Vertex &c = object.vertices[face.v.z()];

            /* Calculate normal */
            n = LibMatrix::vec3::cross(b.v - a.v, c.v - a.v);
            n.normalize();
            a.n += n;
            b.n += n;
            c.n += n;

            LibMatrix::vec3 q1(b.v - a.v);
            LibMatrix::vec3 q2(c.v - a.v);
            LibMatrix::vec2 u1(b.t - a.t);
            LibMatrix::vec2 u2(c.t - a.t);
            float det = (u1.x() * u2.y() - u2.x() * u1.y());

            /* Calculate tangent */
            LibMatrix::vec3 nt;
            nt.x(det * (u2.y() * q1.x() - u1.y() * q2.x()));
            nt.y(det * (u2.y() * q1.y() - u1.y() * q2.y()));
            nt.z(det * (u2.y() * q1.z() - u1.y() * q2.z()));
            nt.normalize();
            a.nt += nt;
            b.nt += nt;
            c.nt += nt;

            /* Calculate bitangent */
            LibMatrix::vec3 nb;
            nb.x(det * (u1.x() * q2.x() - u2.x() * q1.x()));
            nb.y(det * (u1.x() * q2.y() - u2.x() * q1.y()));
            nb.z(det * (u1.x() * q2.z() - u2.x() * q1.z()));
            nb.normalize();
            a.nb += nb;
            b.nb += nb;
            c.nb += nb;
        }

        for (vector<Vertex>::iterator v_iter = object.vertices.begin();
             v_iter != object.vertices.end();
             v_iter++)
        {
            Vertex &v = *v_iter;
            /* Orthogonalize */
            v.nt = (v.nt - v.n * LibMatrix::vec3::dot(v.nt, v.n));
            v.n.normalize();
            v.nt.normalize();
            v.nb.normalize();
        }
    }

    gotNormals_ = true;
}

/**
 * Load a model from a 3DS file.
 *
 * @param filename the name of the file
 *
 * @return whether loading succeeded
 */
bool
Model::load_3ds(const std::string &filename)
{
    Object *object(0);

    Log::debug("Loading model from 3ds file '%s'\n", filename.c_str());

    const std::unique_ptr<std::istream> input_file_ptr(Util::get_resource(filename));
    std::istream& input_file(*input_file_ptr);

    if (!input_file) {
        Log::error("Could not open 3ds file '%s'\n", filename.c_str());
        return false;
    }

    // Loop to scan the whole file
    while (!input_file.eof()) {
        uint16_t chunk_id;
        uint32_t chunk_length;

        // Read the chunk header
        input_file.read(reinterpret_cast<char *>(&chunk_id), 2);
        if (input_file.gcount() == 0) {
            continue;
        }
        else if (input_file.gcount() < 2) {
            Log::error("%s: %d: Failed to read %zd bytes from 3ds file (read %zd)\n",
                       __FUNCTION__, __LINE__, 2, input_file.gcount());
            return false;
        }

        //Read the length of the chunk
        read_or_fail(input_file, &chunk_length, 4);

        switch (chunk_id)
        {
            //----------------- MAIN3DS -----------------
            // Description: Main chunk, contains all the other chunks
            // Chunk ID: 4d4d
            // Chunk Length: 0 + sub chunks
            //-------------------------------------------
            case 0x4d4d:
                break;

            //----------------- EDIT3DS -----------------
            // Description: 3D Editor chunk, objects layout info
            // Chunk ID: 3d3d (hex)
            // Chunk Length: 0 + sub chunks
            //-------------------------------------------
            case 0x3d3d:
                break;

            //--------------- EDIT_OBJECT ---------------
            // Description: Object block, info for each object
            // Chunk ID: 4000 (hex)
            // Chunk Length: len(object name) + sub chunks
            //-------------------------------------------
            case 0x4000:
                {
                std::stringstream ss;
                unsigned char c = 1;

                for (int i = 0; i < 20 && c != '\0'; i++) {
                    read_or_fail(input_file, &c, 1);
                    ss << c;
                }

                objects_.push_back(Object(ss.str()));
                object = &objects_.back();
                }
                break;

            //--------------- OBJ_TRIMESH ---------------
            // Description: Triangular mesh, contains chunks for 3d mesh info
            // Chunk ID: 4100 (hex)
            // Chunk Length: 0 + sub chunks
            //-------------------------------------------
            case 0x4100:
                break;

            //--------------- TRI_VERTEXL ---------------
            // Description: Vertices list
            // Chunk ID: 4110 (hex)
            // Chunk Length: 1 x unsigned short (number of vertices)
            //             + 3 x float (vertex coordinates) x (number of vertices)
            //             + sub chunks
            //-------------------------------------------
            case 0x4110:
                {
                uint16_t qty;
                read_or_fail(input_file, &qty, sizeof(uint16_t));
                object->vertices.resize(qty);

                for (uint16_t i = 0; i < qty; i++) {
                    float f[3];
                    read_or_fail(input_file, f, sizeof(float) * 3);
                    vec3& vertex = object->vertices[i].v;
                    vertex.x(f[0]);
                    vertex.y(f[1]);
                    vertex.z(f[2]);
                }
                }
                break;

            //--------------- TRI_FACEL1 ----------------
            // Description: Polygons (faces) list
            // Chunk ID: 4120 (hex)
            // Chunk Length: 1 x unsigned short (number of polygons)
            //             + 3 x unsigned short (polygon points) x (number of polygons)
            //             + sub chunks
            //-------------------------------------------
            case 0x4120:
                {
                uint16_t qty;
                read_or_fail(input_file, &qty, sizeof(uint16_t));
                object->faces.resize(qty);
                for (uint16_t i = 0; i < qty; i++) {
                    uint16_t f[4];
                    read_or_fail(input_file, f, sizeof(uint16_t) * 4);
                    uvec3& face = object->faces[i].v;
                    face.x(f[0]);
                    face.y(f[1]);
                    face.z(f[2]);
                }
                }
                break;

            //------------- TRI_MAPPINGCOORS ------------
            // Description: Vertices list
            // Chunk ID: 4140 (hex)
            // Chunk Length: 1 x unsigned short (number of mapping points)
            //             + 2 x float (mapping coordinates) x (number of mapping points)
            //             + sub chunks
            //-------------------------------------------
            case 0x4140:
                {
                uint16_t qty;
                read_or_fail(input_file, &qty, sizeof(uint16_t));
                for (uint16_t i = 0; i < qty; i++) {
                    float f[2];
                    read_or_fail(input_file, f, sizeof(float) * 2);
                    vec2& texcoord = object->vertices[i].t;
                    texcoord.x(f[0]);
                    texcoord.y(f[1]);
                }
                }
                gotTexcoords_ = true;
                break;

            //----------- Skip unknow chunks ------------
            //We need to skip all the chunks that currently we don't use
            //We use the chunk length information to set the file pointer
            //to the same level next chunk
            //-------------------------------------------
            default:
                input_file.seekg(chunk_length - 6, std::ios::cur);
        }
    }

    // Compute bounding box for perspective projection
    compute_bounding_box(*object);

    if (Options::show_debug) {
        for (std::vector<Object>::const_iterator iter = objects_.begin();
             iter != objects_.end();
             iter++)
        {
            Log::debug("    Object name: %s Vertex count: %d Face count: %d\n",
                       iter->name.c_str(), iter->vertices.size(), iter->faces.size());
        }
    }

    return true;
}


const unsigned int Model::Face::OBJ_FACE_V = 0x1;
const unsigned int Model::Face::OBJ_FACE_T = 0x2;
const unsigned int Model::Face::OBJ_FACE_N = 0x4;

/**
 * Parse 2-element vertex attribute from an OBJ file.
 *
 * @param source the source line to parse
 * @param v the vec2 to populate
 */
void
Model::obj_get_attrib(const string& source, vec2& v)
{
    // Our attribs are whitespace separated, so use a fuzzy split.
    vector<string> elements;
    Util::split(source, ' ', elements, Util::SplitModeFuzzy);

    // Find the first value...
    float x = Util::fromString<float>(elements[0]);
    // And the second value (there might be a third, but we don't care)...
    float y = Util::fromString<float>(elements[1]);
    v.x(x);
    v.y(y);
}

/**
 * Parse 3-element vertex attribute from an OBJ file.
 *
 * @param source the source line to parse
 * @param v the vec3 to populate
 */
void
Model::obj_get_attrib(const string& source, vec3& v)
{
    // Our attribs are whitespace separated, so use a fuzzy split.
    vector<string> elements;
    Util::split(source, ' ', elements, Util::SplitModeFuzzy);

    // Find the first value...
    float x = Util::fromString<float>(elements[0]);
    // Then the second value...
    float y = Util::fromString<float>(elements[1]);
    // And the third value (there might be a fourth, but we don't care)...
    float z = Util::fromString<float>(elements[2]);
    v.x(x);
    v.y(y);
    v.z(z);
}


void
Model::obj_face_get_index(const string& tuple, unsigned int& which,
    unsigned int& v, unsigned int& t, unsigned int& n)
{
    // We can use a normal split here as syntax requires no spaces around
    // the '/' delimiter for a face description.
    vector<string> elements;
    Util::split(tuple, '/', elements, Util::SplitModeNormal);

    if (elements.empty())
    {
        which = 0;
        return;
    }

    which = Face::OBJ_FACE_V;
    v = Util::fromString<unsigned int>(elements[0]);

    unsigned int num_elements = elements.size();

    if (num_elements > 1 && !elements[1].empty())
    {
        which |= Face::OBJ_FACE_T;
        t = Util::fromString<unsigned int>(elements[1]);
    }

    if (num_elements > 2 && !elements[2].empty())
    {
        which |= Face::OBJ_FACE_N;
        n = Util::fromString<unsigned int>(elements[2]);
    }

    return;
}

/**
 * Parse a face description from an OBJ file.
 * Faces always specify position, but optionally can also contain separate
 * indices for texcoords and normals.
 *
 * @param source the source line to parse
 * @param v the uvec3 to populate
 */
void
Model::obj_get_face(const string& source, Face& f)
{
    // Our indices are whitespace separated, so use a fuzzy split.
    vector<string> elements;
    Util::split(source, ' ', elements, Util::SplitModeFuzzy);

    // Find the first value...
    unsigned int which(0);
    unsigned int vx(0);
    unsigned int tx(0);
    unsigned int nx(0);
    obj_face_get_index(elements[0], which, vx, tx, nx);

    // Then the second value...
    unsigned int vy(0);
    unsigned int ty(0);
    unsigned int ny(0);
    obj_face_get_index(elements[1], which, vy, ty, ny);

    // And the third value (there might be a fourth, but we don't care)...
    unsigned int vz(0);
    unsigned int tz(0);
    unsigned int nz(0);
    obj_face_get_index(elements[2], which, vz, tz, nz);

    // OBJ models start absoluted indices at '1', so subtract to re-base to
    // '0'.  We do not handle relative indexing (negative indices).  
    f.which = which;
    f.v.x(vx - 1);
    f.v.y(vy - 1);
    f.v.z(vz - 1);
    if (which & Face::OBJ_FACE_T)
    {
        f.t.x(tx - 1);
        f.t.y(ty - 1);
        f.t.z(tz - 1);
    }
    if (which & Face::OBJ_FACE_N)
    {
        f.n.x(nx - 1);
        f.n.y(ny - 1);
        f.n.z(nz - 1);
    }
}

/**
 * Load a model from an OBJ file.
 *
 * @param filename the name of the file
 *
 * @return whether loading succeeded
 */
bool
Model::load_obj(const std::string &filename)
{
    Log::debug("Loading model from obj file '%s'\n", filename.c_str());

    const std::unique_ptr<std::istream> input_file_ptr(Util::get_resource(filename));
    std::istream& inputFile(*input_file_ptr);
    if (!inputFile)
    {
        Log::error("Failed to open '%s'\n", filename.c_str());
        return false;
    }

    vector<string> sourceVec;
    string curLine;
    while (getline(inputFile, curLine))
    {
        sourceVec.push_back(curLine);
    }

    // Give ourselves an object to populate.
    objects_.push_back(Object(string()));
    Object& object(objects_.back());

    static const string object_definition("o");
    static const string vertex_definition("v");
    static const string normal_definition("vn");
    static const string texcoord_definition("vt");
    static const string face_definition("f");
    vector<vec3> positions;
    vector<vec3> normals;
    vector<vec2> texcoords;
    for (vector<string>::const_iterator lineIt = sourceVec.begin();
         lineIt != sourceVec.end();
         lineIt++)
    {
        const string& curSrc = *lineIt;
        // Is it a vertex attribute, a face description, comment or other?
        // We only care about the first two, we ignore comments, group names,
        // smoothing groups, etc.
        string::size_type startPos(0);
        string::size_type spacePos = curSrc.find(" ", startPos);
        string::size_type num_chars(string::npos);
        string definition;
        if (spacePos != string::npos)
        {
            // Could be arbitrary whitespace between description type and
            // the data
            string::size_type defPos = curSrc.find_first_not_of(' ', spacePos);
            definition = string(curSrc, defPos);
            num_chars = spacePos - startPos;
        }
        string definitionType(curSrc, startPos, num_chars);

        if (definitionType == vertex_definition)
        {
            vec3 p;
            obj_get_attrib(definition, p);
            positions.push_back(p);
        }
        else if (definitionType == normal_definition)
        {
            vec3 n;
            obj_get_attrib(definition, n);
            normals.push_back(n);
        }
        else if (definitionType == texcoord_definition)
        {
            vec2 t;
            obj_get_attrib(definition, t);
            texcoords.push_back(t);
        }
        else if (definitionType == face_definition)
        {
            Face f;
            obj_get_face(definition, f);
            object.faces.push_back(f);
        }
        else if (definitionType == object_definition)
        {
            object.name = definition;
        }
    }

    if (!texcoords.empty())
    {
        gotTexcoords_ = true;
    }
    if (!normals.empty())
    {
        gotNormals_ = true;
    }
    unsigned int numVertices = positions.size();
    object.vertices.resize(numVertices);
    for (unsigned int i = 0; i < numVertices; i++)
    {
        Vertex& curVertex = object.vertices[i];
        curVertex.v = positions[i];
        if (gotTexcoords_)
        {
            curVertex.t = texcoords[i];
        }
        if (gotNormals_)
        {
            curVertex.n = normals[i];
        }
    }

    // Compute bounding box for perspective projection
    compute_bounding_box(object);

    Log::debug("Object name: %s Vertex count: %u Face count: %u\n",
        object.name.empty() ? "(none)" : object.name.c_str(), object.vertices.size(), object.faces.size());
    return true;
}

namespace ModelPrivate
{
ModelMap modelMap;
}

/**
 * Locate all available models.
 *
 * This method scans the built-in data paths and build a database of usable
 * models available to scenes.  Map is available on a read-only basis to scenes
 * that might find it useful for listing models, etc.
 *
 * @return a map containing information about the located models
 */
const ModelMap&
Model::find_models()
{
    if (!ModelPrivate::modelMap.empty())
    {
        return ModelPrivate::modelMap;
    }
    vector<string> pathVec;
    string dataDir(GLMARK_DATA_PATH"/models");
    Util::list_files(dataDir, pathVec);
#ifdef GLMARK_EXTRAS_PATH
    string extrasDir(GLMARK_EXTRAS_PATH"/models");
    Util::list_files(extrasDir, pathVec);
#endif

    // Now that we have a list of all of the model files available to us,
    // let's go through and pull out the names and what format they're in
    // so the scene can decide which ones to use.
    for(vector<string>::const_iterator pathIt = pathVec.begin();
        pathIt != pathVec.end();
        pathIt++)
    {
        const string& curPath = *pathIt;
        string::size_type namePos(0);
        string::size_type slashPos = curPath.rfind("/");
        if (slashPos != string::npos)
        {
            // Advance to the first character after the last slash
            namePos = slashPos + 1;
        }

        ModelFormat format(MODEL_INVALID);
        string::size_type extPos = curPath.rfind(".3ds");
        if (extPos == string::npos)
        {
            // It's not a 3ds model
            extPos = curPath.rfind(".obj");
            if (extPos == string::npos)
            {
                // It's not an obj model either, so skip it.
                continue;
            }
            format = MODEL_OBJ;
        }
        else
        {
            // It's a 3ds model
            format = MODEL_3DS;
        }

        string name(curPath, namePos, extPos - namePos);
        ModelDescriptor* desc = new ModelDescriptor(name, format, curPath);
        ModelPrivate::modelMap.insert(std::make_pair(name, desc));
    }

    return ModelPrivate::modelMap;
}

/**
 * Load a model by name.
 *
 * You must initialize the available model collection using
 * Model::find_models() before using this method.
 *
 * @param modelName the model name
 *
 * @return whether the operation succeeded
 */
bool
Model::load(const string& modelName)
{
    bool retVal(false);
    ModelMap::const_iterator modelIt = ModelPrivate::modelMap.find(modelName);
    if (modelIt == ModelPrivate::modelMap.end())
    {
        return retVal;
    }

    ModelDescriptor* desc = modelIt->second;
    switch (desc->format())
    {
        case MODEL_INVALID:
            break;
        case MODEL_3DS:
            retVal = load_3ds(desc->pathname());
            break;
        case MODEL_OBJ:
            retVal = load_obj(desc->pathname());
            break;
    }

    return retVal;
}
