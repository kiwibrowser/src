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

LibMatrix::vec2 SimplexNoiseRenderer::uv_scale_(1.5f, 1.5);

SimplexNoiseRenderer::SimplexNoiseRenderer(const LibMatrix::vec2 &size) :
    TextureRenderer(size, *noise_program(true))
{
    noise_program_ = noise_program(false);
}

Program *
SimplexNoiseRenderer::noise_program(bool create_new)
{
    static Program *noise_program(0);
    if (create_new)
        noise_program = 0;

    if (!noise_program) {
        noise_program = new Program();
        ShaderSource vtx_shader(GLMARK_DATA_PATH"/shaders/terrain-texture.vert");
        ShaderSource frg_shader(GLMARK_DATA_PATH"/shaders/terrain-noise.frag");

        Scene::load_shaders_from_strings(*noise_program, vtx_shader.str(), frg_shader.str());

        noise_program->start();
        (*noise_program)["time"] = 1.0f;
        (*noise_program)["uvOffset"] = LibMatrix::vec2(0.0f, 0.0f);
        (*noise_program)["uvScale"] = uv_scale_;
        noise_program->stop();
    }

    return noise_program;
}
