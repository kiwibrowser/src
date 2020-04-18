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
#ifndef GLMARK2_MODEL_H_
#define GLMARK2_MODEL_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include "vec.h"

// Forward declare the mesh object.  We don't need the whole header here.
class Mesh;

enum ModelFormat
{
    MODEL_INVALID,
    MODEL_3DS,
    MODEL_OBJ
};

/**
 * A descriptor for a model file.
 */
class ModelDescriptor
{
    std::string name_;
    std::string pathname_;
    ModelFormat format_;
    ModelDescriptor();
public:
    ModelDescriptor(const std::string& name, ModelFormat format,
                    const std::string& pathname) :
        name_(name),
        pathname_(pathname),
        format_(format) {}
    ~ModelDescriptor() {}
    const std::string& pathname() const { return pathname_; }
    ModelFormat format() const { return format_; }
};

typedef std::map<std::string, ModelDescriptor*> ModelMap;

/**
 * A model as loaded from a 3D object data file.
 */
class Model
{
public:

    typedef enum {
        AttribTypePosition = 1,
        AttribTypeNormal,
        AttribTypeTexcoord,
        AttribTypeTangent,
        AttribTypeBitangent,
        AttribTypeCustom
    } AttribType;

    Model() : gotTexcoords_(false), gotNormals_(false) {}
    ~Model() {}

    bool load(const std::string& name);

    bool needTexcoords() const { return !gotTexcoords_; }
    bool needNormals() const { return !gotNormals_; }
    void calculate_texcoords();
    void calculate_normals();
    void convert_to_mesh(Mesh &mesh);
    void convert_to_mesh(Mesh &mesh,
                         const std::vector<std::pair<AttribType, int> > &attribs);
    const LibMatrix::vec3& minVec() const { return minVec_; }
    const LibMatrix::vec3& maxVec() const { return maxVec_; }
    static const ModelMap& find_models();
private:
    // If the model we loaded contained texcoord or normal data...
    bool gotTexcoords_;
    bool gotNormals_;

    struct Face {
        LibMatrix::uvec3 v;
        LibMatrix::uvec3 n;
        LibMatrix::uvec3 t;
        unsigned int which; // mask of which values are relevant for this face.
        static const unsigned int OBJ_FACE_V;
        static const unsigned int OBJ_FACE_T;
        static const unsigned int OBJ_FACE_N;
    };

    struct Vertex {
        LibMatrix::vec3 v;
        LibMatrix::vec3 n;
        LibMatrix::vec2 t;
        LibMatrix::vec3 nt;
        LibMatrix::vec3 nb;
    };

    struct Object {
        Object(const std::string &name) : name(name) {}
        std::string name;
        std::vector<Vertex> vertices;
        std::vector<Face> faces;
    };

    void append_object_to_mesh(const Object &object, Mesh &mesh,
                               int p_pos, int n_pos, int t_pos,
                               int nt_pos, int nb_pos);
    bool load_3ds(const std::string &filename);
    bool load_obj(const std::string &filename);
    void obj_get_attrib(const std::string& description, LibMatrix::vec2& v);
    void obj_get_attrib(const std::string& description, LibMatrix::vec3& v);
    void obj_face_get_index(const std::string& tuple, unsigned int& which,
        unsigned int& v, unsigned int& t, unsigned int& n);
    void obj_get_face(const std::string& description, Face& face);

    // For vertices of the bounding box for this model.
    void compute_bounding_box(const Object& object);
    LibMatrix::vec3 minVec_;
    LibMatrix::vec3 maxVec_;
    std::vector<Object> objects_;
};

#endif
