/*
** The OpenGL Extension Wrangler Library
** Copyright (C) 2008-2017, Nigel Stewart <nigels[]users sourceforge net>
** Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>
** Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
** Copyright (C) 2002, Lev Povalahev
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without 
** modification, are permitted provided that the following conditions are met:
** 
** * Redistributions of source code must retain the above copyright notice, 
**   this list of conditions and the following disclaimer.
** * Redistributions in binary form must reproduce the above copyright notice, 
**   this list of conditions and the following disclaimer in the documentation 
**   and/or other materials provided with the distribution.
** * The name of the author may be used to endorse or promote products 
**   derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
** THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * Mesa 3-D graphics library
 * Version:  7.0
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
** Copyright (c) 2007 The Khronos Group Inc.
** 
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
** 
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
** 
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#ifndef __eglew_h__
#define __eglew_h__
#define __EGLEW_H__

#ifdef __eglext_h_
#error eglext.h included before eglew.h
#endif

#if defined(__egl_h_)
#error egl.h included before eglew.h
#endif

#define __eglext_h_

#define __egl_h_

#ifndef EGLAPIENTRY
#define EGLAPIENTRY
#endif
#ifndef EGLAPI
#define EGLAPI extern
#endif

/* EGL Types */
#include <sys/types.h>

#include <KHR/khrplatform.h>
#include <EGL/eglplatform.h>

#include <GL/glew.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t EGLint;

typedef unsigned int EGLBoolean;
typedef void *EGLDisplay;
typedef void *EGLConfig;
typedef void *EGLSurface;
typedef void *EGLContext;
typedef void (*__eglMustCastToProperFunctionPointerType)(void);

typedef unsigned int EGLenum;
typedef void *EGLClientBuffer;

typedef void *EGLSync;
typedef intptr_t EGLAttrib;
typedef khronos_utime_nanoseconds_t EGLTime;
typedef void *EGLImage;

typedef void *EGLSyncKHR;
typedef intptr_t EGLAttribKHR;
typedef void *EGLLabelKHR;
typedef void *EGLObjectKHR;
typedef void (EGLAPIENTRY  *EGLDEBUGPROCKHR)(EGLenum error,const char *command,EGLint messageType,EGLLabelKHR threadLabel,EGLLabelKHR objectLabel,const char* message);
typedef khronos_utime_nanoseconds_t EGLTimeKHR;
typedef void *EGLImageKHR;
typedef void *EGLStreamKHR;
typedef khronos_uint64_t EGLuint64KHR;
typedef int EGLNativeFileDescriptorKHR;
typedef khronos_ssize_t EGLsizeiANDROID;
typedef void (*EGLSetBlobFuncANDROID) (const void *key, EGLsizeiANDROID keySize, const void *value, EGLsizeiANDROID valueSize);
typedef EGLsizeiANDROID (*EGLGetBlobFuncANDROID) (const void *key, EGLsizeiANDROID keySize, void *value, EGLsizeiANDROID valueSize);
typedef void *EGLDeviceEXT;
typedef void *EGLOutputLayerEXT;
typedef void *EGLOutputPortEXT;
typedef void *EGLSyncNV;
typedef khronos_utime_nanoseconds_t EGLTimeNV;
typedef khronos_utime_nanoseconds_t EGLuint64NV;
typedef khronos_stime_nanoseconds_t EGLnsecsANDROID;

struct EGLClientPixmapHI;
struct AHardwareBuffer;

#define EGL_DONT_CARE                     ((EGLint)-1)

#define EGL_NO_CONTEXT                    ((EGLContext)0)
#define EGL_NO_DISPLAY                    ((EGLDisplay)0)
#define EGL_NO_IMAGE                      ((EGLImage)0)
#define EGL_NO_SURFACE                    ((EGLSurface)0)
#define EGL_NO_SYNC                       ((EGLSync)0)

#define EGL_UNKNOWN                       ((EGLint)-1)

#define EGL_DEFAULT_DISPLAY               ((EGLNativeDisplayType)0)

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY eglGetProcAddress (const char *procname);
/* ---------------------------- EGL_VERSION_1_0 ---------------------------- */

#ifndef EGL_VERSION_1_0
#define EGL_VERSION_1_0 1

#define EGL_FALSE 0
#define EGL_PBUFFER_BIT 0x0001
#define EGL_TRUE 1
#define EGL_PIXMAP_BIT 0x0002
#define EGL_WINDOW_BIT 0x0004
#define EGL_SUCCESS 0x3000
#define EGL_NOT_INITIALIZED 0x3001
#define EGL_BAD_ACCESS 0x3002
#define EGL_BAD_ALLOC 0x3003
#define EGL_BAD_ATTRIBUTE 0x3004
#define EGL_BAD_CONFIG 0x3005
#define EGL_BAD_CONTEXT 0x3006
#define EGL_BAD_CURRENT_SURFACE 0x3007
#define EGL_BAD_DISPLAY 0x3008
#define EGL_BAD_MATCH 0x3009
#define EGL_BAD_NATIVE_PIXMAP 0x300A
#define EGL_BAD_NATIVE_WINDOW 0x300B
#define EGL_BAD_PARAMETER 0x300C
#define EGL_BAD_SURFACE 0x300D
#define EGL_BUFFER_SIZE 0x3020
#define EGL_ALPHA_SIZE 0x3021
#define EGL_BLUE_SIZE 0x3022
#define EGL_GREEN_SIZE 0x3023
#define EGL_RED_SIZE 0x3024
#define EGL_DEPTH_SIZE 0x3025
#define EGL_STENCIL_SIZE 0x3026
#define EGL_CONFIG_CAVEAT 0x3027
#define EGL_CONFIG_ID 0x3028
#define EGL_LEVEL 0x3029
#define EGL_MAX_PBUFFER_HEIGHT 0x302A
#define EGL_MAX_PBUFFER_PIXELS 0x302B
#define EGL_MAX_PBUFFER_WIDTH 0x302C
#define EGL_NATIVE_RENDERABLE 0x302D
#define EGL_NATIVE_VISUAL_ID 0x302E
#define EGL_NATIVE_VISUAL_TYPE 0x302F
#define EGL_SAMPLES 0x3031
#define EGL_SAMPLE_BUFFERS 0x3032
#define EGL_SURFACE_TYPE 0x3033
#define EGL_TRANSPARENT_TYPE 0x3034
#define EGL_TRANSPARENT_BLUE_VALUE 0x3035
#define EGL_TRANSPARENT_GREEN_VALUE 0x3036
#define EGL_TRANSPARENT_RED_VALUE 0x3037
#define EGL_NONE 0x3038
#define EGL_SLOW_CONFIG 0x3050
#define EGL_NON_CONFORMANT_CONFIG 0x3051
#define EGL_TRANSPARENT_RGB 0x3052
#define EGL_VENDOR 0x3053
#define EGL_VERSION 0x3054
#define EGL_EXTENSIONS 0x3055
#define EGL_HEIGHT 0x3056
#define EGL_WIDTH 0x3057
#define EGL_LARGEST_PBUFFER 0x3058
#define EGL_DRAW 0x3059
#define EGL_READ 0x305A
#define EGL_CORE_NATIVE_ENGINE 0x305B

typedef EGLBoolean  ( * PFNEGLCHOOSECONFIGPROC) (EGLDisplay  dpy, const EGLint * attrib_list, EGLConfig * configs, EGLint  config_size, EGLint * num_config);
typedef EGLBoolean  ( * PFNEGLCOPYBUFFERSPROC) (EGLDisplay  dpy, EGLSurface  surface, EGLNativePixmapType  target);
typedef EGLContext  ( * PFNEGLCREATECONTEXTPROC) (EGLDisplay  dpy, EGLConfig  config, EGLContext  share_context, const EGLint * attrib_list);
typedef EGLSurface  ( * PFNEGLCREATEPBUFFERSURFACEPROC) (EGLDisplay  dpy, EGLConfig  config, const EGLint * attrib_list);
typedef EGLSurface  ( * PFNEGLCREATEPIXMAPSURFACEPROC) (EGLDisplay  dpy, EGLConfig  config, EGLNativePixmapType  pixmap, const EGLint * attrib_list);
typedef EGLSurface  ( * PFNEGLCREATEWINDOWSURFACEPROC) (EGLDisplay  dpy, EGLConfig  config, EGLNativeWindowType  win, const EGLint * attrib_list);
typedef EGLBoolean  ( * PFNEGLDESTROYCONTEXTPROC) (EGLDisplay  dpy, EGLContext  ctx);
typedef EGLBoolean  ( * PFNEGLDESTROYSURFACEPROC) (EGLDisplay  dpy, EGLSurface  surface);
typedef EGLBoolean  ( * PFNEGLGETCONFIGATTRIBPROC) (EGLDisplay  dpy, EGLConfig  config, EGLint  attribute, EGLint * value);
typedef EGLBoolean  ( * PFNEGLGETCONFIGSPROC) (EGLDisplay  dpy, EGLConfig * configs, EGLint  config_size, EGLint * num_config);
typedef EGLDisplay  ( * PFNEGLGETCURRENTDISPLAYPROC) ( void );
typedef EGLSurface  ( * PFNEGLGETCURRENTSURFACEPROC) (EGLint  readdraw);
typedef EGLDisplay  ( * PFNEGLGETDISPLAYPROC) (EGLNativeDisplayType  display_id);
typedef EGLint  ( * PFNEGLGETERRORPROC) ( void );
typedef EGLBoolean  ( * PFNEGLINITIALIZEPROC) (EGLDisplay  dpy, EGLint * major, EGLint * minor);
typedef EGLBoolean  ( * PFNEGLMAKECURRENTPROC) (EGLDisplay  dpy, EGLSurface  draw, EGLSurface  read, EGLContext  ctx);
typedef EGLBoolean  ( * PFNEGLQUERYCONTEXTPROC) (EGLDisplay  dpy, EGLContext  ctx, EGLint  attribute, EGLint * value);
typedef const char * ( * PFNEGLQUERYSTRINGPROC) (EGLDisplay  dpy, EGLint  name);
typedef EGLBoolean  ( * PFNEGLQUERYSURFACEPROC) (EGLDisplay  dpy, EGLSurface  surface, EGLint  attribute, EGLint * value);
typedef EGLBoolean  ( * PFNEGLSWAPBUFFERSPROC) (EGLDisplay  dpy, EGLSurface  surface);
typedef EGLBoolean  ( * PFNEGLTERMINATEPROC) (EGLDisplay  dpy);
typedef EGLBoolean  ( * PFNEGLWAITGLPROC) ( void );
typedef EGLBoolean  ( * PFNEGLWAITNATIVEPROC) (EGLint  engine);

#define eglChooseConfig EGLEW_GET_FUN(__eglewChooseConfig)
#define eglCopyBuffers EGLEW_GET_FUN(__eglewCopyBuffers)
#define eglCreateContext EGLEW_GET_FUN(__eglewCreateContext)
#define eglCreatePbufferSurface EGLEW_GET_FUN(__eglewCreatePbufferSurface)
#define eglCreatePixmapSurface EGLEW_GET_FUN(__eglewCreatePixmapSurface)
#define eglCreateWindowSurface EGLEW_GET_FUN(__eglewCreateWindowSurface)
#define eglDestroyContext EGLEW_GET_FUN(__eglewDestroyContext)
#define eglDestroySurface EGLEW_GET_FUN(__eglewDestroySurface)
#define eglGetConfigAttrib EGLEW_GET_FUN(__eglewGetConfigAttrib)
#define eglGetConfigs EGLEW_GET_FUN(__eglewGetConfigs)
#define eglGetCurrentDisplay EGLEW_GET_FUN(__eglewGetCurrentDisplay)
#define eglGetCurrentSurface EGLEW_GET_FUN(__eglewGetCurrentSurface)
#define eglGetDisplay EGLEW_GET_FUN(__eglewGetDisplay)
#define eglGetError EGLEW_GET_FUN(__eglewGetError)
#define eglInitialize EGLEW_GET_FUN(__eglewInitialize)
#define eglMakeCurrent EGLEW_GET_FUN(__eglewMakeCurrent)
#define eglQueryContext EGLEW_GET_FUN(__eglewQueryContext)
#define eglQueryString EGLEW_GET_FUN(__eglewQueryString)
#define eglQuerySurface EGLEW_GET_FUN(__eglewQuerySurface)
#define eglSwapBuffers EGLEW_GET_FUN(__eglewSwapBuffers)
#define eglTerminate EGLEW_GET_FUN(__eglewTerminate)
#define eglWaitGL EGLEW_GET_FUN(__eglewWaitGL)
#define eglWaitNative EGLEW_GET_FUN(__eglewWaitNative)

#define EGLEW_VERSION_1_0 EGLEW_GET_VAR(__EGLEW_VERSION_1_0)

#endif /* EGL_VERSION_1_0 */

/* ---------------------------- EGL_VERSION_1_1 ---------------------------- */

#ifndef EGL_VERSION_1_1
#define EGL_VERSION_1_1 1

#define EGL_CONTEXT_LOST 0x300E
#define EGL_BIND_TO_TEXTURE_RGB 0x3039
#define EGL_BIND_TO_TEXTURE_RGBA 0x303A
#define EGL_MIN_SWAP_INTERVAL 0x303B
#define EGL_MAX_SWAP_INTERVAL 0x303C
#define EGL_NO_TEXTURE 0x305C
#define EGL_TEXTURE_RGB 0x305D
#define EGL_TEXTURE_RGBA 0x305E
#define EGL_TEXTURE_2D 0x305F
#define EGL_TEXTURE_FORMAT 0x3080
#define EGL_TEXTURE_TARGET 0x3081
#define EGL_MIPMAP_TEXTURE 0x3082
#define EGL_MIPMAP_LEVEL 0x3083
#define EGL_BACK_BUFFER 0x3084

typedef EGLBoolean  ( * PFNEGLBINDTEXIMAGEPROC) (EGLDisplay  dpy, EGLSurface  surface, EGLint  buffer);
typedef EGLBoolean  ( * PFNEGLRELEASETEXIMAGEPROC) (EGLDisplay  dpy, EGLSurface  surface, EGLint  buffer);
typedef EGLBoolean  ( * PFNEGLSURFACEATTRIBPROC) (EGLDisplay  dpy, EGLSurface  surface, EGLint  attribute, EGLint  value);
typedef EGLBoolean  ( * PFNEGLSWAPINTERVALPROC) (EGLDisplay  dpy, EGLint  interval);

#define eglBindTexImage EGLEW_GET_FUN(__eglewBindTexImage)
#define eglReleaseTexImage EGLEW_GET_FUN(__eglewReleaseTexImage)
#define eglSurfaceAttrib EGLEW_GET_FUN(__eglewSurfaceAttrib)
#define eglSwapInterval EGLEW_GET_FUN(__eglewSwapInterval)

#define EGLEW_VERSION_1_1 EGLEW_GET_VAR(__EGLEW_VERSION_1_1)

#endif /* EGL_VERSION_1_1 */

/* ---------------------------- EGL_VERSION_1_2 ---------------------------- */

#ifndef EGL_VERSION_1_2
#define EGL_VERSION_1_2 1

#define EGL_OPENGL_ES_BIT 0x0001
#define EGL_OPENVG_BIT 0x0002
#define EGL_LUMINANCE_SIZE 0x303D
#define EGL_ALPHA_MASK_SIZE 0x303E
#define EGL_COLOR_BUFFER_TYPE 0x303F
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_SINGLE_BUFFER 0x3085
#define EGL_RENDER_BUFFER 0x3086
#define EGL_COLORSPACE 0x3087
#define EGL_ALPHA_FORMAT 0x3088
#define EGL_COLORSPACE_LINEAR 0x308A
#define EGL_ALPHA_FORMAT_NONPRE 0x308B
#define EGL_ALPHA_FORMAT_PRE 0x308C
#define EGL_CLIENT_APIS 0x308D
#define EGL_RGB_BUFFER 0x308E
#define EGL_LUMINANCE_BUFFER 0x308F
#define EGL_HORIZONTAL_RESOLUTION 0x3090
#define EGL_VERTICAL_RESOLUTION 0x3091
#define EGL_PIXEL_ASPECT_RATIO 0x3092
#define EGL_SWAP_BEHAVIOR 0x3093
#define EGL_BUFFER_PRESERVED 0x3094
#define EGL_BUFFER_DESTROYED 0x3095
#define EGL_OPENVG_IMAGE 0x3096
#define EGL_CONTEXT_CLIENT_TYPE 0x3097
#define EGL_OPENGL_ES_API 0x30A0
#define EGL_OPENVG_API 0x30A1
#define EGL_DISPLAY_SCALING 10000

typedef EGLBoolean  ( * PFNEGLBINDAPIPROC) (EGLenum  api);
typedef EGLSurface  ( * PFNEGLCREATEPBUFFERFROMCLIENTBUFFERPROC) (EGLDisplay  dpy, EGLenum  buftype, EGLClientBuffer  buffer, EGLConfig  config, const EGLint * attrib_list);
typedef EGLenum  ( * PFNEGLQUERYAPIPROC) ( void );
typedef EGLBoolean  ( * PFNEGLRELEASETHREADPROC) ( void );
typedef EGLBoolean  ( * PFNEGLWAITCLIENTPROC) ( void );

#define eglBindAPI EGLEW_GET_FUN(__eglewBindAPI)
#define eglCreatePbufferFromClientBuffer EGLEW_GET_FUN(__eglewCreatePbufferFromClientBuffer)
#define eglQueryAPI EGLEW_GET_FUN(__eglewQueryAPI)
#define eglReleaseThread EGLEW_GET_FUN(__eglewReleaseThread)
#define eglWaitClient EGLEW_GET_FUN(__eglewWaitClient)

#define EGLEW_VERSION_1_2 EGLEW_GET_VAR(__EGLEW_VERSION_1_2)

#endif /* EGL_VERSION_1_2 */

/* ---------------------------- EGL_VERSION_1_3 ---------------------------- */

#ifndef EGL_VERSION_1_3
#define EGL_VERSION_1_3 1

#define EGL_OPENGL_ES2_BIT 0x0004
#define EGL_VG_COLORSPACE_LINEAR_BIT 0x0020
#define EGL_VG_ALPHA_FORMAT_PRE_BIT 0x0040
#define EGL_MATCH_NATIVE_PIXMAP 0x3041
#define EGL_CONFORMANT 0x3042
#define EGL_VG_COLORSPACE 0x3087
#define EGL_VG_ALPHA_FORMAT 0x3088
#define EGL_VG_COLORSPACE_LINEAR 0x308A
#define EGL_VG_ALPHA_FORMAT_NONPRE 0x308B
#define EGL_VG_ALPHA_FORMAT_PRE 0x308C
#define EGL_CONTEXT_CLIENT_VERSION 0x3098

#define EGLEW_VERSION_1_3 EGLEW_GET_VAR(__EGLEW_VERSION_1_3)

#endif /* EGL_VERSION_1_3 */

/* ---------------------------- EGL_VERSION_1_4 ---------------------------- */

#ifndef EGL_VERSION_1_4
#define EGL_VERSION_1_4 1

#define EGL_OPENGL_BIT 0x0008
#define EGL_MULTISAMPLE_RESOLVE_BOX_BIT 0x0200
#define EGL_SWAP_BEHAVIOR_PRESERVED_BIT 0x0400
#define EGL_MULTISAMPLE_RESOLVE 0x3099
#define EGL_MULTISAMPLE_RESOLVE_DEFAULT 0x309A
#define EGL_MULTISAMPLE_RESOLVE_BOX 0x309B
#define EGL_OPENGL_API 0x30A2

typedef EGLContext  ( * PFNEGLGETCURRENTCONTEXTPROC) ( void );

#define eglGetCurrentContext EGLEW_GET_FUN(__eglewGetCurrentContext)

#define EGLEW_VERSION_1_4 EGLEW_GET_VAR(__EGLEW_VERSION_1_4)

#endif /* EGL_VERSION_1_4 */

/* ---------------------------- EGL_VERSION_1_5 ---------------------------- */

#ifndef EGL_VERSION_1_5
#define EGL_VERSION_1_5 1

#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT 0x00000001
#define EGL_SYNC_FLUSH_COMMANDS_BIT 0x0001
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT 0x00000002
#define EGL_OPENGL_ES3_BIT 0x00000040
#define EGL_GL_COLORSPACE_SRGB 0x3089
#define EGL_GL_COLORSPACE_LINEAR 0x308A
#define EGL_CONTEXT_MAJOR_VERSION 0x3098
#define EGL_CL_EVENT_HANDLE 0x309C
#define EGL_GL_COLORSPACE 0x309D
#define EGL_GL_TEXTURE_2D 0x30B1
#define EGL_GL_TEXTURE_3D 0x30B2
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x30B3
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x30B4
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x30B5
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x30B6
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x30B7
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x30B8
#define EGL_GL_RENDERBUFFER 0x30B9
#define EGL_GL_TEXTURE_LEVEL 0x30BC
#define EGL_GL_TEXTURE_ZOFFSET 0x30BD
#define EGL_IMAGE_PRESERVED 0x30D2
#define EGL_SYNC_PRIOR_COMMANDS_COMPLETE 0x30F0
#define EGL_SYNC_STATUS 0x30F1
#define EGL_SIGNALED 0x30F2
#define EGL_UNSIGNALED 0x30F3
#define EGL_TIMEOUT_EXPIRED 0x30F5
#define EGL_CONDITION_SATISFIED 0x30F6
#define EGL_SYNC_TYPE 0x30F7
#define EGL_SYNC_CONDITION 0x30F8
#define EGL_SYNC_FENCE 0x30F9
#define EGL_CONTEXT_MINOR_VERSION 0x30FB
#define EGL_CONTEXT_OPENGL_PROFILE_MASK 0x30FD
#define EGL_SYNC_CL_EVENT 0x30FE
#define EGL_SYNC_CL_EVENT_COMPLETE 0x30FF
#define EGL_CONTEXT_OPENGL_DEBUG 0x31B0
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE 0x31B1
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS 0x31B2
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY 0x31BD
#define EGL_NO_RESET_NOTIFICATION 0x31BE
#define EGL_LOSE_CONTEXT_ON_RESET 0x31BF
#define EGL_FOREVER 0xFFFFFFFFFFFFFFFF

typedef EGLint  ( * PFNEGLCLIENTWAITSYNCPROC) (EGLDisplay  dpy, EGLSync  sync, EGLint  flags, EGLTime  timeout);
typedef EGLImage  ( * PFNEGLCREATEIMAGEPROC) (EGLDisplay  dpy, EGLContext  ctx, EGLenum  target, EGLClientBuffer  buffer, const EGLAttrib * attrib_list);
typedef EGLSurface  ( * PFNEGLCREATEPLATFORMPIXMAPSURFACEPROC) (EGLDisplay  dpy, EGLConfig  config, void * native_pixmap, const EGLAttrib * attrib_list);
typedef EGLSurface  ( * PFNEGLCREATEPLATFORMWINDOWSURFACEPROC) (EGLDisplay  dpy, EGLConfig  config, void * native_window, const EGLAttrib * attrib_list);
typedef EGLSync  ( * PFNEGLCREATESYNCPROC) (EGLDisplay  dpy, EGLenum  type, const EGLAttrib * attrib_list);
typedef EGLBoolean  ( * PFNEGLDESTROYIMAGEPROC) (EGLDisplay  dpy, EGLImage  image);
typedef EGLBoolean  ( * PFNEGLDESTROYSYNCPROC) (EGLDisplay  dpy, EGLSync  sync);
typedef EGLDisplay  ( * PFNEGLGETPLATFORMDISPLAYPROC) (EGLenum  platform, void * native_display, const EGLAttrib * attrib_list);
typedef EGLBoolean  ( * PFNEGLGETSYNCATTRIBPROC) (EGLDisplay  dpy, EGLSync  sync, EGLint  attribute, EGLAttrib * value);
typedef EGLBoolean  ( * PFNEGLWAITSYNCPROC) (EGLDisplay  dpy, EGLSync  sync, EGLint  flags);

#define eglClientWaitSync EGLEW_GET_FUN(__eglewClientWaitSync)
#define eglCreateImage EGLEW_GET_FUN(__eglewCreateImage)
#define eglCreatePlatformPixmapSurface EGLEW_GET_FUN(__eglewCreatePlatformPixmapSurface)
#define eglCreatePlatformWindowSurface EGLEW_GET_FUN(__eglewCreatePlatformWindowSurface)
#define eglCreateSync EGLEW_GET_FUN(__eglewCreateSync)
#define eglDestroyImage EGLEW_GET_FUN(__eglewDestroyImage)
#define eglDestroySync EGLEW_GET_FUN(__eglewDestroySync)
#define eglGetPlatformDisplay EGLEW_GET_FUN(__eglewGetPlatformDisplay)
#define eglGetSyncAttrib EGLEW_GET_FUN(__eglewGetSyncAttrib)
#define eglWaitSync EGLEW_GET_FUN(__eglewWaitSync)

#define EGLEW_VERSION_1_5 EGLEW_GET_VAR(__EGLEW_VERSION_1_5)

#endif /* EGL_VERSION_1_5 */

/* ------------------------------------------------------------------------- */

#define EGLEW_FUN_EXPORT GLEW_FUN_EXPORT
#define EGLEW_VAR_EXPORT GLEW_VAR_EXPORT

EGLEW_FUN_EXPORT PFNEGLCHOOSECONFIGPROC __eglewChooseConfig;
EGLEW_FUN_EXPORT PFNEGLCOPYBUFFERSPROC __eglewCopyBuffers;
EGLEW_FUN_EXPORT PFNEGLCREATECONTEXTPROC __eglewCreateContext;
EGLEW_FUN_EXPORT PFNEGLCREATEPBUFFERSURFACEPROC __eglewCreatePbufferSurface;
EGLEW_FUN_EXPORT PFNEGLCREATEPIXMAPSURFACEPROC __eglewCreatePixmapSurface;
EGLEW_FUN_EXPORT PFNEGLCREATEWINDOWSURFACEPROC __eglewCreateWindowSurface;
EGLEW_FUN_EXPORT PFNEGLDESTROYCONTEXTPROC __eglewDestroyContext;
EGLEW_FUN_EXPORT PFNEGLDESTROYSURFACEPROC __eglewDestroySurface;
EGLEW_FUN_EXPORT PFNEGLGETCONFIGATTRIBPROC __eglewGetConfigAttrib;
EGLEW_FUN_EXPORT PFNEGLGETCONFIGSPROC __eglewGetConfigs;
EGLEW_FUN_EXPORT PFNEGLGETCURRENTDISPLAYPROC __eglewGetCurrentDisplay;
EGLEW_FUN_EXPORT PFNEGLGETCURRENTSURFACEPROC __eglewGetCurrentSurface;
EGLEW_FUN_EXPORT PFNEGLGETDISPLAYPROC __eglewGetDisplay;
EGLEW_FUN_EXPORT PFNEGLGETERRORPROC __eglewGetError;
EGLEW_FUN_EXPORT PFNEGLINITIALIZEPROC __eglewInitialize;
EGLEW_FUN_EXPORT PFNEGLMAKECURRENTPROC __eglewMakeCurrent;
EGLEW_FUN_EXPORT PFNEGLQUERYCONTEXTPROC __eglewQueryContext;
EGLEW_FUN_EXPORT PFNEGLQUERYSTRINGPROC __eglewQueryString;
EGLEW_FUN_EXPORT PFNEGLQUERYSURFACEPROC __eglewQuerySurface;
EGLEW_FUN_EXPORT PFNEGLSWAPBUFFERSPROC __eglewSwapBuffers;
EGLEW_FUN_EXPORT PFNEGLTERMINATEPROC __eglewTerminate;
EGLEW_FUN_EXPORT PFNEGLWAITGLPROC __eglewWaitGL;
EGLEW_FUN_EXPORT PFNEGLWAITNATIVEPROC __eglewWaitNative;

EGLEW_FUN_EXPORT PFNEGLBINDTEXIMAGEPROC __eglewBindTexImage;
EGLEW_FUN_EXPORT PFNEGLRELEASETEXIMAGEPROC __eglewReleaseTexImage;
EGLEW_FUN_EXPORT PFNEGLSURFACEATTRIBPROC __eglewSurfaceAttrib;
EGLEW_FUN_EXPORT PFNEGLSWAPINTERVALPROC __eglewSwapInterval;

EGLEW_FUN_EXPORT PFNEGLBINDAPIPROC __eglewBindAPI;
EGLEW_FUN_EXPORT PFNEGLCREATEPBUFFERFROMCLIENTBUFFERPROC __eglewCreatePbufferFromClientBuffer;
EGLEW_FUN_EXPORT PFNEGLQUERYAPIPROC __eglewQueryAPI;
EGLEW_FUN_EXPORT PFNEGLRELEASETHREADPROC __eglewReleaseThread;
EGLEW_FUN_EXPORT PFNEGLWAITCLIENTPROC __eglewWaitClient;

EGLEW_FUN_EXPORT PFNEGLGETCURRENTCONTEXTPROC __eglewGetCurrentContext;

EGLEW_FUN_EXPORT PFNEGLCLIENTWAITSYNCPROC __eglewClientWaitSync;
EGLEW_FUN_EXPORT PFNEGLCREATEIMAGEPROC __eglewCreateImage;
EGLEW_FUN_EXPORT PFNEGLCREATEPLATFORMPIXMAPSURFACEPROC __eglewCreatePlatformPixmapSurface;
EGLEW_FUN_EXPORT PFNEGLCREATEPLATFORMWINDOWSURFACEPROC __eglewCreatePlatformWindowSurface;
EGLEW_FUN_EXPORT PFNEGLCREATESYNCPROC __eglewCreateSync;
EGLEW_FUN_EXPORT PFNEGLDESTROYIMAGEPROC __eglewDestroyImage;
EGLEW_FUN_EXPORT PFNEGLDESTROYSYNCPROC __eglewDestroySync;
EGLEW_FUN_EXPORT PFNEGLGETPLATFORMDISPLAYPROC __eglewGetPlatformDisplay;
EGLEW_FUN_EXPORT PFNEGLGETSYNCATTRIBPROC __eglewGetSyncAttrib;
EGLEW_FUN_EXPORT PFNEGLWAITSYNCPROC __eglewWaitSync;
EGLEW_VAR_EXPORT GLboolean __EGLEW_VERSION_1_0;
EGLEW_VAR_EXPORT GLboolean __EGLEW_VERSION_1_1;
EGLEW_VAR_EXPORT GLboolean __EGLEW_VERSION_1_2;
EGLEW_VAR_EXPORT GLboolean __EGLEW_VERSION_1_3;
EGLEW_VAR_EXPORT GLboolean __EGLEW_VERSION_1_4;
EGLEW_VAR_EXPORT GLboolean __EGLEW_VERSION_1_5;
/* ------------------------------------------------------------------------ */

GLEWAPI GLenum GLEWAPIENTRY eglewInit (EGLDisplay display);
GLEWAPI GLboolean GLEWAPIENTRY eglewIsSupported (const char *name);

#define EGLEW_GET_VAR(x) (*(const GLboolean*)&x)
#define EGLEW_GET_FUN(x) x

GLEWAPI GLboolean GLEWAPIENTRY eglewGetExtension (const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __eglew_h__ */
