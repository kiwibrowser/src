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

NormalFromHeightRenderer::NormalFromHeightRenderer(const LibMatrix::vec2 &size) :
    TextureRenderer(size, *normal_from_height_program(size, true))
{
    normal_from_height_program_ = normal_from_height_program(size, false);
}

Program *
NormalFromHeightRenderer::normal_from_height_program(const LibMatrix::vec2 &size,
                                                     bool create_new)
{
    static Program *normal_from_height_program(0);
    if (create_new)
        normal_from_height_program = 0;

    if (!normal_from_height_program) {
        normal_from_height_program = new Program();
        ShaderSource vtx_shader(GLMARK_DATA_PATH"/shaders/terrain-texture.vert");
        ShaderSource frg_shader(GLMARK_DATA_PATH"/shaders/terrain-normalmap.frag");

        Scene::load_shaders_from_strings(*normal_from_height_program,
                                         vtx_shader.str(), frg_shader.str());

        normal_from_height_program->start();
        (*normal_from_height_program)["heightMap"] = 0;
        (*normal_from_height_program)["resolution"] = size;
        (*normal_from_height_program)["height"] = 0.05f;
        (*normal_from_height_program)["uvOffset"] = LibMatrix::vec2(0.0f, 0.0f);
        (*normal_from_height_program)["uvScale"] = LibMatrix::vec2(1.0f, 1.0f);
        normal_from_height_program->stop();
    }

    return normal_from_height_program;
}

