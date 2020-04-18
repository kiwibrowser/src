// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file is auto-generated from
// ui/gl/generate_bindings.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

#include <string>

#include "base/trace_event/trace_event.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_enums.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_version_info.h"
#include "ui/gl/gl_wgl_api_implementation.h"

namespace gl {

DriverWGL g_driver_wgl;  // Exists in .bss

void DriverWGL::InitializeStaticBindings() {
  // Ensure struct has been zero-initialized.
  char* this_bytes = reinterpret_cast<char*>(this);
  DCHECK(this_bytes[0] == 0);
  DCHECK(memcmp(this_bytes, this_bytes + 1, sizeof(*this) - 1) == 0);

  fn.wglCopyContextFn =
      reinterpret_cast<wglCopyContextProc>(GetGLProcAddress("wglCopyContext"));
  fn.wglCreateContextFn = reinterpret_cast<wglCreateContextProc>(
      GetGLProcAddress("wglCreateContext"));
  fn.wglCreateLayerContextFn = reinterpret_cast<wglCreateLayerContextProc>(
      GetGLProcAddress("wglCreateLayerContext"));
  fn.wglDeleteContextFn = reinterpret_cast<wglDeleteContextProc>(
      GetGLProcAddress("wglDeleteContext"));
  fn.wglGetCurrentContextFn = reinterpret_cast<wglGetCurrentContextProc>(
      GetGLProcAddress("wglGetCurrentContext"));
  fn.wglGetCurrentDCFn = reinterpret_cast<wglGetCurrentDCProc>(
      GetGLProcAddress("wglGetCurrentDC"));
  fn.wglGetExtensionsStringARBFn =
      reinterpret_cast<wglGetExtensionsStringARBProc>(
          GetGLProcAddress("wglGetExtensionsStringARB"));
  fn.wglGetExtensionsStringEXTFn =
      reinterpret_cast<wglGetExtensionsStringEXTProc>(
          GetGLProcAddress("wglGetExtensionsStringEXT"));
  fn.wglMakeCurrentFn =
      reinterpret_cast<wglMakeCurrentProc>(GetGLProcAddress("wglMakeCurrent"));
  fn.wglShareListsFn =
      reinterpret_cast<wglShareListsProc>(GetGLProcAddress("wglShareLists"));
  fn.wglSwapLayerBuffersFn = reinterpret_cast<wglSwapLayerBuffersProc>(
      GetGLProcAddress("wglSwapLayerBuffers"));
}

void DriverWGL::InitializeExtensionBindings() {
  std::string platform_extensions(GetPlatformExtensions());
  ExtensionSet extensions(MakeExtensionSet(platform_extensions));
  ALLOW_UNUSED_LOCAL(extensions);

  ext.b_WGL_ARB_create_context =
      HasExtension(extensions, "WGL_ARB_create_context");
  ext.b_WGL_ARB_extensions_string =
      HasExtension(extensions, "WGL_ARB_extensions_string");
  ext.b_WGL_ARB_pbuffer = HasExtension(extensions, "WGL_ARB_pbuffer");
  ext.b_WGL_ARB_pixel_format = HasExtension(extensions, "WGL_ARB_pixel_format");
  ext.b_WGL_EXT_extensions_string =
      HasExtension(extensions, "WGL_EXT_extensions_string");
  ext.b_WGL_EXT_swap_control = HasExtension(extensions, "WGL_EXT_swap_control");

  if (ext.b_WGL_ARB_pixel_format) {
    fn.wglChoosePixelFormatARBFn =
        reinterpret_cast<wglChoosePixelFormatARBProc>(
            GetGLProcAddress("wglChoosePixelFormatARB"));
  }

  if (ext.b_WGL_ARB_create_context) {
    fn.wglCreateContextAttribsARBFn =
        reinterpret_cast<wglCreateContextAttribsARBProc>(
            GetGLProcAddress("wglCreateContextAttribsARB"));
  }

  if (ext.b_WGL_ARB_pbuffer) {
    fn.wglCreatePbufferARBFn = reinterpret_cast<wglCreatePbufferARBProc>(
        GetGLProcAddress("wglCreatePbufferARB"));
  }

  if (ext.b_WGL_ARB_pbuffer) {
    fn.wglDestroyPbufferARBFn = reinterpret_cast<wglDestroyPbufferARBProc>(
        GetGLProcAddress("wglDestroyPbufferARB"));
  }

  if (ext.b_WGL_ARB_pbuffer) {
    fn.wglGetPbufferDCARBFn = reinterpret_cast<wglGetPbufferDCARBProc>(
        GetGLProcAddress("wglGetPbufferDCARB"));
  }

  if (ext.b_WGL_ARB_pbuffer) {
    fn.wglQueryPbufferARBFn = reinterpret_cast<wglQueryPbufferARBProc>(
        GetGLProcAddress("wglQueryPbufferARB"));
  }

  if (ext.b_WGL_ARB_pbuffer) {
    fn.wglReleasePbufferDCARBFn = reinterpret_cast<wglReleasePbufferDCARBProc>(
        GetGLProcAddress("wglReleasePbufferDCARB"));
  }

  if (ext.b_WGL_EXT_swap_control) {
    fn.wglSwapIntervalEXTFn = reinterpret_cast<wglSwapIntervalEXTProc>(
        GetGLProcAddress("wglSwapIntervalEXT"));
  }
}

void DriverWGL::ClearBindings() {
  memset(this, 0, sizeof(*this));
}

BOOL WGLApiBase::wglChoosePixelFormatARBFn(HDC dc,
                                           const int* int_attrib_list,
                                           const float* float_attrib_list,
                                           UINT max_formats,
                                           int* formats,
                                           UINT* num_formats) {
  return driver_->fn.wglChoosePixelFormatARBFn(dc, int_attrib_list,
                                               float_attrib_list, max_formats,
                                               formats, num_formats);
}

BOOL WGLApiBase::wglCopyContextFn(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask) {
  return driver_->fn.wglCopyContextFn(hglrcSrc, hglrcDst, mask);
}

HGLRC WGLApiBase::wglCreateContextFn(HDC hdc) {
  return driver_->fn.wglCreateContextFn(hdc);
}

HGLRC WGLApiBase::wglCreateContextAttribsARBFn(HDC hDC,
                                               HGLRC hShareContext,
                                               const int* attribList) {
  return driver_->fn.wglCreateContextAttribsARBFn(hDC, hShareContext,
                                                  attribList);
}

HGLRC WGLApiBase::wglCreateLayerContextFn(HDC hdc, int iLayerPlane) {
  return driver_->fn.wglCreateLayerContextFn(hdc, iLayerPlane);
}

HPBUFFERARB WGLApiBase::wglCreatePbufferARBFn(HDC hDC,
                                              int iPixelFormat,
                                              int iWidth,
                                              int iHeight,
                                              const int* piAttribList) {
  return driver_->fn.wglCreatePbufferARBFn(hDC, iPixelFormat, iWidth, iHeight,
                                           piAttribList);
}

BOOL WGLApiBase::wglDeleteContextFn(HGLRC hglrc) {
  return driver_->fn.wglDeleteContextFn(hglrc);
}

BOOL WGLApiBase::wglDestroyPbufferARBFn(HPBUFFERARB hPbuffer) {
  return driver_->fn.wglDestroyPbufferARBFn(hPbuffer);
}

HGLRC WGLApiBase::wglGetCurrentContextFn() {
  return driver_->fn.wglGetCurrentContextFn();
}

HDC WGLApiBase::wglGetCurrentDCFn() {
  return driver_->fn.wglGetCurrentDCFn();
}

const char* WGLApiBase::wglGetExtensionsStringARBFn(HDC hDC) {
  return driver_->fn.wglGetExtensionsStringARBFn(hDC);
}

const char* WGLApiBase::wglGetExtensionsStringEXTFn() {
  return driver_->fn.wglGetExtensionsStringEXTFn();
}

HDC WGLApiBase::wglGetPbufferDCARBFn(HPBUFFERARB hPbuffer) {
  return driver_->fn.wglGetPbufferDCARBFn(hPbuffer);
}

BOOL WGLApiBase::wglMakeCurrentFn(HDC hdc, HGLRC hglrc) {
  return driver_->fn.wglMakeCurrentFn(hdc, hglrc);
}

BOOL WGLApiBase::wglQueryPbufferARBFn(HPBUFFERARB hPbuffer,
                                      int iAttribute,
                                      int* piValue) {
  return driver_->fn.wglQueryPbufferARBFn(hPbuffer, iAttribute, piValue);
}

int WGLApiBase::wglReleasePbufferDCARBFn(HPBUFFERARB hPbuffer, HDC hDC) {
  return driver_->fn.wglReleasePbufferDCARBFn(hPbuffer, hDC);
}

BOOL WGLApiBase::wglShareListsFn(HGLRC hglrc1, HGLRC hglrc2) {
  return driver_->fn.wglShareListsFn(hglrc1, hglrc2);
}

BOOL WGLApiBase::wglSwapIntervalEXTFn(int interval) {
  return driver_->fn.wglSwapIntervalEXTFn(interval);
}

BOOL WGLApiBase::wglSwapLayerBuffersFn(HDC hdc, UINT fuPlanes) {
  return driver_->fn.wglSwapLayerBuffersFn(hdc, fuPlanes);
}

BOOL TraceWGLApi::wglChoosePixelFormatARBFn(HDC dc,
                                            const int* int_attrib_list,
                                            const float* float_attrib_list,
                                            UINT max_formats,
                                            int* formats,
                                            UINT* num_formats) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglChoosePixelFormatARB")
  return wgl_api_->wglChoosePixelFormatARBFn(dc, int_attrib_list,
                                             float_attrib_list, max_formats,
                                             formats, num_formats);
}

BOOL TraceWGLApi::wglCopyContextFn(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglCopyContext")
  return wgl_api_->wglCopyContextFn(hglrcSrc, hglrcDst, mask);
}

HGLRC TraceWGLApi::wglCreateContextFn(HDC hdc) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglCreateContext")
  return wgl_api_->wglCreateContextFn(hdc);
}

HGLRC TraceWGLApi::wglCreateContextAttribsARBFn(HDC hDC,
                                                HGLRC hShareContext,
                                                const int* attribList) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglCreateContextAttribsARB")
  return wgl_api_->wglCreateContextAttribsARBFn(hDC, hShareContext, attribList);
}

HGLRC TraceWGLApi::wglCreateLayerContextFn(HDC hdc, int iLayerPlane) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglCreateLayerContext")
  return wgl_api_->wglCreateLayerContextFn(hdc, iLayerPlane);
}

HPBUFFERARB TraceWGLApi::wglCreatePbufferARBFn(HDC hDC,
                                               int iPixelFormat,
                                               int iWidth,
                                               int iHeight,
                                               const int* piAttribList) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglCreatePbufferARB")
  return wgl_api_->wglCreatePbufferARBFn(hDC, iPixelFormat, iWidth, iHeight,
                                         piAttribList);
}

BOOL TraceWGLApi::wglDeleteContextFn(HGLRC hglrc) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglDeleteContext")
  return wgl_api_->wglDeleteContextFn(hglrc);
}

BOOL TraceWGLApi::wglDestroyPbufferARBFn(HPBUFFERARB hPbuffer) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglDestroyPbufferARB")
  return wgl_api_->wglDestroyPbufferARBFn(hPbuffer);
}

HGLRC TraceWGLApi::wglGetCurrentContextFn() {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglGetCurrentContext")
  return wgl_api_->wglGetCurrentContextFn();
}

HDC TraceWGLApi::wglGetCurrentDCFn() {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglGetCurrentDC")
  return wgl_api_->wglGetCurrentDCFn();
}

const char* TraceWGLApi::wglGetExtensionsStringARBFn(HDC hDC) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglGetExtensionsStringARB")
  return wgl_api_->wglGetExtensionsStringARBFn(hDC);
}

const char* TraceWGLApi::wglGetExtensionsStringEXTFn() {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglGetExtensionsStringEXT")
  return wgl_api_->wglGetExtensionsStringEXTFn();
}

HDC TraceWGLApi::wglGetPbufferDCARBFn(HPBUFFERARB hPbuffer) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglGetPbufferDCARB")
  return wgl_api_->wglGetPbufferDCARBFn(hPbuffer);
}

BOOL TraceWGLApi::wglMakeCurrentFn(HDC hdc, HGLRC hglrc) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglMakeCurrent")
  return wgl_api_->wglMakeCurrentFn(hdc, hglrc);
}

BOOL TraceWGLApi::wglQueryPbufferARBFn(HPBUFFERARB hPbuffer,
                                       int iAttribute,
                                       int* piValue) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglQueryPbufferARB")
  return wgl_api_->wglQueryPbufferARBFn(hPbuffer, iAttribute, piValue);
}

int TraceWGLApi::wglReleasePbufferDCARBFn(HPBUFFERARB hPbuffer, HDC hDC) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglReleasePbufferDCARB")
  return wgl_api_->wglReleasePbufferDCARBFn(hPbuffer, hDC);
}

BOOL TraceWGLApi::wglShareListsFn(HGLRC hglrc1, HGLRC hglrc2) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglShareLists")
  return wgl_api_->wglShareListsFn(hglrc1, hglrc2);
}

BOOL TraceWGLApi::wglSwapIntervalEXTFn(int interval) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglSwapIntervalEXT")
  return wgl_api_->wglSwapIntervalEXTFn(interval);
}

BOOL TraceWGLApi::wglSwapLayerBuffersFn(HDC hdc, UINT fuPlanes) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::wglSwapLayerBuffers")
  return wgl_api_->wglSwapLayerBuffersFn(hdc, fuPlanes);
}

BOOL DebugWGLApi::wglChoosePixelFormatARBFn(HDC dc,
                                            const int* int_attrib_list,
                                            const float* float_attrib_list,
                                            UINT max_formats,
                                            int* formats,
                                            UINT* num_formats) {
  GL_SERVICE_LOG("wglChoosePixelFormatARB"
                 << "(" << dc << ", "
                 << static_cast<const void*>(int_attrib_list) << ", "
                 << static_cast<const void*>(float_attrib_list) << ", "
                 << max_formats << ", " << static_cast<const void*>(formats)
                 << ", " << static_cast<const void*>(num_formats) << ")");
  BOOL result = wgl_api_->wglChoosePixelFormatARBFn(
      dc, int_attrib_list, float_attrib_list, max_formats, formats,
      num_formats);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

BOOL DebugWGLApi::wglCopyContextFn(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask) {
  GL_SERVICE_LOG("wglCopyContext"
                 << "(" << hglrcSrc << ", " << hglrcDst << ", " << mask << ")");
  BOOL result = wgl_api_->wglCopyContextFn(hglrcSrc, hglrcDst, mask);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

HGLRC DebugWGLApi::wglCreateContextFn(HDC hdc) {
  GL_SERVICE_LOG("wglCreateContext"
                 << "(" << hdc << ")");
  HGLRC result = wgl_api_->wglCreateContextFn(hdc);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

HGLRC DebugWGLApi::wglCreateContextAttribsARBFn(HDC hDC,
                                                HGLRC hShareContext,
                                                const int* attribList) {
  GL_SERVICE_LOG("wglCreateContextAttribsARB"
                 << "(" << hDC << ", " << hShareContext << ", "
                 << static_cast<const void*>(attribList) << ")");
  HGLRC result =
      wgl_api_->wglCreateContextAttribsARBFn(hDC, hShareContext, attribList);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

HGLRC DebugWGLApi::wglCreateLayerContextFn(HDC hdc, int iLayerPlane) {
  GL_SERVICE_LOG("wglCreateLayerContext"
                 << "(" << hdc << ", " << iLayerPlane << ")");
  HGLRC result = wgl_api_->wglCreateLayerContextFn(hdc, iLayerPlane);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

HPBUFFERARB DebugWGLApi::wglCreatePbufferARBFn(HDC hDC,
                                               int iPixelFormat,
                                               int iWidth,
                                               int iHeight,
                                               const int* piAttribList) {
  GL_SERVICE_LOG("wglCreatePbufferARB"
                 << "(" << hDC << ", " << iPixelFormat << ", " << iWidth << ", "
                 << iHeight << ", " << static_cast<const void*>(piAttribList)
                 << ")");
  HPBUFFERARB result = wgl_api_->wglCreatePbufferARBFn(
      hDC, iPixelFormat, iWidth, iHeight, piAttribList);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

BOOL DebugWGLApi::wglDeleteContextFn(HGLRC hglrc) {
  GL_SERVICE_LOG("wglDeleteContext"
                 << "(" << hglrc << ")");
  BOOL result = wgl_api_->wglDeleteContextFn(hglrc);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

BOOL DebugWGLApi::wglDestroyPbufferARBFn(HPBUFFERARB hPbuffer) {
  GL_SERVICE_LOG("wglDestroyPbufferARB"
                 << "(" << hPbuffer << ")");
  BOOL result = wgl_api_->wglDestroyPbufferARBFn(hPbuffer);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

HGLRC DebugWGLApi::wglGetCurrentContextFn() {
  GL_SERVICE_LOG("wglGetCurrentContext"
                 << "("
                 << ")");
  HGLRC result = wgl_api_->wglGetCurrentContextFn();
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

HDC DebugWGLApi::wglGetCurrentDCFn() {
  GL_SERVICE_LOG("wglGetCurrentDC"
                 << "("
                 << ")");
  HDC result = wgl_api_->wglGetCurrentDCFn();
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

const char* DebugWGLApi::wglGetExtensionsStringARBFn(HDC hDC) {
  GL_SERVICE_LOG("wglGetExtensionsStringARB"
                 << "(" << hDC << ")");
  const char* result = wgl_api_->wglGetExtensionsStringARBFn(hDC);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

const char* DebugWGLApi::wglGetExtensionsStringEXTFn() {
  GL_SERVICE_LOG("wglGetExtensionsStringEXT"
                 << "("
                 << ")");
  const char* result = wgl_api_->wglGetExtensionsStringEXTFn();
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

HDC DebugWGLApi::wglGetPbufferDCARBFn(HPBUFFERARB hPbuffer) {
  GL_SERVICE_LOG("wglGetPbufferDCARB"
                 << "(" << hPbuffer << ")");
  HDC result = wgl_api_->wglGetPbufferDCARBFn(hPbuffer);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

BOOL DebugWGLApi::wglMakeCurrentFn(HDC hdc, HGLRC hglrc) {
  GL_SERVICE_LOG("wglMakeCurrent"
                 << "(" << hdc << ", " << hglrc << ")");
  BOOL result = wgl_api_->wglMakeCurrentFn(hdc, hglrc);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

BOOL DebugWGLApi::wglQueryPbufferARBFn(HPBUFFERARB hPbuffer,
                                       int iAttribute,
                                       int* piValue) {
  GL_SERVICE_LOG("wglQueryPbufferARB"
                 << "(" << hPbuffer << ", " << iAttribute << ", "
                 << static_cast<const void*>(piValue) << ")");
  BOOL result = wgl_api_->wglQueryPbufferARBFn(hPbuffer, iAttribute, piValue);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

int DebugWGLApi::wglReleasePbufferDCARBFn(HPBUFFERARB hPbuffer, HDC hDC) {
  GL_SERVICE_LOG("wglReleasePbufferDCARB"
                 << "(" << hPbuffer << ", " << hDC << ")");
  int result = wgl_api_->wglReleasePbufferDCARBFn(hPbuffer, hDC);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

BOOL DebugWGLApi::wglShareListsFn(HGLRC hglrc1, HGLRC hglrc2) {
  GL_SERVICE_LOG("wglShareLists"
                 << "(" << hglrc1 << ", " << hglrc2 << ")");
  BOOL result = wgl_api_->wglShareListsFn(hglrc1, hglrc2);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

BOOL DebugWGLApi::wglSwapIntervalEXTFn(int interval) {
  GL_SERVICE_LOG("wglSwapIntervalEXT"
                 << "(" << interval << ")");
  BOOL result = wgl_api_->wglSwapIntervalEXTFn(interval);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

BOOL DebugWGLApi::wglSwapLayerBuffersFn(HDC hdc, UINT fuPlanes) {
  GL_SERVICE_LOG("wglSwapLayerBuffers"
                 << "(" << hdc << ", " << fuPlanes << ")");
  BOOL result = wgl_api_->wglSwapLayerBuffersFn(hdc, fuPlanes);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

}  // namespace gl
