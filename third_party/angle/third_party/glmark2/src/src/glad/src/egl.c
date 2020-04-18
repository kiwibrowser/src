#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/egl.h>

#ifndef GLAD_IMPL_UTIL_C_
#define GLAD_IMPL_UTIL_C_

#ifdef _MSC_VER
#define GLAD_IMPL_UTIL_SSCANF sscanf_s
#else
#define GLAD_IMPL_UTIL_SSCANF sscanf
#endif

#endif /* GLAD_IMPL_UTIL_C_ */


int GLAD_EGL_VERSION_1_0 = 0;
int GLAD_EGL_VERSION_1_1 = 0;
int GLAD_EGL_VERSION_1_2 = 0;
int GLAD_EGL_VERSION_1_3 = 0;
int GLAD_EGL_VERSION_1_4 = 0;
int GLAD_EGL_VERSION_1_5 = 0;
int GLAD_EGL_EXT_platform_base = 0;
int GLAD_EGL_KHR_platform_gbm = 0;
int GLAD_EGL_KHR_platform_wayland = 0;
int GLAD_EGL_KHR_platform_x11 = 0;



PFNEGLBINDAPIPROC glad_eglBindAPI = NULL;
PFNEGLBINDTEXIMAGEPROC glad_eglBindTexImage = NULL;
PFNEGLCHOOSECONFIGPROC glad_eglChooseConfig = NULL;
PFNEGLCLIENTWAITSYNCPROC glad_eglClientWaitSync = NULL;
PFNEGLCOPYBUFFERSPROC glad_eglCopyBuffers = NULL;
PFNEGLCREATECONTEXTPROC glad_eglCreateContext = NULL;
PFNEGLCREATEIMAGEPROC glad_eglCreateImage = NULL;
PFNEGLCREATEPBUFFERFROMCLIENTBUFFERPROC glad_eglCreatePbufferFromClientBuffer = NULL;
PFNEGLCREATEPBUFFERSURFACEPROC glad_eglCreatePbufferSurface = NULL;
PFNEGLCREATEPIXMAPSURFACEPROC glad_eglCreatePixmapSurface = NULL;
PFNEGLCREATEPLATFORMPIXMAPSURFACEPROC glad_eglCreatePlatformPixmapSurface = NULL;
PFNEGLCREATEPLATFORMPIXMAPSURFACEEXTPROC glad_eglCreatePlatformPixmapSurfaceEXT = NULL;
PFNEGLCREATEPLATFORMWINDOWSURFACEPROC glad_eglCreatePlatformWindowSurface = NULL;
PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC glad_eglCreatePlatformWindowSurfaceEXT = NULL;
PFNEGLCREATESYNCPROC glad_eglCreateSync = NULL;
PFNEGLCREATEWINDOWSURFACEPROC glad_eglCreateWindowSurface = NULL;
PFNEGLDESTROYCONTEXTPROC glad_eglDestroyContext = NULL;
PFNEGLDESTROYIMAGEPROC glad_eglDestroyImage = NULL;
PFNEGLDESTROYSURFACEPROC glad_eglDestroySurface = NULL;
PFNEGLDESTROYSYNCPROC glad_eglDestroySync = NULL;
PFNEGLGETCONFIGATTRIBPROC glad_eglGetConfigAttrib = NULL;
PFNEGLGETCONFIGSPROC glad_eglGetConfigs = NULL;
PFNEGLGETCURRENTCONTEXTPROC glad_eglGetCurrentContext = NULL;
PFNEGLGETCURRENTDISPLAYPROC glad_eglGetCurrentDisplay = NULL;
PFNEGLGETCURRENTSURFACEPROC glad_eglGetCurrentSurface = NULL;
PFNEGLGETDISPLAYPROC glad_eglGetDisplay = NULL;
PFNEGLGETERRORPROC glad_eglGetError = NULL;
PFNEGLGETPLATFORMDISPLAYPROC glad_eglGetPlatformDisplay = NULL;
PFNEGLGETPLATFORMDISPLAYEXTPROC glad_eglGetPlatformDisplayEXT = NULL;
PFNEGLGETPROCADDRESSPROC glad_eglGetProcAddress = NULL;
PFNEGLGETSYNCATTRIBPROC glad_eglGetSyncAttrib = NULL;
PFNEGLINITIALIZEPROC glad_eglInitialize = NULL;
PFNEGLMAKECURRENTPROC glad_eglMakeCurrent = NULL;
PFNEGLQUERYAPIPROC glad_eglQueryAPI = NULL;
PFNEGLQUERYCONTEXTPROC glad_eglQueryContext = NULL;
PFNEGLQUERYSTRINGPROC glad_eglQueryString = NULL;
PFNEGLQUERYSURFACEPROC glad_eglQuerySurface = NULL;
PFNEGLRELEASETEXIMAGEPROC glad_eglReleaseTexImage = NULL;
PFNEGLRELEASETHREADPROC glad_eglReleaseThread = NULL;
PFNEGLSURFACEATTRIBPROC glad_eglSurfaceAttrib = NULL;
PFNEGLSWAPBUFFERSPROC glad_eglSwapBuffers = NULL;
PFNEGLSWAPINTERVALPROC glad_eglSwapInterval = NULL;
PFNEGLTERMINATEPROC glad_eglTerminate = NULL;
PFNEGLWAITCLIENTPROC glad_eglWaitClient = NULL;
PFNEGLWAITGLPROC glad_eglWaitGL = NULL;
PFNEGLWAITNATIVEPROC glad_eglWaitNative = NULL;
PFNEGLWAITSYNCPROC glad_eglWaitSync = NULL;


static void glad_egl_load_EGL_VERSION_1_0( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_VERSION_1_0) return;
    eglChooseConfig = (PFNEGLCHOOSECONFIGPROC) load("eglChooseConfig", userptr);
    eglCopyBuffers = (PFNEGLCOPYBUFFERSPROC) load("eglCopyBuffers", userptr);
    eglCreateContext = (PFNEGLCREATECONTEXTPROC) load("eglCreateContext", userptr);
    eglCreatePbufferSurface = (PFNEGLCREATEPBUFFERSURFACEPROC) load("eglCreatePbufferSurface", userptr);
    eglCreatePixmapSurface = (PFNEGLCREATEPIXMAPSURFACEPROC) load("eglCreatePixmapSurface", userptr);
    eglCreateWindowSurface = (PFNEGLCREATEWINDOWSURFACEPROC) load("eglCreateWindowSurface", userptr);
    eglDestroyContext = (PFNEGLDESTROYCONTEXTPROC) load("eglDestroyContext", userptr);
    eglDestroySurface = (PFNEGLDESTROYSURFACEPROC) load("eglDestroySurface", userptr);
    eglGetConfigAttrib = (PFNEGLGETCONFIGATTRIBPROC) load("eglGetConfigAttrib", userptr);
    eglGetConfigs = (PFNEGLGETCONFIGSPROC) load("eglGetConfigs", userptr);
    eglGetCurrentDisplay = (PFNEGLGETCURRENTDISPLAYPROC) load("eglGetCurrentDisplay", userptr);
    eglGetCurrentSurface = (PFNEGLGETCURRENTSURFACEPROC) load("eglGetCurrentSurface", userptr);
    eglGetDisplay = (PFNEGLGETDISPLAYPROC) load("eglGetDisplay", userptr);
    eglGetError = (PFNEGLGETERRORPROC) load("eglGetError", userptr);
    eglGetProcAddress = (PFNEGLGETPROCADDRESSPROC) load("eglGetProcAddress", userptr);
    eglInitialize = (PFNEGLINITIALIZEPROC) load("eglInitialize", userptr);
    eglMakeCurrent = (PFNEGLMAKECURRENTPROC) load("eglMakeCurrent", userptr);
    eglQueryContext = (PFNEGLQUERYCONTEXTPROC) load("eglQueryContext", userptr);
    eglQueryString = (PFNEGLQUERYSTRINGPROC) load("eglQueryString", userptr);
    eglQuerySurface = (PFNEGLQUERYSURFACEPROC) load("eglQuerySurface", userptr);
    eglSwapBuffers = (PFNEGLSWAPBUFFERSPROC) load("eglSwapBuffers", userptr);
    eglTerminate = (PFNEGLTERMINATEPROC) load("eglTerminate", userptr);
    eglWaitGL = (PFNEGLWAITGLPROC) load("eglWaitGL", userptr);
    eglWaitNative = (PFNEGLWAITNATIVEPROC) load("eglWaitNative", userptr);
}
static void glad_egl_load_EGL_VERSION_1_1( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_VERSION_1_1) return;
    eglBindTexImage = (PFNEGLBINDTEXIMAGEPROC) load("eglBindTexImage", userptr);
    eglReleaseTexImage = (PFNEGLRELEASETEXIMAGEPROC) load("eglReleaseTexImage", userptr);
    eglSurfaceAttrib = (PFNEGLSURFACEATTRIBPROC) load("eglSurfaceAttrib", userptr);
    eglSwapInterval = (PFNEGLSWAPINTERVALPROC) load("eglSwapInterval", userptr);
}
static void glad_egl_load_EGL_VERSION_1_2( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_VERSION_1_2) return;
    eglBindAPI = (PFNEGLBINDAPIPROC) load("eglBindAPI", userptr);
    eglCreatePbufferFromClientBuffer = (PFNEGLCREATEPBUFFERFROMCLIENTBUFFERPROC) load("eglCreatePbufferFromClientBuffer", userptr);
    eglQueryAPI = (PFNEGLQUERYAPIPROC) load("eglQueryAPI", userptr);
    eglReleaseThread = (PFNEGLRELEASETHREADPROC) load("eglReleaseThread", userptr);
    eglWaitClient = (PFNEGLWAITCLIENTPROC) load("eglWaitClient", userptr);
}
static void glad_egl_load_EGL_VERSION_1_4( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_VERSION_1_4) return;
    eglGetCurrentContext = (PFNEGLGETCURRENTCONTEXTPROC) load("eglGetCurrentContext", userptr);
}
static void glad_egl_load_EGL_VERSION_1_5( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_VERSION_1_5) return;
    eglClientWaitSync = (PFNEGLCLIENTWAITSYNCPROC) load("eglClientWaitSync", userptr);
    eglCreateImage = (PFNEGLCREATEIMAGEPROC) load("eglCreateImage", userptr);
    eglCreatePlatformPixmapSurface = (PFNEGLCREATEPLATFORMPIXMAPSURFACEPROC) load("eglCreatePlatformPixmapSurface", userptr);
    eglCreatePlatformWindowSurface = (PFNEGLCREATEPLATFORMWINDOWSURFACEPROC) load("eglCreatePlatformWindowSurface", userptr);
    eglCreateSync = (PFNEGLCREATESYNCPROC) load("eglCreateSync", userptr);
    eglDestroyImage = (PFNEGLDESTROYIMAGEPROC) load("eglDestroyImage", userptr);
    eglDestroySync = (PFNEGLDESTROYSYNCPROC) load("eglDestroySync", userptr);
    eglGetPlatformDisplay = (PFNEGLGETPLATFORMDISPLAYPROC) load("eglGetPlatformDisplay", userptr);
    eglGetSyncAttrib = (PFNEGLGETSYNCATTRIBPROC) load("eglGetSyncAttrib", userptr);
    eglWaitSync = (PFNEGLWAITSYNCPROC) load("eglWaitSync", userptr);
}
static void glad_egl_load_EGL_EXT_platform_base( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_platform_base) return;
    eglCreatePlatformPixmapSurfaceEXT = (PFNEGLCREATEPLATFORMPIXMAPSURFACEEXTPROC) load("eglCreatePlatformPixmapSurfaceEXT", userptr);
    eglCreatePlatformWindowSurfaceEXT = (PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC) load("eglCreatePlatformWindowSurfaceEXT", userptr);
    eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC) load("eglGetPlatformDisplayEXT", userptr);
}



static int glad_egl_get_extensions(EGLDisplay display, const char **extensions) {
    *extensions = eglQueryString(display, EGL_EXTENSIONS);

    return extensions != NULL;
}

static int glad_egl_has_extension(const char *extensions, const char *ext) {
    const char *loc;
    const char *terminator;
    if(extensions == NULL) {
        return 0;
    }
    while(1) {
        loc = strstr(extensions, ext);
        if(loc == NULL) {
            return 0;
        }
        terminator = loc + strlen(ext);
        if((loc == extensions || *(loc - 1) == ' ') &&
            (*terminator == ' ' || *terminator == '\0')) {
            return 1;
        }
        extensions = terminator;
    }
}

static GLADapiproc glad_egl_get_proc_from_userptr(const char *name, void *userptr) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

static int glad_egl_find_extensions_egl(EGLDisplay display) {
    const char *extensions;
    if (!glad_egl_get_extensions(display, &extensions)) return 0;

    GLAD_EGL_EXT_platform_base = glad_egl_has_extension(extensions, "EGL_EXT_platform_base");
    GLAD_EGL_KHR_platform_gbm = glad_egl_has_extension(extensions, "EGL_KHR_platform_gbm");
    GLAD_EGL_KHR_platform_wayland = glad_egl_has_extension(extensions, "EGL_KHR_platform_wayland");
    GLAD_EGL_KHR_platform_x11 = glad_egl_has_extension(extensions, "EGL_KHR_platform_x11");

    return 1;
}

static int glad_egl_find_core_egl(EGLDisplay display) {
    int major, minor;
    const char *version;

    if (display == NULL) {
        display = EGL_NO_DISPLAY; /* this is usually NULL, better safe than sorry */
    }
    if (display == EGL_NO_DISPLAY) {
        display = eglGetCurrentDisplay();
    }
#ifdef EGL_VERSION_1_4
    if (display == EGL_NO_DISPLAY) {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
#endif
    if (display == EGL_NO_DISPLAY) {
        return 0;
    }

    version = eglQueryString(display, EGL_VERSION);
    (void) eglGetError();

    if (version == NULL) {
        major = 1;
        minor = 0;
    } else {
        GLAD_IMPL_UTIL_SSCANF(version, "%d.%d", &major, &minor);
    }

    GLAD_EGL_VERSION_1_0 = (major == 1 && minor >= 0) || major > 1;
    GLAD_EGL_VERSION_1_1 = (major == 1 && minor >= 1) || major > 1;
    GLAD_EGL_VERSION_1_2 = (major == 1 && minor >= 2) || major > 1;
    GLAD_EGL_VERSION_1_3 = (major == 1 && minor >= 3) || major > 1;
    GLAD_EGL_VERSION_1_4 = (major == 1 && minor >= 4) || major > 1;
    GLAD_EGL_VERSION_1_5 = (major == 1 && minor >= 5) || major > 1;

    return GLAD_MAKE_VERSION(major, minor);
}

int gladLoadEGLUserPtr(EGLDisplay display, GLADuserptrloadfunc load, void* userptr) {
    int version;
    eglGetDisplay = (PFNEGLGETDISPLAYPROC) load("eglGetDisplay", userptr);
    eglGetCurrentDisplay = (PFNEGLGETCURRENTDISPLAYPROC) load("eglGetCurrentDisplay", userptr);
    eglQueryString = (PFNEGLQUERYSTRINGPROC) load("eglQueryString", userptr);
    eglGetError = (PFNEGLGETERRORPROC) load("eglGetError", userptr);
    if (eglGetDisplay == NULL || eglGetCurrentDisplay == NULL || eglQueryString == NULL || eglGetError == NULL) return 0;

    version = glad_egl_find_core_egl(display);
    if (!version) return 0;
    glad_egl_load_EGL_VERSION_1_0(load, userptr);
    glad_egl_load_EGL_VERSION_1_1(load, userptr);
    glad_egl_load_EGL_VERSION_1_2(load, userptr);
    glad_egl_load_EGL_VERSION_1_4(load, userptr);
    glad_egl_load_EGL_VERSION_1_5(load, userptr);

    if (!glad_egl_find_extensions_egl(display)) return 0;
    glad_egl_load_EGL_EXT_platform_base(load, userptr);

    return version;
}

int gladLoadEGL(EGLDisplay display, GLADloadfunc load) {
    return gladLoadEGLUserPtr(display, glad_egl_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}


