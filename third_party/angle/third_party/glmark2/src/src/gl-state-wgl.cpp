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
#include "gl-state-wgl.h"

#include "gl-headers.h"
#include "log.h"
#include "options.h"

/******************
 * Public methods *
 ******************/

GLStateWGL::GLStateWGL() : hdc_(nullptr), wgl_context_(nullptr), get_proc_addr_(nullptr) {}

GLStateWGL::~GLStateWGL()
{
    if (wgl_context_) {
        wglDeleteContext(wgl_context_);
        wgl_context_ = nullptr;
    }
}

bool
GLStateWGL::init_display(void* native_display, GLVisualConfig& /*visual_config*/)
{
    if (!wgl_library_.open("opengl32.dll")) {
        Log::error("Failed to load opengl32.dll\n");
        return false;
    }

    hdc_ = reinterpret_cast<HDC>(native_display);

    PIXELFORMATDESCRIPTOR pixel_format_desc = {};
    pixel_format_desc.nSize = sizeof(pixel_format_desc);
    pixel_format_desc.nVersion = 1;
    pixel_format_desc.dwFlags =
        PFD_DRAW_TO_WINDOW | PFD_GENERIC_ACCELERATED | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixel_format_desc.iPixelType = PFD_TYPE_RGBA;
    pixel_format_desc.cColorBits = 24;
    pixel_format_desc.cAlphaBits = 8;
    pixel_format_desc.cDepthBits = 24;
    pixel_format_desc.cStencilBits = 8;
    pixel_format_desc.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(hdc_, &pixel_format_desc);
    if (pixelFormat == 0) {
        Log::error("Could not find a compatible pixel format\n");
        return false;
    }

    if (SetPixelFormat(hdc_, pixelFormat, &pixel_format_desc) != TRUE) {
        Log::error("Failed to set the pixel format\n");
        return false;
    }

    wgl_context_ = wglCreateContext(hdc_);
    if (!wgl_context_) {
        Log::error("Failed to create a WGL context\n");
        return false;
    }

    if (wglMakeCurrent(hdc_, wgl_context_) == FALSE) {
        Log::error("Error during wglMakeCurrent\n");
        return false;
    }

    get_proc_addr_ = wgl_library_.load("wglGetProcAddress");
    if (!get_proc_addr_) {
        Log::error("Failed to find wglGetProcAddress\n");
        return false;
    }

    if (gladLoadGLUserPtr(load_proc, this) == 0) {
        Log::error("Failed to load WGL entry points\n");
        return false;
    }

    return true;
}

bool
GLStateWGL::init_surface(void* /*native_window*/)
{
    return true;
}

bool
GLStateWGL::init_gl_extensions()
{
    GLExtensions::MapBuffer = glMapBuffer;
    GLExtensions::UnmapBuffer = glUnmapBuffer;
    return true;
}

bool
GLStateWGL::valid()
{
    return wgl_context_ != nullptr;
}


bool
GLStateWGL::reset()
{
    return true;
}

void
GLStateWGL::swap()
{
    if (SwapBuffers(hdc_) == FALSE) {
        Log::error("Error during SwapBuffers\n");
    }
}

bool
GLStateWGL::gotNativeConfig(int& vid)
{
    vid = 1;
    return true;
}

void
GLStateWGL::getVisualConfig(GLVisualConfig& vc)
{
    vc.buffer = 32;
    vc.red = 8;
    vc.green = 8;
    vc.blue = 8;
    vc.alpha = 8;
    vc.depth = 24;
    vc.stencil = 8;
}

/*******************
 * Private methods *
 *******************/

GLStateWGL::api_proc GLStateWGL::load_proc(const char *name, void *userptr)
{
    GLStateWGL* state = reinterpret_cast<GLStateWGL*>(userptr);
    auto get_proc_addr = reinterpret_cast<decltype(&wglGetProcAddress)>(state->get_proc_addr_);
    auto proc = reinterpret_cast<GLStateWGL::api_proc>(get_proc_addr(name));
    if (proc) {
        return proc;
    }
    return reinterpret_cast<GLStateWGL::api_proc>(state->wgl_library_.load(name));
}

