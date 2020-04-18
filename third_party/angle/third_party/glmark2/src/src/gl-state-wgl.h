//
// Copyright © 2019 Jamie Madill
//
// This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
//
// glmark2 is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// glmark2.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
//  Jamie Madill
//
#ifndef GLMARK2_GL_STATE_WGL_H_
#define GLMARK2_GL_STATE_WGL_H_

#include "gl-state.h"
#include "shared-library.h"
#include <windows.h>

class GLStateWGL : public GLState
{
public:
    GLStateWGL();
    ~GLStateWGL();

    bool init_display(void* native_display, GLVisualConfig& config_pref);
    bool init_surface(void* native_window);
    bool init_gl_extensions();
    bool valid();
    bool reset();
    void swap();
    bool gotNativeConfig(int& vid);
    void getVisualConfig(GLVisualConfig& vc);

private:
    using api_proc = void (*)();
    static api_proc load_proc(const char *name, void *userptr);

    SharedLibrary wgl_library_;
    HDC hdc_;
    HGLRC wgl_context_;
    void* get_proc_addr_;
};

#endif // GLMARK2_GL_STATE_WGL_H_
