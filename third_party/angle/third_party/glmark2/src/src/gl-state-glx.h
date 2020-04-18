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
#ifndef GLMARK2_GL_STATE_GLX_H_
#define GLMARK2_GL_STATE_GLX_H_

#include "gl-state.h"
#include "gl-visual-config.h"
#include "gl-headers.h"
#include "shared-library.h"

#include <vector>

#include <glad/glx.h>

class GLStateGLX : public GLState
{
public:
    GLStateGLX()
        : xdpy_(0), xwin_(0), glx_fbconfig_(0), glx_context_(0) {}

    bool valid();
    bool init_display(void* native_display, GLVisualConfig& config_pref);
    bool init_surface(void* native_window);
    bool init_gl_extensions();
    bool reset();
    void swap();
    bool gotNativeConfig(int& vid);
    void getVisualConfig(GLVisualConfig& vc);

private:
    bool check_glx_version();
    void init_extensions();
    bool ensure_glx_fbconfig();
    bool ensure_glx_context();
    void get_glvisualconfig_glx(GLXFBConfig config, GLVisualConfig &visual_config);
    GLXFBConfig select_best_config(std::vector<GLXFBConfig> configs);

    static GLADapiproc load_proc(const char* name, void* userptr);

    Display* xdpy_;
    Window xwin_;
    GLXFBConfig glx_fbconfig_;
    GLXContext glx_context_;
    GLVisualConfig requested_visual_config_;
    SharedLibrary lib_;
};

#endif /* GLMARK2_GL_STATE_GLX_H_ */
