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
#include "ui/gl/gl_osmesa_api_implementation.h"
#include "ui/gl/gl_version_info.h"

namespace gl {

DriverOSMESA g_driver_osmesa;  // Exists in .bss

void DriverOSMESA::InitializeStaticBindings() {
  // Ensure struct has been zero-initialized.
  char* this_bytes = reinterpret_cast<char*>(this);
  DCHECK(this_bytes[0] == 0);
  DCHECK(memcmp(this_bytes, this_bytes + 1, sizeof(*this) - 1) == 0);

  fn.OSMesaColorClampFn = reinterpret_cast<OSMesaColorClampProc>(
      GetGLProcAddress("OSMesaColorClamp"));
  fn.OSMesaCreateContextFn = reinterpret_cast<OSMesaCreateContextProc>(
      GetGLProcAddress("OSMesaCreateContext"));
  fn.OSMesaCreateContextExtFn = reinterpret_cast<OSMesaCreateContextExtProc>(
      GetGLProcAddress("OSMesaCreateContextExt"));
  fn.OSMesaDestroyContextFn = reinterpret_cast<OSMesaDestroyContextProc>(
      GetGLProcAddress("OSMesaDestroyContext"));
  fn.OSMesaGetColorBufferFn = reinterpret_cast<OSMesaGetColorBufferProc>(
      GetGLProcAddress("OSMesaGetColorBuffer"));
  fn.OSMesaGetCurrentContextFn = reinterpret_cast<OSMesaGetCurrentContextProc>(
      GetGLProcAddress("OSMesaGetCurrentContext"));
  fn.OSMesaGetDepthBufferFn = reinterpret_cast<OSMesaGetDepthBufferProc>(
      GetGLProcAddress("OSMesaGetDepthBuffer"));
  fn.OSMesaGetIntegervFn = reinterpret_cast<OSMesaGetIntegervProc>(
      GetGLProcAddress("OSMesaGetIntegerv"));
  fn.OSMesaGetProcAddressFn = reinterpret_cast<OSMesaGetProcAddressProc>(
      GetGLProcAddress("OSMesaGetProcAddress"));
  fn.OSMesaMakeCurrentFn = reinterpret_cast<OSMesaMakeCurrentProc>(
      GetGLProcAddress("OSMesaMakeCurrent"));
  fn.OSMesaPixelStoreFn = reinterpret_cast<OSMesaPixelStoreProc>(
      GetGLProcAddress("OSMesaPixelStore"));
}

void DriverOSMESA::InitializeExtensionBindings() {
  std::string platform_extensions(GetPlatformExtensions());
  ExtensionSet extensions(MakeExtensionSet(platform_extensions));
  ALLOW_UNUSED_LOCAL(extensions);
}

void DriverOSMESA::ClearBindings() {
  memset(this, 0, sizeof(*this));
}

void OSMESAApiBase::OSMesaColorClampFn(GLboolean enable) {
  driver_->fn.OSMesaColorClampFn(enable);
}

OSMesaContext OSMESAApiBase::OSMesaCreateContextFn(GLenum format,
                                                   OSMesaContext sharelist) {
  return driver_->fn.OSMesaCreateContextFn(format, sharelist);
}

OSMesaContext OSMESAApiBase::OSMesaCreateContextExtFn(GLenum format,
                                                      GLint depthBits,
                                                      GLint stencilBits,
                                                      GLint accumBits,
                                                      OSMesaContext sharelist) {
  return driver_->fn.OSMesaCreateContextExtFn(format, depthBits, stencilBits,
                                              accumBits, sharelist);
}

void OSMESAApiBase::OSMesaDestroyContextFn(OSMesaContext ctx) {
  driver_->fn.OSMesaDestroyContextFn(ctx);
}

GLboolean OSMESAApiBase::OSMesaGetColorBufferFn(OSMesaContext c,
                                                GLint* width,
                                                GLint* height,
                                                GLint* format,
                                                void** buffer) {
  return driver_->fn.OSMesaGetColorBufferFn(c, width, height, format, buffer);
}

OSMesaContext OSMESAApiBase::OSMesaGetCurrentContextFn(void) {
  return driver_->fn.OSMesaGetCurrentContextFn();
}

GLboolean OSMESAApiBase::OSMesaGetDepthBufferFn(OSMesaContext c,
                                                GLint* width,
                                                GLint* height,
                                                GLint* bytesPerValue,
                                                void** buffer) {
  return driver_->fn.OSMesaGetDepthBufferFn(c, width, height, bytesPerValue,
                                            buffer);
}

void OSMESAApiBase::OSMesaGetIntegervFn(GLint pname, GLint* value) {
  driver_->fn.OSMesaGetIntegervFn(pname, value);
}

OSMESAproc OSMESAApiBase::OSMesaGetProcAddressFn(const char* funcName) {
  return driver_->fn.OSMesaGetProcAddressFn(funcName);
}

GLboolean OSMESAApiBase::OSMesaMakeCurrentFn(OSMesaContext ctx,
                                             void* buffer,
                                             GLenum type,
                                             GLsizei width,
                                             GLsizei height) {
  return driver_->fn.OSMesaMakeCurrentFn(ctx, buffer, type, width, height);
}

void OSMESAApiBase::OSMesaPixelStoreFn(GLint pname, GLint value) {
  driver_->fn.OSMesaPixelStoreFn(pname, value);
}

void TraceOSMESAApi::OSMesaColorClampFn(GLboolean enable) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::OSMesaColorClamp")
  osmesa_api_->OSMesaColorClampFn(enable);
}

OSMesaContext TraceOSMESAApi::OSMesaCreateContextFn(GLenum format,
                                                    OSMesaContext sharelist) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::OSMesaCreateContext")
  return osmesa_api_->OSMesaCreateContextFn(format, sharelist);
}

OSMesaContext TraceOSMESAApi::OSMesaCreateContextExtFn(
    GLenum format,
    GLint depthBits,
    GLint stencilBits,
    GLint accumBits,
    OSMesaContext sharelist) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::OSMesaCreateContextExt")
  return osmesa_api_->OSMesaCreateContextExtFn(format, depthBits, stencilBits,
                                               accumBits, sharelist);
}

void TraceOSMESAApi::OSMesaDestroyContextFn(OSMesaContext ctx) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::OSMesaDestroyContext")
  osmesa_api_->OSMesaDestroyContextFn(ctx);
}

GLboolean TraceOSMESAApi::OSMesaGetColorBufferFn(OSMesaContext c,
                                                 GLint* width,
                                                 GLint* height,
                                                 GLint* format,
                                                 void** buffer) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::OSMesaGetColorBuffer")
  return osmesa_api_->OSMesaGetColorBufferFn(c, width, height, format, buffer);
}

OSMesaContext TraceOSMESAApi::OSMesaGetCurrentContextFn(void) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::OSMesaGetCurrentContext")
  return osmesa_api_->OSMesaGetCurrentContextFn();
}

GLboolean TraceOSMESAApi::OSMesaGetDepthBufferFn(OSMesaContext c,
                                                 GLint* width,
                                                 GLint* height,
                                                 GLint* bytesPerValue,
                                                 void** buffer) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::OSMesaGetDepthBuffer")
  return osmesa_api_->OSMesaGetDepthBufferFn(c, width, height, bytesPerValue,
                                             buffer);
}

void TraceOSMESAApi::OSMesaGetIntegervFn(GLint pname, GLint* value) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::OSMesaGetIntegerv")
  osmesa_api_->OSMesaGetIntegervFn(pname, value);
}

OSMESAproc TraceOSMESAApi::OSMesaGetProcAddressFn(const char* funcName) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::OSMesaGetProcAddress")
  return osmesa_api_->OSMesaGetProcAddressFn(funcName);
}

GLboolean TraceOSMESAApi::OSMesaMakeCurrentFn(OSMesaContext ctx,
                                              void* buffer,
                                              GLenum type,
                                              GLsizei width,
                                              GLsizei height) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::OSMesaMakeCurrent")
  return osmesa_api_->OSMesaMakeCurrentFn(ctx, buffer, type, width, height);
}

void TraceOSMESAApi::OSMesaPixelStoreFn(GLint pname, GLint value) {
  TRACE_EVENT_BINARY_EFFICIENT0("gpu", "TraceGLAPI::OSMesaPixelStore")
  osmesa_api_->OSMesaPixelStoreFn(pname, value);
}

void DebugOSMESAApi::OSMesaColorClampFn(GLboolean enable) {
  GL_SERVICE_LOG("OSMesaColorClamp"
                 << "(" << GLEnums::GetStringBool(enable) << ")");
  osmesa_api_->OSMesaColorClampFn(enable);
}

OSMesaContext DebugOSMESAApi::OSMesaCreateContextFn(GLenum format,
                                                    OSMesaContext sharelist) {
  GL_SERVICE_LOG("OSMesaCreateContext"
                 << "(" << GLEnums::GetStringEnum(format) << ", " << sharelist
                 << ")");
  OSMesaContext result = osmesa_api_->OSMesaCreateContextFn(format, sharelist);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

OSMesaContext DebugOSMESAApi::OSMesaCreateContextExtFn(
    GLenum format,
    GLint depthBits,
    GLint stencilBits,
    GLint accumBits,
    OSMesaContext sharelist) {
  GL_SERVICE_LOG("OSMesaCreateContextExt"
                 << "(" << GLEnums::GetStringEnum(format) << ", " << depthBits
                 << ", " << stencilBits << ", " << accumBits << ", "
                 << sharelist << ")");
  OSMesaContext result = osmesa_api_->OSMesaCreateContextExtFn(
      format, depthBits, stencilBits, accumBits, sharelist);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

void DebugOSMESAApi::OSMesaDestroyContextFn(OSMesaContext ctx) {
  GL_SERVICE_LOG("OSMesaDestroyContext"
                 << "(" << ctx << ")");
  osmesa_api_->OSMesaDestroyContextFn(ctx);
}

GLboolean DebugOSMESAApi::OSMesaGetColorBufferFn(OSMesaContext c,
                                                 GLint* width,
                                                 GLint* height,
                                                 GLint* format,
                                                 void** buffer) {
  GL_SERVICE_LOG("OSMesaGetColorBuffer"
                 << "(" << c << ", " << static_cast<const void*>(width) << ", "
                 << static_cast<const void*>(height) << ", "
                 << static_cast<const void*>(format) << ", " << buffer << ")");
  GLboolean result =
      osmesa_api_->OSMesaGetColorBufferFn(c, width, height, format, buffer);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

OSMesaContext DebugOSMESAApi::OSMesaGetCurrentContextFn(void) {
  GL_SERVICE_LOG("OSMesaGetCurrentContext"
                 << "("
                 << ")");
  OSMesaContext result = osmesa_api_->OSMesaGetCurrentContextFn();
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

GLboolean DebugOSMESAApi::OSMesaGetDepthBufferFn(OSMesaContext c,
                                                 GLint* width,
                                                 GLint* height,
                                                 GLint* bytesPerValue,
                                                 void** buffer) {
  GL_SERVICE_LOG("OSMesaGetDepthBuffer"
                 << "(" << c << ", " << static_cast<const void*>(width) << ", "
                 << static_cast<const void*>(height) << ", "
                 << static_cast<const void*>(bytesPerValue) << ", " << buffer
                 << ")");
  GLboolean result = osmesa_api_->OSMesaGetDepthBufferFn(c, width, height,
                                                         bytesPerValue, buffer);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

void DebugOSMESAApi::OSMesaGetIntegervFn(GLint pname, GLint* value) {
  GL_SERVICE_LOG("OSMesaGetIntegerv"
                 << "(" << pname << ", " << static_cast<const void*>(value)
                 << ")");
  osmesa_api_->OSMesaGetIntegervFn(pname, value);
}

OSMESAproc DebugOSMESAApi::OSMesaGetProcAddressFn(const char* funcName) {
  GL_SERVICE_LOG("OSMesaGetProcAddress"
                 << "(" << funcName << ")");
  OSMESAproc result = osmesa_api_->OSMesaGetProcAddressFn(funcName);

  GL_SERVICE_LOG("GL_RESULT: " << reinterpret_cast<void*>(result));

  return result;
}

GLboolean DebugOSMESAApi::OSMesaMakeCurrentFn(OSMesaContext ctx,
                                              void* buffer,
                                              GLenum type,
                                              GLsizei width,
                                              GLsizei height) {
  GL_SERVICE_LOG("OSMesaMakeCurrent"
                 << "(" << ctx << ", " << static_cast<const void*>(buffer)
                 << ", " << GLEnums::GetStringEnum(type) << ", " << width
                 << ", " << height << ")");
  GLboolean result =
      osmesa_api_->OSMesaMakeCurrentFn(ctx, buffer, type, width, height);
  GL_SERVICE_LOG("GL_RESULT: " << result);
  return result;
}

void DebugOSMESAApi::OSMesaPixelStoreFn(GLint pname, GLint value) {
  GL_SERVICE_LOG("OSMesaPixelStore"
                 << "(" << pname << ", " << value << ")");
  osmesa_api_->OSMesaPixelStoreFn(pname, value);
}

}  // namespace gl
