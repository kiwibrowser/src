/*
 * Copyright © 2010-2011 Linaro Limited
 * Copyright © 2013 Canonical Ltd
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
#ifndef GLMARK2_CANVAS_GENERIC_H_
#define GLMARK2_CANVAS_GENERIC_H_

#include "canvas.h"

class GLState;
class NativeState;

/**
 * Canvas for rendering with GL to an X11 window.
 */
class CanvasGeneric : public Canvas
{
public:
    CanvasGeneric(NativeState& native_state, GLState& gl_state,
                  int width, int height)
        : Canvas(width, height),
          native_state_(native_state), gl_state_(gl_state),
          gl_color_format_(0), gl_depth_format_(0),
          color_renderbuffer_(0), depth_renderbuffer_(0), fbo_(0) {}

    bool init();
    bool reset();
    void visible(bool visible);
    void clear();
    void update();
    void print_info();
    Pixel read_pixel(int x, int y);
    void write_to_file(std::string &filename);
    bool should_quit();
    void resize(int width, int height);
    unsigned int fbo();

private:
    bool supports_gl2();
    bool resize_no_viewport(int width, int height);
    bool do_make_current();
    bool ensure_gl_formats();
    bool ensure_fbo();
    void release_fbo();
    const char *get_gl_format_str(GLenum f);

    NativeState& native_state_;
    GLState& gl_state_;
    void* native_window_;
    GLenum gl_color_format_;
    GLenum gl_depth_format_;
    GLuint color_renderbuffer_;
    GLuint depth_renderbuffer_;
    GLuint fbo_;
};

#endif /* GLMARK2_CANVAS_GENERIC_H_ */
