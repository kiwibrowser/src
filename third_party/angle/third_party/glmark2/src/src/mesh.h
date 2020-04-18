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
 *  Jesse Barker (glmark2)
 */
#ifndef GLMARK2_MESH_H_
#define GLMARK2_MESH_H_

#include <vector>
#include "vec.h"
#include "gl-headers.h"

/**
 * A mesh of vertices.
 */
class Mesh
{
public:
    Mesh();
    ~Mesh();

    void set_vertex_format(const std::vector<int> &format);
    void set_attrib_locations(const std::vector<int> &locations);

    void set_attrib(unsigned int pos, const LibMatrix::vec2 &v, std::vector<float> *vertex = 0);
    void set_attrib(unsigned int pos, const LibMatrix::vec3 &v, std::vector<float> *vertex = 0);
    void set_attrib(unsigned int pos, const LibMatrix::vec4 &v, std::vector<float> *vertex = 0);
    void next_vertex();
    std::vector<std::vector<float> >& vertices();

    enum VBOUpdateMethod {
        VBOUpdateMethodMap,
        VBOUpdateMethodSubData,
    };
    enum VBOUsage {
        VBOUsageStatic,
        VBOUsageStream,
        VBOUsageDynamic,
    };

    void vbo_update_method(VBOUpdateMethod method);
    void vbo_usage(VBOUsage usage);
    void interleave(bool interleave);

    void reset();
    void build_array();
    void build_vbo();
    void update_array(const std::vector<std::pair<size_t, size_t> >& ranges);
    void update_vbo(const std::vector<std::pair<size_t, size_t> >& ranges);
    void delete_array();
    void delete_vbo();

    void render_array();
    void render_vbo();

    typedef void (*grid_configuration_func)(Mesh &mesh, int x, int y, int n_x, int n_y,
                                            LibMatrix::vec3 &ul,
                                            LibMatrix::vec3 &ll,
                                            LibMatrix::vec3 &ur,
                                            LibMatrix::vec3 &lr);

    void make_grid(int n_x, int n_y, double width, double height,
                   double spacing, grid_configuration_func conf_func = 0);

private:
    bool check_attrib(unsigned int pos, int dim);
    std::vector<float> &ensure_vertex();
    void update_single_array(const std::vector<std::pair<size_t, size_t> >& ranges,
                             size_t n, size_t nfloats, size_t offset);
    void update_single_vbo(const std::vector<std::pair<size_t, size_t> >& ranges,
                           size_t n, size_t nfloats);

    //
    // vertex_format_ is a vector of pairs describing the attribute data.
    // pair.first is the dimension of the attribute (vec2, vec3, vec4) and
    // pair.second is the number of float values into the vertex data of that
    // attribute.  So, for example, if there are 3 attributes, a vec3 position
    // a vec3 normal and a vec2 texture coordinate, you would have a
    // vertex_format_ with 3 elements as follows: {<3, 0>, <3, 3>, <2, 6>} and
    // the total size of each vertex would be the sum of the dimensions times
    // the size of a float, or: 8 * sizeof(float) == 32 bytes
    //
    std::vector<std::pair<int, int> > vertex_format_;
    std::vector<int> attrib_locations_;
    int vertex_size_;

    std::vector<std::vector<float> > vertices_;

    std::vector<float *> vertex_arrays_;
    std::vector<GLuint> vbos_;
    std::vector<float *> attrib_data_ptr_;
    int vertex_stride_;
    bool interleave_;
    VBOUpdateMethod vbo_update_method_;
    VBOUsage vbo_usage_;
};

#endif
