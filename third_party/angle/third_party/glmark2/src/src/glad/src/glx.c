#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glx.h>

#ifndef GLAD_IMPL_UTIL_C_
#define GLAD_IMPL_UTIL_C_

#ifdef _MSC_VER
#define GLAD_IMPL_UTIL_SSCANF sscanf_s
#else
#define GLAD_IMPL_UTIL_SSCANF sscanf
#endif

#endif /* GLAD_IMPL_UTIL_C_ */


int GLAD_GLX_VERSION_1_0 = 0;
int GLAD_GLX_VERSION_1_1 = 0;
int GLAD_GLX_VERSION_1_2 = 0;
int GLAD_GLX_VERSION_1_3 = 0;
int GLAD_GLX_VERSION_1_4 = 0;
int GLAD_GLX_EXT_swap_control = 0;
int GLAD_GLX_MESA_swap_control = 0;



PFNGLXCHOOSEFBCONFIGPROC glad_glXChooseFBConfig = NULL;
PFNGLXCHOOSEVISUALPROC glad_glXChooseVisual = NULL;
PFNGLXCOPYCONTEXTPROC glad_glXCopyContext = NULL;
PFNGLXCREATECONTEXTPROC glad_glXCreateContext = NULL;
PFNGLXCREATEGLXPIXMAPPROC glad_glXCreateGLXPixmap = NULL;
PFNGLXCREATENEWCONTEXTPROC glad_glXCreateNewContext = NULL;
PFNGLXCREATEPBUFFERPROC glad_glXCreatePbuffer = NULL;
PFNGLXCREATEPIXMAPPROC glad_glXCreatePixmap = NULL;
PFNGLXCREATEWINDOWPROC glad_glXCreateWindow = NULL;
PFNGLXDESTROYCONTEXTPROC glad_glXDestroyContext = NULL;
PFNGLXDESTROYGLXPIXMAPPROC glad_glXDestroyGLXPixmap = NULL;
PFNGLXDESTROYPBUFFERPROC glad_glXDestroyPbuffer = NULL;
PFNGLXDESTROYPIXMAPPROC glad_glXDestroyPixmap = NULL;
PFNGLXDESTROYWINDOWPROC glad_glXDestroyWindow = NULL;
PFNGLXGETCLIENTSTRINGPROC glad_glXGetClientString = NULL;
PFNGLXGETCONFIGPROC glad_glXGetConfig = NULL;
PFNGLXGETCURRENTCONTEXTPROC glad_glXGetCurrentContext = NULL;
PFNGLXGETCURRENTDISPLAYPROC glad_glXGetCurrentDisplay = NULL;
PFNGLXGETCURRENTDRAWABLEPROC glad_glXGetCurrentDrawable = NULL;
PFNGLXGETCURRENTREADDRAWABLEPROC glad_glXGetCurrentReadDrawable = NULL;
PFNGLXGETFBCONFIGATTRIBPROC glad_glXGetFBConfigAttrib = NULL;
PFNGLXGETFBCONFIGSPROC glad_glXGetFBConfigs = NULL;
PFNGLXGETPROCADDRESSPROC glad_glXGetProcAddress = NULL;
PFNGLXGETSELECTEDEVENTPROC glad_glXGetSelectedEvent = NULL;
PFNGLXGETSWAPINTERVALMESAPROC glad_glXGetSwapIntervalMESA = NULL;
PFNGLXGETVISUALFROMFBCONFIGPROC glad_glXGetVisualFromFBConfig = NULL;
PFNGLXISDIRECTPROC glad_glXIsDirect = NULL;
PFNGLXMAKECONTEXTCURRENTPROC glad_glXMakeContextCurrent = NULL;
PFNGLXMAKECURRENTPROC glad_glXMakeCurrent = NULL;
PFNGLXQUERYCONTEXTPROC glad_glXQueryContext = NULL;
PFNGLXQUERYDRAWABLEPROC glad_glXQueryDrawable = NULL;
PFNGLXQUERYEXTENSIONPROC glad_glXQueryExtension = NULL;
PFNGLXQUERYEXTENSIONSSTRINGPROC glad_glXQueryExtensionsString = NULL;
PFNGLXQUERYSERVERSTRINGPROC glad_glXQueryServerString = NULL;
PFNGLXQUERYVERSIONPROC glad_glXQueryVersion = NULL;
PFNGLXSELECTEVENTPROC glad_glXSelectEvent = NULL;
PFNGLXSWAPBUFFERSPROC glad_glXSwapBuffers = NULL;
PFNGLXSWAPINTERVALEXTPROC glad_glXSwapIntervalEXT = NULL;
PFNGLXSWAPINTERVALMESAPROC glad_glXSwapIntervalMESA = NULL;
PFNGLXUSEXFONTPROC glad_glXUseXFont = NULL;
PFNGLXWAITGLPROC glad_glXWaitGL = NULL;
PFNGLXWAITXPROC glad_glXWaitX = NULL;


static void glad_glx_load_GLX_VERSION_1_0( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GLX_VERSION_1_0) return;
    glXChooseVisual = (PFNGLXCHOOSEVISUALPROC) load("glXChooseVisual", userptr);
    glXCopyContext = (PFNGLXCOPYCONTEXTPROC) load("glXCopyContext", userptr);
    glXCreateContext = (PFNGLXCREATECONTEXTPROC) load("glXCreateContext", userptr);
    glXCreateGLXPixmap = (PFNGLXCREATEGLXPIXMAPPROC) load("glXCreateGLXPixmap", userptr);
    glXDestroyContext = (PFNGLXDESTROYCONTEXTPROC) load("glXDestroyContext", userptr);
    glXDestroyGLXPixmap = (PFNGLXDESTROYGLXPIXMAPPROC) load("glXDestroyGLXPixmap", userptr);
    glXGetConfig = (PFNGLXGETCONFIGPROC) load("glXGetConfig", userptr);
    glXGetCurrentContext = (PFNGLXGETCURRENTCONTEXTPROC) load("glXGetCurrentContext", userptr);
    glXGetCurrentDrawable = (PFNGLXGETCURRENTDRAWABLEPROC) load("glXGetCurrentDrawable", userptr);
    glXIsDirect = (PFNGLXISDIRECTPROC) load("glXIsDirect", userptr);
    glXMakeCurrent = (PFNGLXMAKECURRENTPROC) load("glXMakeCurrent", userptr);
    glXQueryExtension = (PFNGLXQUERYEXTENSIONPROC) load("glXQueryExtension", userptr);
    glXQueryVersion = (PFNGLXQUERYVERSIONPROC) load("glXQueryVersion", userptr);
    glXSwapBuffers = (PFNGLXSWAPBUFFERSPROC) load("glXSwapBuffers", userptr);
    glXUseXFont = (PFNGLXUSEXFONTPROC) load("glXUseXFont", userptr);
    glXWaitGL = (PFNGLXWAITGLPROC) load("glXWaitGL", userptr);
    glXWaitX = (PFNGLXWAITXPROC) load("glXWaitX", userptr);
}
static void glad_glx_load_GLX_VERSION_1_1( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GLX_VERSION_1_1) return;
    glXGetClientString = (PFNGLXGETCLIENTSTRINGPROC) load("glXGetClientString", userptr);
    glXQueryExtensionsString = (PFNGLXQUERYEXTENSIONSSTRINGPROC) load("glXQueryExtensionsString", userptr);
    glXQueryServerString = (PFNGLXQUERYSERVERSTRINGPROC) load("glXQueryServerString", userptr);
}
static void glad_glx_load_GLX_VERSION_1_2( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GLX_VERSION_1_2) return;
    glXGetCurrentDisplay = (PFNGLXGETCURRENTDISPLAYPROC) load("glXGetCurrentDisplay", userptr);
}
static void glad_glx_load_GLX_VERSION_1_3( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GLX_VERSION_1_3) return;
    glXChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC) load("glXChooseFBConfig", userptr);
    glXCreateNewContext = (PFNGLXCREATENEWCONTEXTPROC) load("glXCreateNewContext", userptr);
    glXCreatePbuffer = (PFNGLXCREATEPBUFFERPROC) load("glXCreatePbuffer", userptr);
    glXCreatePixmap = (PFNGLXCREATEPIXMAPPROC) load("glXCreatePixmap", userptr);
    glXCreateWindow = (PFNGLXCREATEWINDOWPROC) load("glXCreateWindow", userptr);
    glXDestroyPbuffer = (PFNGLXDESTROYPBUFFERPROC) load("glXDestroyPbuffer", userptr);
    glXDestroyPixmap = (PFNGLXDESTROYPIXMAPPROC) load("glXDestroyPixmap", userptr);
    glXDestroyWindow = (PFNGLXDESTROYWINDOWPROC) load("glXDestroyWindow", userptr);
    glXGetCurrentReadDrawable = (PFNGLXGETCURRENTREADDRAWABLEPROC) load("glXGetCurrentReadDrawable", userptr);
    glXGetFBConfigAttrib = (PFNGLXGETFBCONFIGATTRIBPROC) load("glXGetFBConfigAttrib", userptr);
    glXGetFBConfigs = (PFNGLXGETFBCONFIGSPROC) load("glXGetFBConfigs", userptr);
    glXGetSelectedEvent = (PFNGLXGETSELECTEDEVENTPROC) load("glXGetSelectedEvent", userptr);
    glXGetVisualFromFBConfig = (PFNGLXGETVISUALFROMFBCONFIGPROC) load("glXGetVisualFromFBConfig", userptr);
    glXMakeContextCurrent = (PFNGLXMAKECONTEXTCURRENTPROC) load("glXMakeContextCurrent", userptr);
    glXQueryContext = (PFNGLXQUERYCONTEXTPROC) load("glXQueryContext", userptr);
    glXQueryDrawable = (PFNGLXQUERYDRAWABLEPROC) load("glXQueryDrawable", userptr);
    glXSelectEvent = (PFNGLXSELECTEVENTPROC) load("glXSelectEvent", userptr);
}
static void glad_glx_load_GLX_VERSION_1_4( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GLX_VERSION_1_4) return;
    glXGetProcAddress = (PFNGLXGETPROCADDRESSPROC) load("glXGetProcAddress", userptr);
}
static void glad_glx_load_GLX_EXT_swap_control( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GLX_EXT_swap_control) return;
    glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC) load("glXSwapIntervalEXT", userptr);
}
static void glad_glx_load_GLX_MESA_swap_control( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GLX_MESA_swap_control) return;
    glXGetSwapIntervalMESA = (PFNGLXGETSWAPINTERVALMESAPROC) load("glXGetSwapIntervalMESA", userptr);
    glXSwapIntervalMESA = (PFNGLXSWAPINTERVALMESAPROC) load("glXSwapIntervalMESA", userptr);
}



static int glad_glx_has_extension(Display *display, int screen, const char *ext) {
#ifndef GLX_VERSION_1_1
    (void) display;
    (void) screen;
    (void) ext;
#else
    const char *terminator;
    const char *loc;
    const char *extensions;

    if (glXQueryExtensionsString == NULL) {
        return 0;
    }

    extensions = glXQueryExtensionsString(display, screen);

    if(extensions == NULL || ext == NULL) {
        return 0;
    }

    while(1) {
        loc = strstr(extensions, ext);
        if(loc == NULL)
            break;

        terminator = loc + strlen(ext);
        if((loc == extensions || *(loc - 1) == ' ') &&
            (*terminator == ' ' || *terminator == '\0')) {
            return 1;
        }
        extensions = terminator;
    }
#endif

    return 0;
}

static GLADapiproc glad_glx_get_proc_from_userptr(const char* name, void *userptr) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

static int glad_glx_find_extensions(Display *display, int screen) {
    GLAD_GLX_EXT_swap_control = glad_glx_has_extension(display, screen, "GLX_EXT_swap_control");
    GLAD_GLX_MESA_swap_control = glad_glx_has_extension(display, screen, "GLX_MESA_swap_control");
    return 1;
}

static int glad_glx_find_core_glx(Display **display, int *screen) {
    int major = 0, minor = 0;
    if(*display == NULL) {
#ifdef GLAD_GLX_NO_X11
        (void) screen;
        return 0;
#else
        *display = XOpenDisplay(0);
        if (*display == NULL) {
            return 0;
        }
        *screen = XScreenNumberOfScreen(XDefaultScreenOfDisplay(*display));
#endif
    }
    glXQueryVersion(*display, &major, &minor);
    GLAD_GLX_VERSION_1_0 = (major == 1 && minor >= 0) || major > 1;
    GLAD_GLX_VERSION_1_1 = (major == 1 && minor >= 1) || major > 1;
    GLAD_GLX_VERSION_1_2 = (major == 1 && minor >= 2) || major > 1;
    GLAD_GLX_VERSION_1_3 = (major == 1 && minor >= 3) || major > 1;
    GLAD_GLX_VERSION_1_4 = (major == 1 && minor >= 4) || major > 1;
    return GLAD_MAKE_VERSION(major, minor);
}

int gladLoadGLXUserPtr(Display *display, int screen, GLADuserptrloadfunc load, void *userptr) {
    int version;
    glXQueryVersion = (PFNGLXQUERYVERSIONPROC) load("glXQueryVersion", userptr);
    if(glXQueryVersion == NULL) return 0;
    version = glad_glx_find_core_glx(&display, &screen);

    glad_glx_load_GLX_VERSION_1_0(load, userptr);
    glad_glx_load_GLX_VERSION_1_1(load, userptr);
    glad_glx_load_GLX_VERSION_1_2(load, userptr);
    glad_glx_load_GLX_VERSION_1_3(load, userptr);
    glad_glx_load_GLX_VERSION_1_4(load, userptr);

    if (!glad_glx_find_extensions(display, screen)) return 0;
    glad_glx_load_GLX_EXT_swap_control(load, userptr);
    glad_glx_load_GLX_MESA_swap_control(load, userptr);

    return version;
}

int gladLoadGLX(Display *display, int screen, GLADloadfunc load) {
    return gladLoadGLXUserPtr(display, screen, glad_glx_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}


