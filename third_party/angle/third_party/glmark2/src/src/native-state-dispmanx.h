/*
 * Copyright © 2013 Rafal Mielniczuk
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
 *  Rafal Mielniczuk <rafal.mielniczuk2@gmail.com>
 */

#ifndef GLMARK2_NATIVE_STATE_DISPMANX_H_
#define GLMARK2_NATIVE_STATE_DISPMANX_H_

#include <vector>
#include "bcm_host.h"
#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "native-state.h"

class NativeStateDispmanx : public NativeState
{
public:
    NativeStateDispmanx();
    ~NativeStateDispmanx();

    bool init_display();
    void* display();
    bool create_window(WindowProperties const& properties);
    void* window(WindowProperties& properties);
    void visible(bool v);
    bool should_quit();
    void flip();

private:
    DISPMANX_DISPLAY_HANDLE_T dispmanx_display;
    DISPMANX_UPDATE_HANDLE_T dispmanx_update;
    DISPMANX_ELEMENT_HANDLE_T dispmanx_element;
    EGL_DISPMANX_WINDOW_T egl_dispmanx_window;
    WindowProperties properties_;
};

#endif /* GLMARK2_NATIVE_STATE_DISPMANX_H_ */
