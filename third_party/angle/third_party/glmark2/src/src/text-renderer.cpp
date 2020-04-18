/*
 * Copyright © 2011 Linaro Limited
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
 *  Alexandros Frantzis (glmark2)
 */
#include "text-renderer.h"
#include "gl-headers.h"
#include "scene.h"
#include "shader-source.h"
#include "vec.h"
#include "mat.h"
#include "texture.h"

using LibMatrix::vec2;
using LibMatrix::mat4;

/* These are specific to the glyph texture atlas we are using */
static const unsigned int texture_size(512);
static const vec2 glyph_size_pixels(29.0, 57.0);
static const vec2 glyph_size(glyph_size_pixels/texture_size);

/******************
 * Public methods *
 ******************/

/**
 * TextRenderer default constructor.
 */
TextRenderer::TextRenderer(Canvas& canvas) :
    canvas_(canvas), dirty_(false), position_(-1.0, -1.0),
    texture_(0)
{
    size(0.03);

    glGenBuffers(2, vbo_);
    ShaderSource vtx_source(GLMARK_DATA_PATH"/shaders/text-renderer.vert");
    ShaderSource frg_source(GLMARK_DATA_PATH"/shaders/text-renderer.frag");

    if (!Scene::load_shaders_from_strings(program_, vtx_source.str(),
                                          frg_source.str()))
    {
        return;
    }

    GLint prev_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program);

    program_.start();
    program_["Texture0"] = 0;

    glUseProgram(prev_program);

    /* Load the glyph texture atlas */
    Texture::find_textures();
    Texture::load("glyph-atlas", &texture_,
                  GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,0);
}

TextRenderer::~TextRenderer()
{
    glDeleteBuffers(2, vbo_);
    glDeleteTextures(1, &texture_);
}

/**
 * Sets the text string to render.
 *
 * @param t the text string
 */
void
TextRenderer::text(const std::string& t)
{
    if (text_ != t) {
        text_ = t;
        dirty_ = true;
    }
}

/**
 * Sets the screen position to render at.
 *
 * @param t the position
 */
void
TextRenderer::position(const LibMatrix::vec2& p)
{
    if (position_ != p) {
        position_ = p;
        dirty_ = true;
    }
}

/**
 * Sets the size of each rendered glyph.
 *
 * The size corresponds to the width of each glyph
 * in normalized screen coordinates.
 *
 * @param s the size of each glyph
 */
void
TextRenderer::size(float s)
{
    if (size_.x() != s) {
        /* Take into account the glyph and canvas aspect ratio */
        double canvas_aspect =
            static_cast<double>(canvas_.width()) / canvas_.height();
        double glyph_aspect_rev = glyph_size.y() / glyph_size.x();
        size_ = vec2(s, s * canvas_aspect * glyph_aspect_rev);
        dirty_ = true;
    }
}

/**
 * Renders the text.
 */
void
TextRenderer::render()
{
    /* Save state */
    GLint prev_program = 0;
    GLint prev_array_buffer = 0;
    GLint prev_elem_array_buffer = 0;
    GLint prev_blend_src_rgb = 0;
    GLint prev_blend_dst_rgb = 0;
    GLint prev_blend_src_alpha = 0;
    GLint prev_blend_dst_alpha = 0;
    GLboolean prev_blend = GL_FALSE;
    GLboolean prev_depth_test = GL_FALSE;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev_array_buffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prev_elem_array_buffer);
    glGetIntegerv(GL_BLEND_SRC_RGB, &prev_blend_src_rgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &prev_blend_dst_rgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prev_blend_src_alpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prev_blend_dst_alpha);
    glGetBooleanv(GL_BLEND, &prev_blend);
    glGetBooleanv(GL_DEPTH_TEST, &prev_depth_test);

    /* Set new state */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_[1]);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    if (dirty_) {
        create_geometry();
        dirty_ = false;
    }

    program_.start();
    GLint position_loc = program_["position"].location();
    GLint texcoord_loc = program_["texcoord"].location();

    /* Render */
    glEnableVertexAttribArray(position_loc);
    glEnableVertexAttribArray(texcoord_loc);
    glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glVertexAttribPointer(texcoord_loc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<const GLvoid *>(2 * sizeof(float)));

    glDrawElements(GL_TRIANGLES, 6 * text_.length(), GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(texcoord_loc);
    glDisableVertexAttribArray(position_loc);

    /* Restore state */
    if (prev_depth_test == GL_TRUE)
        glEnable(GL_DEPTH_TEST);
    if (prev_blend == GL_FALSE)
        glDisable(GL_BLEND);
    glBlendFuncSeparate(prev_blend_src_rgb, prev_blend_dst_rgb,
                        prev_blend_src_alpha, prev_blend_dst_alpha);
    glBindBuffer(GL_ARRAY_BUFFER, prev_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prev_elem_array_buffer);
    glUseProgram(prev_program);
}

/*******************
 * Private methods *
 *******************/

/**
 * Creates the geometry needed to render the text.
 *
 * This method assumes that the text VBOs are properly bound.
 */
void
TextRenderer::create_geometry()
{
    std::vector<float> array;
    std::vector<GLushort> elem_array;
    vec2 pos(position_);

    for (size_t i = 0; i < text_.size(); i++) {
        vec2 texcoord = get_glyph_coords(text_[i]);

        /* Emit the elements for this glyph quad */
        /* Lower left */
        array.push_back(pos.x());
        array.push_back(pos.y());
        array.push_back(texcoord.x());
        array.push_back(texcoord.y());

        /* Lower right */
        pos.x(pos.x() + size_.x());
        texcoord.x(texcoord.x() + glyph_size.x());
        array.push_back(pos.x());
        array.push_back(pos.y());
        array.push_back(texcoord.x());
        array.push_back(texcoord.y());

        /* Upper left */
        pos.x(pos.x() - size_.x());
        pos.y(pos.y() + size_.y());
        texcoord.x(texcoord.x() - glyph_size.x());
        texcoord.y(texcoord.y() + glyph_size.y());
        array.push_back(pos.x());
        array.push_back(pos.y());
        array.push_back(texcoord.x());
        array.push_back(texcoord.y());

        /* Upper right */
        pos.x(pos.x() + size_.x());
        texcoord.x(texcoord.x() + glyph_size.x());
        array.push_back(pos.x());
        array.push_back(pos.y());
        array.push_back(texcoord.x());
        array.push_back(texcoord.y());

        /* Prepare for the next glyph */
        pos.y(pos.y() - size_.y());

        /* Emit the element indices for this glyph quad */
        elem_array.push_back(4 * i);
        elem_array.push_back(4 * i + 1);
        elem_array.push_back(4 * i + 2);
        elem_array.push_back(4 * i + 2);
        elem_array.push_back(4 * i + 1);
        elem_array.push_back(4 * i + 3);
    }

    /* Load the data into the corresponding VBOs */
    glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(float),
                 &array[0], GL_DYNAMIC_DRAW);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elem_array.size() * sizeof(GLushort),
                 &elem_array[0], GL_DYNAMIC_DRAW);
}

/**
 * Gets the texcoords of a glyph in the glyph texture atlas.
 *
 * @param c the character to get the glyph texcoords of
 *
 * @return the texcoords
 */
vec2
TextRenderer::get_glyph_coords(char c)
{
    static const unsigned int glyphs_per_row(texture_size / glyph_size_pixels.x());

    /* We only support the ASCII printable characters */
    if (c < 32 || c >= 127)
        c = 32;

    int n = c - 32;
    int row = n / glyphs_per_row;
    int col = n % glyphs_per_row;

    return vec2(col * glyph_size.x(), 1.0 - (row + 1) * glyph_size.y());
}

