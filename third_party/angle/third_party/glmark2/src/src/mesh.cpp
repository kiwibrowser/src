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
#include "log.h"
#include "gl-headers.h"


Mesh::Mesh() :
    vertex_size_(0), interleave_(false), vbo_update_method_(VBOUpdateMethodMap),
    vbo_usage_(VBOUsageStatic)
{
}

Mesh::~Mesh()
{
    reset();
}

/*
 * Sets the vertex format for this mesh.
 *
 * The format consists of a vector of integers, each
 * specifying the size in floats of each vertex attribute.
 *
 * e.g. {4, 3, 2} => 3 attributes vec4, vec3, vec2
 */
void
Mesh::set_vertex_format(const std::vector<int> &format)
{
    int pos = 0;
    vertex_format_.clear();

    for (std::vector<int>::const_iterator iter = format.begin();
         iter != format.end();
         iter++)
    {
        int n = *iter;
        vertex_format_.push_back(std::pair<int,int>(n, pos));

        pos += n;
    }

    vertex_size_ = pos;
}

/*
 * Sets the attribute locations.
 *
 * These are the locations used in glEnableVertexAttribArray()
 * and other related functions.
 */
void
Mesh::set_attrib_locations(const std::vector<int> &locations)
{
    if (locations.size() != vertex_format_.size())
        Log::error("Trying to set attribute locations using wrong size\n");
    attrib_locations_ = locations;
}


/**
 * Checks that an attribute is of the correct dimensionality.
 *
 * @param pos the position/index of the attribute to check
 * @param dim the size of the attribute (in #floats)
 *
 * @return whether the check succeeded
 */
bool
Mesh::check_attrib(unsigned int pos, int dim)
{
    if (pos > vertex_format_.size()) {
        Log::error("Trying to set non-existent attribute\n");
        return false;
    }

    if (vertex_format_[pos].first != dim) {
        Log::error("Trying to set attribute with value of invalid type\n");
        return false;
    }

    return true;
}


/**
 * Ensures that we have a vertex to process.
 *
 * @return the vertex to process
 */
std::vector<float> &
Mesh::ensure_vertex()
{
    if (vertices_.empty())
        next_vertex();

    return vertices_.back();
}

/*
 * Sets the value of an attribute in the current vertex.
 *
 * The pos parameter refers to the position of the attribute
 * as specified indirectly when setting the format using
 * set_vertex_format(). e.g. 0 = first attribute, 1 = second
 * etc
 */
void
Mesh::set_attrib(unsigned int pos, const LibMatrix::vec2 &v, std::vector<float> *vertex)
{
    if (!check_attrib(pos, 2))
        return;

    std::vector<float> &vtx = !vertex ? ensure_vertex() : *vertex;

    int offset = vertex_format_[pos].second;

    vtx[offset] = v.x();
    vtx[offset + 1] = v.y();
}

void
Mesh::set_attrib(unsigned int pos, const LibMatrix::vec3 &v, std::vector<float> *vertex)
{
    if (!check_attrib(pos, 3))
        return;

    std::vector<float> &vtx = !vertex ? ensure_vertex() : *vertex;

    int offset = vertex_format_[pos].second;

    vtx[offset] = v.x();
    vtx[offset + 1] = v.y();
    vtx[offset + 2] = v.z();
}

void
Mesh::set_attrib(unsigned int pos, const LibMatrix::vec4 &v, std::vector<float> *vertex)
{
    if (!check_attrib(pos, 4))
        return;

    std::vector<float> &vtx = !vertex ? ensure_vertex() : *vertex;

    int offset = vertex_format_[pos].second;

    vtx[offset] = v.x();
    vtx[offset + 1] = v.y();
    vtx[offset + 2] = v.z();
    vtx[offset + 3] = v.w();
}

/*
 * Adds a new vertex to the list and makes it current.
 */
void
Mesh::next_vertex()
{
    vertices_.push_back(std::vector<float>(vertex_size_));
}

/**
 * Gets the mesh vertices.
 *
 * You should use the ::set_attrib() method to manipulate
 * the vertex data.
 *
 * You shouldn't resize the vector (change the number of vertices)
 * manually. Use ::next_vertex() instead.
 */
std::vector<std::vector<float> >&
Mesh::vertices()
{
    return vertices_;
}

/**
 * Sets the VBO update method.
 *
 * The default value is VBOUpdateMethodMap.
 */
void
Mesh::vbo_update_method(Mesh::VBOUpdateMethod method)
{
    vbo_update_method_ = method;
}

/**
 * Sets the VBO usage hint.
 *
 * The usage hint takes effect in the next call to ::build_vbo().
 *
 * The default value is VBOUsageStatic.
 */
void
Mesh::vbo_usage(Mesh::VBOUsage usage)
{
    vbo_usage_ = usage;
}

/**
 * Sets the vertex attribute interleaving mode.
 *
 * If true the vertex attributes are going to be interleaved in a single
 * buffer. Otherwise they will be separated into different buffers (one
 * per attribute).
 *
 * Interleaving mode takes effect in the next call to ::build_array() or
 * ::build_vbo().
 *
 * @param interleave whether to interleave
 */
void
Mesh::interleave(bool interleave)
{
    interleave_ = interleave;
}

/**
 * Resets a Mesh object to its initial, empty state.
 */
void
Mesh::reset()
{
    delete_array();
    delete_vbo();

    vertices_.clear();
    vertex_format_.clear();
    attrib_locations_.clear();
    attrib_data_ptr_.clear();
    vertex_size_ = 0;
    vertex_stride_ = 0;
}

/**
 * Builds a vertex array containing the mesh vertex data.
 *
 * The way the vertex array is constructed is affected by the current
 * interleave value, which can set using ::interleave().
 */
void
Mesh::build_array()
{
    int nvertices = vertices_.size();

    if (!interleave_) {
        /* Create an array for each attribute */
        for (std::vector<std::pair<int, int> >::const_iterator ai = vertex_format_.begin();
             ai != vertex_format_.end();
             ai++)
        {
            float *array = new float[nvertices * ai->first];
            float *cur = array;

            /* Fill in the array */
            for (std::vector<std::vector<float> >::const_iterator vi = vertices_.begin();
                    vi != vertices_.end();
                    vi++)
            {
                for (int i = 0; i < ai->first; i++)
                    *cur++ = (*vi)[ai->second + i];
            }

            vertex_arrays_.push_back(array);
            attrib_data_ptr_.push_back(array);
        }
        vertex_stride_ = 0;
    }
    else {
        float *array = new float[nvertices * vertex_size_];
        float *cur = array;

        for (std::vector<std::vector<float> >::const_iterator vi = vertices_.begin();
             vi != vertices_.end();
             vi++)
        {
            /* Fill in the array */
            for (int i = 0; i < vertex_size_; i++)
                *cur++ = (*vi)[i];
        }

        for (size_t i = 0; i < vertex_format_.size(); i++)
            attrib_data_ptr_.push_back(array + vertex_format_[i].second);

        vertex_arrays_.push_back(array);
        vertex_stride_ = vertex_size_ * sizeof(float);
    }
}

/**
 * Builds a vertex buffer object containing the mesh vertex data.
 *
 * The way the VBO is constructed is affected by the current interleave
 * value (::interleave()) and the vbo usage hint (::vbo_usage()).
 */
void
Mesh::build_vbo()
{
    delete_array();
    build_array();

    int nvertices = vertices_.size();

    attrib_data_ptr_.clear();

    GLenum buffer_usage;
    if (vbo_usage_ == Mesh::VBOUsageStream)
        buffer_usage = GL_STREAM_DRAW;
    else if (vbo_usage_ == Mesh::VBOUsageDynamic)
        buffer_usage = GL_DYNAMIC_DRAW;
    else /* if (vbo_usage_ == Mesh::VBOUsageStatic) */
        buffer_usage = GL_STATIC_DRAW;

    if (!interleave_) {
        /* Create a vbo for each attribute */
        for (std::vector<std::pair<int, int> >::const_iterator ai = vertex_format_.begin();
             ai != vertex_format_.end();
             ai++)
        {
            float *data = vertex_arrays_[ai - vertex_format_.begin()];
            GLuint vbo;

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, nvertices * ai->first * sizeof(float),
                         data, buffer_usage);

            vbos_.push_back(vbo);
            attrib_data_ptr_.push_back(0);
        }

        vertex_stride_ = 0;
    }
    else {
        GLuint vbo;
        /* Create a single vbo to store all attribute data */
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, nvertices * vertex_size_ * sizeof(float),
                     vertex_arrays_[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        for (size_t i = 0; i < vertex_format_.size(); i++) {
            attrib_data_ptr_.push_back(reinterpret_cast<float *>(sizeof(float) * vertex_format_[i].second));
            vbos_.push_back(vbo);
        }
        vertex_stride_ = vertex_size_ * sizeof(float);
    }

    delete_array();
}

/**
 * Updates ranges of a single vertex array.
 *
 * @param ranges the ranges of vertices to update
 * @param n the index of the vertex array to update
 * @param nfloats how many floats to update for each vertex
 * @param offset the offset (in floats) in the vertex data to start reading from
 */
void
Mesh::update_single_array(const std::vector<std::pair<size_t, size_t> >& ranges,
                          size_t n, size_t nfloats, size_t offset)
{
    float *array(vertex_arrays_[n]);

    /* Update supplied ranges */
    for (std::vector<std::pair<size_t, size_t> >::const_iterator ri = ranges.begin();
         ri != ranges.end();
         ri++)
    {
        /* Update the current range from the vertex data */
        float *dest(array + nfloats * ri->first);
        for (size_t n = ri->first; n <= ri->second; n++) {
            float *src(vertices_[n].data() + offset);
            std::copy(src, src + nfloats, dest);
            dest += nfloats;
        }

    }
}

/**
 * Updates ranges of the vertex arrays.
 *
 * @param ranges the ranges of vertices to update
 */
void
Mesh::update_array(const std::vector<std::pair<size_t, size_t> >& ranges)
{
    /* If we don't have arrays to update, create them */
    if (vertex_arrays_.empty()) {
        build_array();
        return;
    }

    if (!interleave_) {
        for (size_t i = 0; i < vertex_arrays_.size(); i++) {
            update_single_array(ranges, i, vertex_format_[i].first,
                                vertex_format_[i].second);
        }
    }
    else {
        update_single_array(ranges, 0, vertex_size_, 0);
    }

}


/**
 * Updates ranges of a single VBO.
 *
 * This method use either glMapBuffer or glBufferSubData to perform
 * the update. The used method can be set with ::vbo_update_method().
 *
 * @param ranges the ranges of vertices to update
 * @param n the index of the vbo to update
 * @param nfloats how many floats to update for each vertex
 */
void
Mesh::update_single_vbo(const std::vector<std::pair<size_t, size_t> >& ranges,
                        size_t n, size_t nfloats)
{
    float *src_start(vertex_arrays_[n]);
    float *dest_start(0);

    glBindBuffer(GL_ARRAY_BUFFER, vbos_[n]);

    if (vbo_update_method_ == VBOUpdateMethodMap) {
        dest_start = reinterpret_cast<float *>(
                GLExtensions::MapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)
                );
    }

    /* Update supplied ranges */
    for (std::vector<std::pair<size_t, size_t> >::const_iterator iter = ranges.begin();
         iter != ranges.end();
         iter++)
    {
        float *src(src_start + nfloats * iter->first);
        float *src_end(src_start + nfloats * (iter->second + 1));

        if (vbo_update_method_ == VBOUpdateMethodMap) {
            float *dest(dest_start + nfloats * iter->first);
            std::copy(src, src_end, dest);
        }
        else if (vbo_update_method_ == VBOUpdateMethodSubData) {
            glBufferSubData(GL_ARRAY_BUFFER, nfloats * iter->first * sizeof(float),
                            (src_end - src) * sizeof(float), src);
        }
    }

    if (vbo_update_method_ == VBOUpdateMethodMap)
        GLExtensions::UnmapBuffer(GL_ARRAY_BUFFER);
}

/**
 * Updates ranges of the VBOs.
 *
 * @param ranges the ranges of vertices to update
 */
void
Mesh::update_vbo(const std::vector<std::pair<size_t, size_t> >& ranges)
{
    /* If we don't have VBOs to update, create them */
    if (vbos_.empty()) {
        build_vbo();
        return;
    }

    update_array(ranges);

    if (!interleave_) {
        for (size_t i = 0; i < vbos_.size(); i++)
            update_single_vbo(ranges, i, vertex_format_[i].first);
    }
    else {
        update_single_vbo(ranges, 0, vertex_size_);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


/**
 * Deletes all resources associated with built vertex arrays.
 */
void
Mesh::delete_array()
{
    for (size_t i = 0; i < vertex_arrays_.size(); i++) {
        delete [] vertex_arrays_[i];
    }

    vertex_arrays_.clear();
}

/**
 * Deletes all resources associated with built VBOs.
 */
void
Mesh::delete_vbo()
{
    for (size_t i = 0; i < vbos_.size(); i++) {
        GLuint vbo = vbos_[i];
        glDeleteBuffers(1, &vbo);
    }

    vbos_.clear();
}


/**
 * Renders a mesh using vertex arrays.
 *
 * The vertex arrays must have been previously initialized using
 * ::build_array().
 */
void
Mesh::render_array()
{
    for (size_t i = 0; i < vertex_format_.size(); i++) {
        if (attrib_locations_[i] < 0)
            continue;
        glEnableVertexAttribArray(attrib_locations_[i]);
        glVertexAttribPointer(attrib_locations_[i], vertex_format_[i].first,
                              GL_FLOAT, GL_FALSE, vertex_stride_,
                              attrib_data_ptr_[i]);
    }

    glDrawArrays(GL_TRIANGLES, 0, vertices_.size());

    for (size_t i = 0; i < vertex_format_.size(); i++) {
        if (attrib_locations_[i] < 0)
            continue;
        glDisableVertexAttribArray(attrib_locations_[i]);
    }
}

/**
 * Renders a mesh using vertex buffer objects.
 *
 * The vertex buffer objects must have been previously initialized using
 * ::build_vbo().
 */
void
Mesh::render_vbo()
{
    for (size_t i = 0; i < vertex_format_.size(); i++) {
        if (attrib_locations_[i] < 0)
            continue;
        glEnableVertexAttribArray(attrib_locations_[i]);
        glBindBuffer(GL_ARRAY_BUFFER, vbos_[i]);
        glVertexAttribPointer(attrib_locations_[i], vertex_format_[i].first,
                              GL_FLOAT, GL_FALSE, vertex_stride_,
                              attrib_data_ptr_[i]);
    }

    glDrawArrays(GL_TRIANGLES, 0, vertices_.size());

    for (size_t i = 0; i < vertex_format_.size(); i++) {
        if (attrib_locations_[i] < 0)
            continue;
        glDisableVertexAttribArray(attrib_locations_[i]);
    }
}

/**
 * Creates a grid mesh.
 *
 * @param n_x the number of grid cells on the X axis
 * @param n_y the number of grid cells on the Y axis
 * @param width the width X of the grid (normalized)
 * @param height the height Y of the grid (normalized)
 * @param spacing the spacing between cells (normalized)
 * @param conf_func a function to call to configure the grid (or NULL)
 */
void
Mesh::make_grid(int n_x, int n_y, double width, double height,
                double spacing, grid_configuration_func conf_func)
{
    double side_width = (width - (n_x - 1) * spacing) / n_x;
    double side_height = (height - (n_y - 1) * spacing) / n_y;

    for (int i = 0; i < n_x; i++) {
        for (int j = 0; j < n_y; j++) {
            LibMatrix::vec3 a(-width / 2 + i * (side_width + spacing),
                              height / 2 - j * (side_height + spacing), 0);
            LibMatrix::vec3 b(a.x(), a.y() - side_height, 0);
            LibMatrix::vec3 c(a.x() + side_width, a.y(), 0);
            LibMatrix::vec3 d(a.x() + side_width, a.y() - side_height, 0);

            if (!conf_func) {
                std::vector<float> ul(vertex_size_);
                std::vector<float> ur(vertex_size_);
                std::vector<float> ll(vertex_size_);
                std::vector<float> lr(vertex_size_);

                set_attrib(0, a, &ul);
                set_attrib(0, c, &ur);
                set_attrib(0, b, &ll);
                set_attrib(0, d, &lr);

                next_vertex(); vertices_.back() = ul;
                next_vertex(); vertices_.back() = ll;
                next_vertex(); vertices_.back() = ur;
                next_vertex(); vertices_.back() = ll;
                next_vertex(); vertices_.back() = lr;
                next_vertex(); vertices_.back() = ur;
            }
            else {
                conf_func(*this, i, j, n_x, n_y, a, b, c, d);
            }
        }
    }
}
