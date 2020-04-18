/**************************************************************************
 *
 * Copyright 2010 LunarG, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include <string.h>
#include "egltypedefs.h"
#include "egldriver.h"
#include "eglconfig.h"
#include "eglcontext.h"
#include "eglsurface.h"
#include "eglmisc.h"
#include "eglscreen.h"
#include "eglmode.h"
#include "eglsync.h"


static EGLBoolean
_eglReturnFalse(void)
{
   return EGL_FALSE;
}


/**
 * Plug all the available fallback routines into the given driver's
 * dispatch table.
 */
void
_eglInitDriverFallbacks(_EGLDriver *drv)
{
   memset(&drv->API, 0, sizeof(drv->API));

   /* the driver has to implement these */
   drv->API.Initialize = NULL;
   drv->API.Terminate = NULL;

   drv->API.GetConfigs = _eglGetConfigs;
   drv->API.ChooseConfig = _eglChooseConfig;
   drv->API.GetConfigAttrib = _eglGetConfigAttrib;

   drv->API.CreateContext = (CreateContext_t) _eglReturnFalse;
   drv->API.DestroyContext = (DestroyContext_t) _eglReturnFalse;
   drv->API.MakeCurrent = (MakeCurrent_t) _eglReturnFalse;
   drv->API.QueryContext = _eglQueryContext;

   drv->API.CreateWindowSurface = (CreateWindowSurface_t) _eglReturnFalse;
   drv->API.CreatePixmapSurface = (CreatePixmapSurface_t) _eglReturnFalse;
   drv->API.CreatePbufferSurface = (CreatePbufferSurface_t) _eglReturnFalse;
   drv->API.CreatePbufferFromClientBuffer =
      (CreatePbufferFromClientBuffer_t) _eglReturnFalse;
   drv->API.DestroySurface = (DestroySurface_t) _eglReturnFalse;
   drv->API.QuerySurface = _eglQuerySurface;
   drv->API.SurfaceAttrib = _eglSurfaceAttrib;

   drv->API.BindTexImage = (BindTexImage_t) _eglReturnFalse;
   drv->API.ReleaseTexImage = (ReleaseTexImage_t) _eglReturnFalse;
   drv->API.CopyBuffers = (CopyBuffers_t) _eglReturnFalse;
   drv->API.SwapBuffers = (SwapBuffers_t) _eglReturnFalse;
   drv->API.SwapInterval = _eglSwapInterval;

   drv->API.WaitClient = (WaitClient_t) _eglReturnFalse;
   drv->API.WaitNative = (WaitNative_t) _eglReturnFalse;
   drv->API.GetProcAddress = (GetProcAddress_t) _eglReturnFalse;
   drv->API.QueryString = _eglQueryString;

#ifdef EGL_MESA_screen_surface
   drv->API.CopyContextMESA = (CopyContextMESA_t) _eglReturnFalse;
   drv->API.CreateScreenSurfaceMESA =
      (CreateScreenSurfaceMESA_t) _eglReturnFalse;
   drv->API.ShowScreenSurfaceMESA = (ShowScreenSurfaceMESA_t) _eglReturnFalse;
   drv->API.ChooseModeMESA = _eglChooseModeMESA;
   drv->API.GetModesMESA = _eglGetModesMESA;
   drv->API.GetModeAttribMESA = _eglGetModeAttribMESA;
   drv->API.GetScreensMESA = _eglGetScreensMESA;
   drv->API.ScreenPositionMESA = _eglScreenPositionMESA;
   drv->API.QueryScreenMESA = _eglQueryScreenMESA;
   drv->API.QueryScreenSurfaceMESA = _eglQueryScreenSurfaceMESA;
   drv->API.QueryScreenModeMESA = _eglQueryScreenModeMESA;
   drv->API.QueryModeStringMESA = _eglQueryModeStringMESA;
#endif /* EGL_MESA_screen_surface */

   drv->API.CreateImageKHR = NULL;
   drv->API.DestroyImageKHR = NULL;

   drv->API.CreateSyncKHR = NULL;
   drv->API.DestroySyncKHR = NULL;
   drv->API.ClientWaitSyncKHR = NULL;
   drv->API.SignalSyncKHR = NULL;
   drv->API.GetSyncAttribKHR = _eglGetSyncAttribKHR;

#ifdef EGL_MESA_drm_image
   drv->API.CreateDRMImageMESA = NULL;
   drv->API.ExportDRMImageMESA = NULL;
#endif

#ifdef EGL_NOK_swap_region
   drv->API.SwapBuffersRegionNOK = NULL;
#endif
}
