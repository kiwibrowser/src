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
#include "scene.h"
#include "renderer.h"
#include "shader-source.h"

OverlayRenderer::OverlayRenderer(IRenderer &target, GLfloat opacity) :
    target_renderer_(target), opacity_(opacity)

{
    create_program();
    create_mesh();
}


void
OverlayRenderer::setup(const LibMatrix::vec2 &size, bool onscreen, bool has_depth)
{
    static_cast<void>(size);
    static_cast<void>(onscreen);
    static_cast<void>(has_depth);
}

void
OverlayRenderer::setup_texture(GLint min_filter, GLint mag_filter,
                               GLint wrap_s, GLint wrap_t)
{
    static_cast<void>(min_filter);
    static_cast<void>(mag_filter);
    static_cast<void>(wrap_s);
    static_cast<void>(wrap_t);
}

void
OverlayRenderer::make_current()
{
    target_renderer_.make_current();
}

void
OverlayRenderer::update_mipmap()
{
    target_renderer_.update_mipmap();
}

void
OverlayRenderer::render()
{
    target_renderer_.make_current();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input_texture_);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    program_.start();

    mesh_.render_vbo();

    program_.stop();

    glDisable(GL_BLEND);

    target_renderer_.update_mipmap();
}

void
OverlayRenderer::create_mesh()
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
OverlayRenderer::create_program()
{
    ShaderSource vtx_shader(GLMARK_DATA_PATH"/shaders/terrain-texture.vert");
    ShaderSource frg_shader(GLMARK_DATA_PATH"/shaders/terrain-overlay.frag");

    if (!Scene::load_shaders_from_strings(program_, vtx_shader.str(), frg_shader.str()))
        return;

    program_.start();
    program_["tDiffuse"] = 0;
    program_["opacity"] = opacity_;
    program_["uvOffset"] = LibMatrix::vec2(0.0f, 0.0f);
    program_["uvScale"] = LibMatrix::vec2(1.0f, 1.0f);
    program_.stop();
}
