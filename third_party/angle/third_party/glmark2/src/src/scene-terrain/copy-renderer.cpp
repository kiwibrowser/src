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

CopyRenderer::CopyRenderer(const LibMatrix::vec2 &size) :
    TextureRenderer(size, *copy_program(true))
{
    copy_program_ = copy_program(false);
}

Program *
CopyRenderer::copy_program(bool create_new)
{
    static Program *copy_program(0);
    if (create_new)
        copy_program = 0;

    if (!copy_program) {
        copy_program = new Program();
        ShaderSource vtx_shader(GLMARK_DATA_PATH"/shaders/terrain-texture.vert");
        ShaderSource frg_shader(GLMARK_DATA_PATH"/shaders/terrain-overlay.frag");

        Scene::load_shaders_from_strings(*copy_program, vtx_shader.str(), frg_shader.str());

        copy_program->start();
        (*copy_program)["tDiffuse"] = 0;
        (*copy_program)["opacity"] = 1.0f;
        (*copy_program)["uvOffset"] = LibMatrix::vec2(0.0f, 0.0f);
        (*copy_program)["uvScale"] = LibMatrix::vec2(1.0f, 1.0f);
        copy_program->stop();
    }

    return copy_program;
}
