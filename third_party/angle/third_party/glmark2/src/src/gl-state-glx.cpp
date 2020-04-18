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
#include "gl-state-glx.h"
#include "log.h"
#include "options.h"

#include <climits>

/******************
 * Public methods *
 ******************/

bool
GLStateGLX::init_display(void* native_display, GLVisualConfig& visual_config)
{
    xdpy_ = reinterpret_cast<Display*>(native_display);
    requested_visual_config_ = visual_config;

    if (!lib_.open_from_alternatives({"libGL.so", "libGL.so.1"})) {
        Log::error("Failed to load libGL\n");
        return false;
    }

    gladLoadGLXUserPtr(xdpy_, DefaultScreen(xdpy_), load_proc, this);
    return (xdpy_ != 0);
}

bool
GLStateGLX::init_surface(void* native_window)
{
    xwin_ = reinterpret_cast<Window>(native_window);

    return (xwin_ != 0);
}

bool
GLStateGLX::init_gl_extensions()
{
    GLExtensions::MapBuffer = glMapBuffer;
    GLExtensions::UnmapBuffer = glUnmapBuffer;
    return true;
}

bool
GLStateGLX::valid()
{
    if (!ensure_glx_fbconfig())
        return false;

    if (!ensure_glx_context())
        return false;

    if (glx_context_ == glXGetCurrentContext())
        return true;

    init_extensions();

    if (!glXMakeCurrent(xdpy_, xwin_, glx_context_)) {
        Log::error("glXMakeCurrent failed\n");
        return false;
    }

    if (gladLoadGLUserPtr(load_proc, this) == 0) {
        Log::error("Failed to load GL entry points\n");
        return false;
    }

    if (!init_gl_extensions())
        return false;

    unsigned int desired_swap(0);
    unsigned int actual_swap(-1);
    if (glXSwapIntervalEXT) {
        glXSwapIntervalEXT(xdpy_, xwin_, desired_swap);
        glXQueryDrawable(xdpy_, xwin_, GLX_SWAP_INTERVAL_EXT, &actual_swap);
        if (actual_swap == desired_swap)
            return true;
    }

    if (glXSwapIntervalMESA) {
        glXSwapIntervalMESA(desired_swap);
        actual_swap = glXGetSwapIntervalMESA();
        if (actual_swap == desired_swap)
            return true;
    }

    Log::info("** Failed to set swap interval. Results may be bounded above by refresh rate.\n");

    return true;
}


bool
GLStateGLX::reset()
{
    if (glx_context_)
    {
        glXDestroyContext(xdpy_, glx_context_);
        glx_context_ = 0;
    }

    return true;
}

void
GLStateGLX::swap()
{
    glXSwapBuffers(xdpy_, xwin_);
}

bool
GLStateGLX::gotNativeConfig(int& vid)
{
    if (!ensure_glx_fbconfig())
        return false;

    int native_id;
    if (glXGetFBConfigAttrib(xdpy_, glx_fbconfig_, GLX_VISUAL_ID, &native_id) != Success)
    {
        Log::debug("Failed to get native visual id for GLXFBConfig 0x%x\n", glx_fbconfig_);
        return false;
    }

    vid = native_id;
    return true;
}

void
GLStateGLX::getVisualConfig(GLVisualConfig& vc)
{
    if (!ensure_glx_fbconfig())
        return;

    get_glvisualconfig_glx(glx_fbconfig_, vc);
}

/*******************
 * Private methods *
 *******************/

bool
GLStateGLX::check_glx_version()
{
    int glx_major, glx_minor;

    if (!glXQueryVersion(xdpy_, &glx_major, &glx_minor ) ||
        (glx_major == 1  && glx_minor < 3) || glx_major < 1)
    {
        Log::error("GLX version >= 1.3 is required\n");
        return false;
    }

    return true;
}

void
GLStateGLX::init_extensions()
{
    /*
     * Parse the extensions we care about from the extension string.
     * Don't even bother to get function pointers until we know the
     * extension is present.
     */
    std::string extString;
    const char* exts = glXQueryExtensionsString(xdpy_, 0);
    if (exts) {
        extString = exts;
    }

    /*
     * GLX_EXT_swap_control or GL_MESA_swap_control. Note that
     * GLX_SGI_swap_control is not enough because it doesn't allow 0 as a valid
     * value (i.e. you can't turn off VSync).
     */
    if (!glXSwapIntervalEXT && !glXSwapIntervalMESA) {
        Log::info("** GLX does not support GLX_EXT_swap_control or GLX_MESA_swap_control!\n");
    }
}

bool
GLStateGLX::ensure_glx_fbconfig()
{
    static int attribs[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, requested_visual_config_.red,
        GLX_GREEN_SIZE, requested_visual_config_.green,
        GLX_BLUE_SIZE, requested_visual_config_.blue,
        GLX_ALPHA_SIZE, requested_visual_config_.alpha,
        GLX_DEPTH_SIZE, requested_visual_config_.depth,
        GLX_STENCIL_SIZE, requested_visual_config_.stencil,
        GLX_BUFFER_SIZE, requested_visual_config_.buffer,
        GLX_DOUBLEBUFFER, True,
        None
    };
    int num_configs;

    if (glx_fbconfig_)
        return true;

    if (!check_glx_version())
        return false;

    GLXFBConfig *fbc = glXChooseFBConfig(xdpy_, DefaultScreen(xdpy_),
                                         attribs, &num_configs);
    if (!fbc) {
        Log::error("glXChooseFBConfig() failed\n");
        return false;
    }

    std::vector<GLXFBConfig> configs(fbc, fbc + num_configs);

    Log::debug("Found %d matching FB configs.\n", num_configs);

    /* Select the best matching config */
    glx_fbconfig_ = select_best_config(configs);

    XFree(fbc);

    if (Options::show_debug) {
        int buf, red, green, blue, alpha, depth, stencil, id, native_id;
        glXGetFBConfigAttrib(xdpy_, glx_fbconfig_, GLX_FBCONFIG_ID, &id);
        glXGetFBConfigAttrib(xdpy_, glx_fbconfig_, GLX_VISUAL_ID, &native_id);
        glXGetFBConfigAttrib(xdpy_, glx_fbconfig_, GLX_BUFFER_SIZE, &buf);
        glXGetFBConfigAttrib(xdpy_, glx_fbconfig_, GLX_RED_SIZE, &red);
        glXGetFBConfigAttrib(xdpy_, glx_fbconfig_, GLX_GREEN_SIZE, &green);
        glXGetFBConfigAttrib(xdpy_, glx_fbconfig_, GLX_BLUE_SIZE, &blue);
        glXGetFBConfigAttrib(xdpy_, glx_fbconfig_, GLX_ALPHA_SIZE, &alpha);
        glXGetFBConfigAttrib(xdpy_, glx_fbconfig_, GLX_DEPTH_SIZE, &depth);
        glXGetFBConfigAttrib(xdpy_, glx_fbconfig_, GLX_STENCIL_SIZE, &stencil);
        Log::debug("GLX chosen config ID: 0x%x Native Visual ID: 0x%x\n"
                   "  Buffer: %d bits\n"
                   "     Red: %d bits\n"
                   "   Green: %d bits\n"
                   "    Blue: %d bits\n"
                   "   Alpha: %d bits\n"
                   "   Depth: %d bits\n"
                   " Stencil: %d bits\n",
                   id, native_id,
                   buf, red, green, blue, alpha, depth, stencil);
    }


    return true;
}

bool
GLStateGLX::ensure_glx_context()
{
    if (glx_context_)
        return true;

    if (!ensure_glx_fbconfig())
        return false;

    glx_context_ = glXCreateNewContext(xdpy_, glx_fbconfig_, GLX_RGBA_TYPE,
                                       0, True);
    if (!glx_context_) {
        Log::error("glXCreateNewContext failed\n");
        return false;
    }


    return true;
}

void
GLStateGLX::get_glvisualconfig_glx(const GLXFBConfig config, GLVisualConfig &visual_config)
{
    glXGetFBConfigAttrib(xdpy_, config, GLX_BUFFER_SIZE, &visual_config.buffer);
    glXGetFBConfigAttrib(xdpy_, config, GLX_RED_SIZE, &visual_config.red);
    glXGetFBConfigAttrib(xdpy_, config, GLX_GREEN_SIZE, &visual_config.green);
    glXGetFBConfigAttrib(xdpy_, config, GLX_BLUE_SIZE, &visual_config.blue);
    glXGetFBConfigAttrib(xdpy_, config, GLX_ALPHA_SIZE, &visual_config.alpha);
    glXGetFBConfigAttrib(xdpy_, config, GLX_DEPTH_SIZE, &visual_config.depth);
    glXGetFBConfigAttrib(xdpy_, config, GLX_STENCIL_SIZE, &visual_config.stencil);
}

GLXFBConfig
GLStateGLX::select_best_config(std::vector<GLXFBConfig> configs)
{
    int best_score(INT_MIN);
    GLXFBConfig best_config(0);

    /*
     * Go through all the configs and choose the one with the best score,
     * i.e., the one better matching the requested config.
     */
    for (std::vector<GLXFBConfig>::const_iterator iter = configs.begin();
         iter != configs.end();
         iter++)
    {
        const GLXFBConfig config(*iter);
        GLVisualConfig vc;
        int score;

        get_glvisualconfig_glx(config, vc);

        score = vc.match_score(requested_visual_config_);

        if (score > best_score) {
            best_score = score;
            best_config = config;
        }
    }

    return best_config;
}

GLADapiproc
GLStateGLX::load_proc(const char* name, void* userptr)
{
    if (glXGetProcAddress) {
        const GLubyte* bytes = reinterpret_cast<const GLubyte*>(name);
        GLADapiproc sym = glXGetProcAddress(bytes);
        if (sym) {
            return sym;
        }
    }

    GLStateGLX* state = reinterpret_cast<GLStateGLX*>(userptr);
    return reinterpret_cast<GLADapiproc>(state->lib_.load(name));
}
