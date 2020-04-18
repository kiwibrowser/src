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
#ifndef GLMARK2_CANVAS_ANDROID_H_
#define GLMARK2_CANVAS_ANDROID_H_

#include "canvas.h"

/**
 * Canvas for rendering to Android surfaces.
 *
 * This class doesn't perform any GLES2.0 surface and context management
 * (contrary to the CanvasX11* classes); these are handled in the Java
 * Android code.
 */
class CanvasAndroid : public Canvas
{
public:
    CanvasAndroid(int width, int height) :
        Canvas(width, height) {}
    ~CanvasAndroid() {}

    bool init();
    void visible(bool visible);
    void clear();
    void update();
    void print_info();
    Pixel read_pixel(int x, int y);
    void write_to_file(std::string &filename);
    bool should_quit();
    void resize(int width, int height);

private:
    void init_gl_extensions();
};

#endif


