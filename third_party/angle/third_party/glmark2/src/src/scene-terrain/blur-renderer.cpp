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

#include <cstdlib>

void
create_blur_shaders(ShaderSource& vtx_source, ShaderSource& frg_source,
                    unsigned int radius, float sigma, BlurRenderer::BlurDirection direction,
                    float tilt_shift);

BlurRenderer::BlurRenderer(const LibMatrix::vec2 &size, int radius, float sigma,
                           BlurDirection dir, const LibMatrix::vec2 &step, float tilt_shift) :
        TextureRenderer(size, *blur_program(true, radius, sigma, dir, step, tilt_shift))
{
    blur_program_ = blur_program(false, radius, sigma, dir, step, tilt_shift);
}

Program *
BlurRenderer::blur_program(bool create_new, int radius, float sigma,
                           BlurDirection dir, const LibMatrix::vec2 &step,
                           float tilt_shift)
{
    static Program *blur_program(0);
    if (create_new)
        blur_program = 0;

    if (!blur_program) {
        blur_program = new Program();
        ShaderSource blur_vtx_shader;
        ShaderSource blur_frg_shader;
        create_blur_shaders(blur_vtx_shader, blur_frg_shader, radius,
                            sigma, dir, tilt_shift);

        if (dir == BlurDirectionHorizontal || dir == BlurDirectionBoth)
            blur_frg_shader.add_const("TextureStepX", step.x());
        if (dir == BlurDirectionVertical || dir == BlurDirectionBoth)
            blur_frg_shader.add_const("TextureStepY", step.y());

        Scene::load_shaders_from_strings(*blur_program,
                blur_vtx_shader.str(),
                blur_frg_shader.str());
        blur_program->start();
        (*blur_program)["Texture0"] = 0;
        (*blur_program)["uvOffset"] = LibMatrix::vec2(0.0f, 0.0f);
        (*blur_program)["uvScale"] = LibMatrix::vec2(1.0f, 1.0f);
        blur_program->stop();
    }

    return blur_program;
}

void
create_blur_shaders(ShaderSource& vtx_source, ShaderSource& frg_source,
                    unsigned int radius, float sigma, BlurRenderer::BlurDirection direction,
                    float tilt_shift)
{
    vtx_source.append_file(GLMARK_DATA_PATH"/shaders/terrain-texture.vert");
    frg_source.append_file(GLMARK_DATA_PATH"/shaders/terrain-blur.frag");

    /* Don't let the gaussian curve become too narrow */
    if (sigma < 1.0)
        sigma = 1.0;

    unsigned int side = 2 * radius + 1;
    std::vector<float> values(radius + 1);
    float sum = 0.0;

    for (unsigned int i = 0; i < radius + 1; i++) {
        float s2 = 2.0 * sigma * sigma;
        float k = 1.0 / std::sqrt(M_PI * s2) * std::exp( - (static_cast<float>(i) * i) / s2);
        values[i] = k;
        sum += k;
    }

    sum += sum - values[0];

    for (unsigned int i = 0; i < radius + 1; i++) {
        std::stringstream ss_tmp;
        ss_tmp << "Kernel" << i;
        frg_source.add_const(ss_tmp.str(), values[i] / sum);
    }

    frg_source.add_const("TiltShift", tilt_shift);

    std::stringstream ss;

    if (direction == BlurRenderer::BlurDirectionHorizontal ||
        direction == BlurRenderer::BlurDirectionBoth)
    {
        if (tilt_shift == 1.0)
            ss << "const float stepX = TextureStepX;" << std::endl;
        else
            ss << "float stepX = TextureStepX * abs(TiltShift - TextureCoord.y) / abs(1.0 - TiltShift);" << std::endl;
    }

    if (direction == BlurRenderer::BlurDirectionVertical ||
        direction == BlurRenderer::BlurDirectionBoth)
    {
        if (tilt_shift == 1.0)
            ss << "const float stepY = TextureStepY;" << std::endl;
        else
            ss << "float stepY = TextureStepY * abs(TiltShift - TextureCoord.y) / abs(1.0 - TiltShift);" << std::endl;
    }

    ss << "result = " << std::endl;

    if (direction == BlurRenderer::BlurDirectionHorizontal) {
        for (unsigned int i = 0; i < side; i++) {
            int offset = static_cast<int>(i - radius);
            ss << "texture2D(Texture0, TextureCoord + vec2(" <<
                  offset << ".0 * stepX, 0.0)) * Kernel" <<
                  std::abs(offset) << " +" << std::endl;
        }
        ss << "0.0 ;" << std::endl;
    }
    else if (direction == BlurRenderer::BlurDirectionVertical) {
        for (unsigned int i = 0; i < side; i++) {
            int offset = static_cast<int>(i - radius);
            ss << "texture2D(Texture0, TextureCoord + vec2(0.0, " <<
                  offset << ".0 * stepY)) * Kernel" <<
                  std::abs(offset) << " +" << std::endl;
        }
        ss << "0.0 ;" << std::endl;
    }
    else if (direction == BlurRenderer::BlurDirectionBoth) {
        for (unsigned int i = 0; i < side; i++) {
            int ioffset = static_cast<int>(i - radius);
            for (unsigned int j = 0; j < side; j++) {
                int joffset = static_cast<int>(j - radius);
                ss << "texture2D(Texture0, TextureCoord + vec2(" <<
                      ioffset << ".0 * stepX, " <<
                      joffset << ".0 * stepY))" <<
                      " * Kernel" << std::abs(ioffset) <<
                      " * Kernel" << std::abs(joffset) << " +" << std::endl;
            }
        }
        ss << " 0.0;" << std::endl;
    }

    frg_source.replace("$CONVOLUTION$", ss.str());
}
