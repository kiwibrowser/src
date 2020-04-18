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
#ifndef GLMARK2_TEXT_RENDERER_H_
#define GLMARK2_TEXT_RENDERER_H_

#include <string>
#include "gl-headers.h"
#include "vec.h"
#include "program.h"
#include "canvas.h"

/**
 * Renders text using OpenGL textures.
 */
class TextRenderer
{
public:
    TextRenderer(Canvas& canvas);
    ~TextRenderer();

    void text(const std::string& t);
    void position(const LibMatrix::vec2& p);
    void size(float s);

    void render();

private:
    void create_geometry();
    LibMatrix::vec2 get_glyph_coords(char c);

    Canvas& canvas_;
    bool dirty_;
    std::string text_;
    LibMatrix::vec2 position_;
    LibMatrix::vec2 size_;
    Program program_;
    GLuint vbo_[2];
    GLuint texture_;
};

#endif
