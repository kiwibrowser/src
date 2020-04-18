//
// Copyright © 2012 Linaro Limited
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
//  Jesse Barker
//
#include "gl-state-egl.h"
#include "log.h"
#include "options.h"
#include "gl-headers.h"
#include "limits.h"
#include "gl-headers.h"
#include <iomanip>
#include <sstream>
#include <cstring>

using std::vector;
using std::string;

GLADapiproc load_egl_func(const char *name, void *userdata)
{
    SharedLibrary *lib = reinterpret_cast<SharedLibrary *>(userdata);
    return reinterpret_cast<GLADapiproc>(lib->load(name));
}

/****************************
 * EGLConfig public methods *
 ****************************/

EglConfig::EglConfig(EGLDisplay dpy, EGLConfig config) :
    handle_(config),
    bufferSize_(0),
    redSize_(0),
    greenSize_(0),
    blueSize_(0),
    luminanceSize_(0),
    alphaSize_(0),
    alphaMaskSize_(0),
    bindTexRGB_(false),
    bindTexRGBA_(false),
    bufferType_(EGL_RGB_BUFFER),
    caveat_(0),
    configID_(0),
    conformant_(0),
    depthSize_(0),
    level_(0),
    pbufferWidth_(0),
    pbufferHeight_(0),
    pbufferPixels_(0),
    minSwapInterval_(0),
    maxSwapInterval_(0),
    nativeID_(0),
    nativeType_(0),
    nativeRenderable_(false),
    sampleBuffers_(0),
    samples_(0),
    stencilSize_(0),
    surfaceType_(0),
    xparentType_(0),
    xparentRedValue_(0),
    xparentGreenValue_(0),
    xparentBlueValue_(0)
{
    vector<string> badAttribVec;
    if (!eglGetConfigAttrib(dpy, handle_, EGL_CONFIG_ID, &configID_))
    {
        badAttribVec.push_back("EGL_CONFIG_ID");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_CONFIG_CAVEAT, &caveat_))
    {
        badAttribVec.push_back("EGL_CONFIG_CAVEAT");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_CONFORMANT, &conformant_))
    {
        badAttribVec.push_back("EGL_CONFORMANT");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_COLOR_BUFFER_TYPE, &bufferType_))
    {
        badAttribVec.push_back("EGL_COLOR_BUFFER_TYPE");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_BUFFER_SIZE, &bufferSize_))
    {
        badAttribVec.push_back("EGL_BUFFER_SIZE");
    }

    if (bufferType_ == EGL_RGB_BUFFER)
    {
        if (!eglGetConfigAttrib(dpy, handle_, EGL_RED_SIZE, &redSize_))
        {
            badAttribVec.push_back("EGL_RED_SIZE");
        }
        if (!eglGetConfigAttrib(dpy, handle_, EGL_GREEN_SIZE, &greenSize_))
        {
            badAttribVec.push_back("EGL_GREEN_SIZE");
        }
        if (!eglGetConfigAttrib(dpy, handle_, EGL_BLUE_SIZE, &blueSize_))
        {
            badAttribVec.push_back("EGL_BLUE_SIZE");
        }
    }
    else
    {
        if (!eglGetConfigAttrib(dpy, handle_, EGL_LUMINANCE_SIZE, &luminanceSize_))
        {
            badAttribVec.push_back("EGL_LUMINANCE_SIZE");
        }
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_ALPHA_SIZE, &alphaSize_))
    {
        badAttribVec.push_back("EGL_ALPHA_SIZE");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_ALPHA_MASK_SIZE, &alphaMaskSize_))
    {
        badAttribVec.push_back("EGL_ALPHA_MASK_SIZE");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_DEPTH_SIZE, &depthSize_))
    {
        badAttribVec.push_back("EGL_DEPTH_SIZE");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_STENCIL_SIZE, &stencilSize_))
    {
        badAttribVec.push_back("EGL_STENCIL_SIZE");
    }
    EGLint doBind(EGL_FALSE);
    if (!eglGetConfigAttrib(dpy, handle_, EGL_BIND_TO_TEXTURE_RGB, &doBind))
    {
        badAttribVec.push_back("EGL_BIND_TO_TEXTURE_RGB");
    }
    bindTexRGB_ = (doBind == EGL_TRUE);
    if (!eglGetConfigAttrib(dpy, handle_, EGL_BIND_TO_TEXTURE_RGBA, &doBind))
    {
        badAttribVec.push_back("EGL_BIND_TO_TEXTURE_RGBA");
    }
    bindTexRGBA_ = (doBind == EGL_TRUE);
    if (!eglGetConfigAttrib(dpy, handle_, EGL_LEVEL, &level_))
    {
        badAttribVec.push_back("EGL_LEVEL");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_MAX_PBUFFER_WIDTH, &pbufferWidth_))
    {
        badAttribVec.push_back("EGL_MAX_PBUFFER_WIDTH");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_MAX_PBUFFER_HEIGHT, &pbufferHeight_))
    {
        badAttribVec.push_back("EGL_MAX_PBUFFER_HEIGHT");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_MAX_PBUFFER_PIXELS, &pbufferPixels_))
    {
        badAttribVec.push_back("EGL_MAX_PBUFFER_PIXELS");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_MIN_SWAP_INTERVAL, &minSwapInterval_))
    {
        badAttribVec.push_back("EGL_MIN_SWAP_INTERVAL");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_MAX_SWAP_INTERVAL, &maxSwapInterval_))
    {
        badAttribVec.push_back("EGL_MAX_SWAP_INTERVAL");
    }
    EGLint doNative(EGL_FALSE);
    if (!eglGetConfigAttrib(dpy, handle_, EGL_NATIVE_RENDERABLE, &doNative))
    {
        badAttribVec.push_back("EGL_NATIVE_RENDERABLE");
    }
    nativeRenderable_ = (doNative == EGL_TRUE);
    if (!eglGetConfigAttrib(dpy, handle_, EGL_NATIVE_VISUAL_TYPE, &nativeType_))
    {
        badAttribVec.push_back("EGL_NATIVE_VISUAL_TYPE");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_NATIVE_VISUAL_ID, &nativeID_))
    {
        badAttribVec.push_back("EGL_NATIVE_VISUAL_ID");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_SURFACE_TYPE, &surfaceType_))
    {
        badAttribVec.push_back("EGL_SURFACE_TYPE");
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_SAMPLE_BUFFERS, &sampleBuffers_))
    {
        badAttribVec.push_back("EGL_SAMPLE_BUFFERS");
    }
    if (sampleBuffers_)
    {
        if (!eglGetConfigAttrib(dpy, handle_, EGL_SAMPLES, &samples_))
        {
            badAttribVec.push_back("EGL_SAMPLES");
        }
    }
    if (!eglGetConfigAttrib(dpy, handle_, EGL_TRANSPARENT_TYPE, &xparentType_))
    {
        badAttribVec.push_back("EGL_TRANSPARENT_TYPE");
    }
    //if (xparentType_ != EGL_NONE)
    {
        if (!eglGetConfigAttrib(dpy, handle_, EGL_TRANSPARENT_RED_VALUE, &xparentRedValue_))
        {
            badAttribVec.push_back("EGL_TRANSPARENT_RED_VALUE");
        }
        if (!eglGetConfigAttrib(dpy, handle_, EGL_TRANSPARENT_GREEN_VALUE, &xparentGreenValue_))
        {
            badAttribVec.push_back("EGL_TRANSPARENT_GREEN_VALUE");
        }
        if (!eglGetConfigAttrib(dpy, handle_, EGL_TRANSPARENT_BLUE_VALUE, &xparentBlueValue_))
        {
            badAttribVec.push_back("EGL_TRANSPARENT_BLUE_VALUE");
        }
    }

    if (!badAttribVec.empty())
    {
        Log::error("Failed to get the following config attributes for config 0x%x:\n",
                    config);
        for (vector<string>::const_iterator attribIt = badAttribVec.begin();
             attribIt != badAttribVec.end();
             attribIt++)
        {
            Log::error("%s\n", attribIt->c_str());
        }
    }
}

void
EglConfig::print_header()
{
    Log::debug("\n");
    Log::debug("    cfg buf  rgb  colorbuffer dp st config native support surface sample\n");
    Log::debug("     id  sz  lum  r  g  b  a  th cl caveat render  visid    type  buf ns\n");
    Log::debug("------------------------------------------------------------------------\n");
}

void
EglConfig::print() const
{
    std::ostringstream s;
    s.setf(std::ios::showbase);
    s.fill(' ');
    s << std::setw(7) << std::hex << configID_;
    s << std::setw(4) << std::dec << bufferSize_;
    if (bufferType_ == EGL_RGB_BUFFER)
    {
        s << std::setw(5) << "rgb";
        s << std::setw(3) << redSize_;
        s << std::setw(3) << greenSize_;
        s << std::setw(3) << blueSize_;
    }
    else
    {
        s << std::setw(5) << "lum";
        s << std::setw(3) << luminanceSize_;
        s << std::setw(3) << 0;
        s << std::setw(3) << 0;
    }
    s << std::setw(3) << alphaSize_;
    s << std::setw(4) << depthSize_;
    s << std::setw(3) << stencilSize_;
    string caveat("None");
    switch (caveat_)
    {
        case EGL_SLOW_CONFIG:
            caveat = string("Slow");
            break;
        case EGL_NON_CONFORMANT_CONFIG:
            caveat = string("Ncon");
            break;
        case EGL_NONE:
            // Initialized to none.
            break;
    }
    s << std::setw(7) << caveat;
    string doNative(nativeRenderable_ ? "true" : "false");
    s << std::setw(7) << doNative;
    s << std::setw(8) << std::hex << nativeID_;
    s << std::setw(8) << std::hex << surfaceType_;
    s << std::setw(4) << std::dec << sampleBuffers_;
    s << std::setw(3) << std::dec << samples_;
    Log::debug("%s\n", s.str().c_str());
}

/*****************************
 * GLStateEGL public methods *
 ****************************/

GLStateEGL::~GLStateEGL()
{
    if(egl_display_ != nullptr){
        if(!eglTerminate(egl_display_))
            Log::error("eglTerminate failed\n");
    }

    if(!eglReleaseThread())
       Log::error("eglReleaseThread failed\n");
}

bool
GLStateEGL::init_display(void* native_display, GLVisualConfig& visual_config)
{
#if defined(WIN32)
    if (!egl_lib_.open("libEGL.dll")) {
#else
    if (!egl_lib_.open_from_alternatives({"libEGL.so", "libEGL.so.1" })) {
#endif
        Log::error("Error loading EGL library\n");
        return false;
    }

    if (gladLoadEGLUserPtr(EGL_NO_DISPLAY, load_egl_func, &egl_lib_) == 0) {
        Log::error("Loading EGL entry points failed\n");
        return false;
    }

    native_display_ = reinterpret_cast<EGLNativeDisplayType>(native_display);
    requested_visual_config_ = visual_config;

    return gotValidDisplay();
}

bool
GLStateEGL::init_surface(void* native_window)
{
    native_window_ = reinterpret_cast<EGLNativeWindowType>(native_window);

    return gotValidSurface();
}

bool
GLStateEGL::init_gl_extensions()
{
#if GLMARK2_USE_GLESv2
    if (!gladLoadGLES2UserPtr(load_proc, this)) {
        Log::error("Loading GLESv2 entry points failed.");
        return false;
    }

    if (GLExtensions::support("GL_OES_mapbuffer")) {
        GLExtensions::MapBuffer = glMapBufferOES;
        GLExtensions::UnmapBuffer = glUnmapBufferOES;
    }
#elif GLMARK2_USE_GL
    if (!gladLoadGLUserPtr(load_proc, this)) {
        Log::error("Loading GL entry points failed.");
        return false;
    }
    GLExtensions::MapBuffer = glMapBuffer;
    GLExtensions::UnmapBuffer = glUnmapBuffer;
#endif
    return true;
}

bool
GLStateEGL::valid()
{
    if (!gotValidDisplay())
        return false;

    if (!gotValidConfig())
        return false;

    if (!gotValidSurface())
        return false;

    if (!gotValidContext())
        return false;

    if (egl_context_ == eglGetCurrentContext())
        return true;

    if (!eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_)) {
        Log::error("eglMakeCurrent failed with error: 0x%x\n", eglGetError());
        return false;
    }

    if (!eglSwapInterval(egl_display_, 0)) {
        Log::info("** Failed to set swap interval. Results may be bounded above by refresh rate.\n");
    }

    if (!init_gl_extensions()) {
        return false;
    }

    return true;
}

bool
GLStateEGL::reset()
{
    if (!gotValidDisplay()) {
        return false;
    }

    if (!egl_context_) {
        return true;
    }

    if (EGL_FALSE == eglDestroyContext(egl_display_, egl_context_)) {
        Log::debug("eglDestroyContext failed with error: 0x%x\n", eglGetError());
    }

    egl_context_ = 0;

    return true;
}

void
GLStateEGL::swap()
{
    eglSwapBuffers(egl_display_, egl_surface_);
}

bool
GLStateEGL::gotNativeConfig(int& vid)
{
    if (!gotValidConfig())
        return false;

    EGLint native_id;
    if (!eglGetConfigAttrib(egl_display_, egl_config_, EGL_NATIVE_VISUAL_ID,
        &native_id))
    {
        Log::debug("Failed to get native visual id for EGLConfig 0x%x\n", egl_config_);
        return false;
    }

    vid = native_id;
    return true;
}

void
GLStateEGL::getVisualConfig(GLVisualConfig& vc)
{
    if (!gotValidConfig())
        return;

    get_glvisualconfig(egl_config_, vc);
}

/******************************
 * GLStateEGL private methods *
 *****************************/

#ifdef GLMARK2_USE_X11
#define GLMARK2_NATIVE_EGL_DISPLAY_ENUM EGL_PLATFORM_X11_KHR
#elif  GLMARK2_USE_WAYLAND
#define GLMARK2_NATIVE_EGL_DISPLAY_ENUM EGL_PLATFORM_WAYLAND_KHR
#elif  GLMARK2_USE_DRM
#define GLMARK2_NATIVE_EGL_DISPLAY_ENUM EGL_PLATFORM_GBM_KHR
#elif  GLMARK2_USE_MIR
#define GLMARK2_NATIVE_EGL_DISPLAY_ENUM EGL_PLATFORM_MIR_KHR
#else
// Platforms not in the above platform enums fall back to eglGetDisplay.
#define GLMARK2_NATIVE_EGL_DISPLAY_ENUM 0
#endif

bool
GLStateEGL::gotValidDisplay()
{
    if (egl_display_)
        return true;

    char const * __restrict const supported_extensions =
        eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

    if (GLMARK2_NATIVE_EGL_DISPLAY_ENUM != 0 && supported_extensions
        && strstr(supported_extensions, "EGL_EXT_platform_base"))
    {
        Log::debug("Using eglGetPlatformDisplayEXT()\n");
        PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display =
            reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
                eglGetProcAddress("eglGetPlatformDisplayEXT"));

        if (get_platform_display != nullptr) {
            egl_display_ = get_platform_display(
                GLMARK2_NATIVE_EGL_DISPLAY_ENUM,
                reinterpret_cast<void*>(native_display_),
                nullptr);
        }

        if (!egl_display_) {
            Log::debug("eglGetPlatformDisplayEXT() failed with error: 0x%x\n",
                       eglGetError());
        }
    }
    else
    {
        Log::debug("eglGetPlatformDisplayEXT() seems unsupported\n");
    }

    /* Just in case get_platform_display failed... */
    if (!egl_display_) {
        Log::debug("Falling back to eglGetDisplay()\n");
        egl_display_ = eglGetDisplay(native_display_);
    }

    if (!egl_display_) {
        Log::error("eglGetDisplay() failed with error: 0x%x\n", eglGetError());
        return false;
    }

    int egl_major(-1);
    int egl_minor(-1);
    if (!eglInitialize(egl_display_, &egl_major, &egl_minor)) {
        Log::error("eglInitialize() failed with error: 0x%x\n", eglGetError());
        egl_display_ = 0;
        return false;
    }

    /* Reinitialize GLAD with a known display */
    if (gladLoadEGLUserPtr(egl_display_, load_egl_func, &egl_lib_) == 0) {
        Log::error("Loading EGL entry points failed\n");
        return false;
    }

#if GLMARK2_USE_GLESv2
    EGLenum apiType(EGL_OPENGL_ES_API);
#if defined(WIN32)
    std::initializer_list<const char *> libNames = { "libGLESv2.dll" };
#else
    std::initializer_list<const char *> libNames = { "libGLESv2.so", "libGLESv2.so.2" };
#endif
#elif GLMARK2_USE_GL
    EGLenum apiType(EGL_OPENGL_API);
    std::initializer_list<const char *> libNames = { "libGL.so", "libGL.so.1" };
#endif
    if (!eglBindAPI(apiType)) {
        Log::error("Failed to bind api EGL_OPENGL_ES_API\n");
        return false;
    }

    if (!gl_lib_.open_from_alternatives(libNames)) {
        Log::error("Error loading GL library\n");
        return false;
    }

    return true;
}

void
GLStateEGL::get_glvisualconfig(EGLConfig config, GLVisualConfig& visual_config)
{
    eglGetConfigAttrib(egl_display_, config, EGL_BUFFER_SIZE, &visual_config.buffer);
    eglGetConfigAttrib(egl_display_, config, EGL_RED_SIZE, &visual_config.red);
    eglGetConfigAttrib(egl_display_, config, EGL_GREEN_SIZE, &visual_config.green);
    eglGetConfigAttrib(egl_display_, config, EGL_BLUE_SIZE, &visual_config.blue);
    eglGetConfigAttrib(egl_display_, config, EGL_ALPHA_SIZE, &visual_config.alpha);
    eglGetConfigAttrib(egl_display_, config, EGL_DEPTH_SIZE, &visual_config.depth);
    eglGetConfigAttrib(egl_display_, config, EGL_STENCIL_SIZE, &visual_config.stencil);
}

EGLConfig
GLStateEGL::select_best_config(std::vector<EGLConfig>& configs)
{
    int best_score(INT_MIN);
    EGLConfig best_config(0);

    /*
     * Go through all the configs and choose the one with the best score,
     * i.e., the one better matching the requested config.
     */
    for (std::vector<EGLConfig>::const_iterator iter = configs.begin();
         iter != configs.end();
         iter++)
    {
        const EGLConfig config(*iter);
        GLVisualConfig vc;
        int score;

        get_glvisualconfig(config, vc);

        score = vc.match_score(requested_visual_config_);

        if (score > best_score) {
            best_score = score;
            best_config = config;
        }
    }

    return best_config;
}

bool
GLStateEGL::gotValidConfig()
{
    if (egl_config_)
        return true;

    if (!gotValidDisplay())
        return false;

    const EGLint config_attribs[] = {
        EGL_RED_SIZE, requested_visual_config_.red,
        EGL_GREEN_SIZE, requested_visual_config_.green,
        EGL_BLUE_SIZE, requested_visual_config_.blue,
        EGL_ALPHA_SIZE, requested_visual_config_.alpha,
        EGL_DEPTH_SIZE, requested_visual_config_.depth,
        EGL_STENCIL_SIZE, requested_visual_config_.stencil,
#if GLMARK2_USE_GLESv2
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#elif GLMARK2_USE_GL
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
        EGL_NONE
    };

    // Find out how many configs match the attributes.
    EGLint num_configs(0);
    if (!eglChooseConfig(egl_display_, config_attribs, 0, 0, &num_configs)) {
        Log::error("eglChooseConfig() (count query) failed with error: %d\n",
                   eglGetError());
        return false;
    }

    if (num_configs == 0) {
        Log::error("eglChooseConfig() didn't return any configs\n");
        return false;
    }

    // Get all the matching configs
    vector<EGLConfig> configs(num_configs);
    if (!eglChooseConfig(egl_display_, config_attribs, &configs.front(),
                         num_configs, &num_configs))
    {
        Log::error("eglChooseConfig() failed with error: %d\n",
                     eglGetError());
        return false;
    }

    // Select the best matching config
    egl_config_ = select_best_config(configs);

    vector<EglConfig> configVec;
    for (vector<EGLConfig>::const_iterator configIt = configs.begin();
         configIt != configs.end();
         configIt++)
    {
        EglConfig cfg(egl_display_, *configIt);
        configVec.push_back(cfg);
        if (*configIt == egl_config_) {
            best_config_ = cfg;
        }
    }

    // Print out the config information, and let the user know the decision
    // about the "best" one with respect to the options.
    unsigned int lineNumber(0);
    Log::debug("Got %u suitable EGLConfigs:\n", num_configs);
    for (vector<EglConfig>::const_iterator configIt = configVec.begin();
         configIt != configVec.end();
         configIt++, lineNumber++)
    {
        if (!(lineNumber % 32))
        {
            configIt->print_header();
        }
        configIt->print();
    }
    Log::debug("\n");
    Log::debug("Best EGLConfig ID: 0x%x\n", best_config_.configID());

    return true;
}

bool
GLStateEGL::gotValidSurface()
{
    if (egl_surface_)
        return true;

    if (!gotValidDisplay())
        return false;

    if (!gotValidConfig())
        return false;

    egl_surface_ = eglCreateWindowSurface(egl_display_, egl_config_, native_window_, 0);
    if (!egl_surface_) {
        Log::error("eglCreateWindowSurface failed with error: 0x%x\n", eglGetError());
        return false;
    }

    return true;
}

bool
GLStateEGL::gotValidContext()
{
    if (egl_context_)
        return true;

    if (!gotValidDisplay())
        return false;

    if (!gotValidConfig())
        return false;

    static const EGLint context_attribs[] = {
#ifdef GLMARK2_USE_GLESv2
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
        EGL_NONE
    };

    egl_context_ = eglCreateContext(egl_display_, egl_config_,
                                    EGL_NO_CONTEXT, context_attribs);
    if (!egl_context_) {
        Log::error("eglCreateContext() failed with error: 0x%x\n",
                   eglGetError());
        return false;
    }

    return true;
}

GLADapiproc
GLStateEGL::load_proc(const char* name, void* userptr)
{
    if (eglGetProcAddress) {
        GLADapiproc sym = reinterpret_cast<GLADapiproc>(eglGetProcAddress(name));
        if (sym) {
            return sym;
        }
    }

    GLStateEGL* state = reinterpret_cast<GLStateEGL*>(userptr);
    return reinterpret_cast<GLADapiproc>(state->gl_lib_.load(name));
}
