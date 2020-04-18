/*
 * Copyright © 2012 Linaro Limited
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
 *  Alexandros Frantzis
 */
#include "renderer.h"

TextureRenderer::TextureRenderer(const LibMatrix::vec2 &size, Program &program) :
    BaseRenderer(size), program_(program)
{
    /* Create the mesh (quad) used for rendering */
    create_mesh();
}

void
TextureRenderer::create_mesh()
{
    // Set vertex format
    std::vector<int> vertex_format;
    vertex_format.push_back(3); // Position
    mesh_.set_vertex_format(vertex_format);

    mesh_.make_grid(1, 1, 2.0, 2.0, 0);
    mesh_.build_vbo();

    program_.start();

    // Set attribute locations
    std::vector<GLint> attrib_locations;
    attrib_locations.push_back(program_["position"].location());
    mesh_.set_attrib_locations(attrib_locations);

    program_.stop();
}

void
TextureRenderer::render()
{
    make_current();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input_texture_);

    program_.start();

    mesh_.render_vbo();

    program_.stop();

    update_mipmap();
}
