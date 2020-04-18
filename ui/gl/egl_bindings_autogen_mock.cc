// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file is auto-generated from
// ui/gl/generate_bindings.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

#include <string.h>

#include "ui/gl/egl_mock.h"

namespace {
// This is called mainly to prevent the compiler combining the code of mock
// functions with identical contents, so that their function pointers will be
// different.
void MakeFunctionUnique(const char* func_name) {
  VLOG(2) << "Calling mock " << func_name;
}
}  // namespace

namespace gl {

EGLBoolean GL_BINDING_CALL MockEGLInterface::Mock_eglBindAPI(EGLenum api) {
  MakeFunctionUnique("eglBindAPI");
  return interface_->BindAPI(api);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglBindTexImage(EGLDisplay dpy,
                                       EGLSurface surface,
                                       EGLint buffer) {
  MakeFunctionUnique("eglBindTexImage");
  return interface_->BindTexImage(dpy, surface, buffer);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglChooseConfig(EGLDisplay dpy,
                                       const EGLint* attrib_list,
                                       EGLConfig* configs,
                                       EGLint config_size,
                                       EGLint* num_config) {
  MakeFunctionUnique("eglChooseConfig");
  return interface_->ChooseConfig(dpy, attrib_list, configs, config_size,
                                  num_config);
}

EGLint GL_BINDING_CALL
MockEGLInterface::Mock_eglClientWaitSyncKHR(EGLDisplay dpy,
                                            EGLSyncKHR sync,
                                            EGLint flags,
                                            EGLTimeKHR timeout) {
  MakeFunctionUnique("eglClientWaitSyncKHR");
  return interface_->ClientWaitSyncKHR(dpy, sync, flags, timeout);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglCopyBuffers(EGLDisplay dpy,
                                      EGLSurface surface,
                                      EGLNativePixmapType target) {
  MakeFunctionUnique("eglCopyBuffers");
  return interface_->CopyBuffers(dpy, surface, target);
}

EGLContext GL_BINDING_CALL
MockEGLInterface::Mock_eglCreateContext(EGLDisplay dpy,
                                        EGLConfig config,
                                        EGLContext share_context,
                                        const EGLint* attrib_list) {
  MakeFunctionUnique("eglCreateContext");
  return interface_->CreateContext(dpy, config, share_context, attrib_list);
}

EGLImageKHR GL_BINDING_CALL
MockEGLInterface::Mock_eglCreateImageKHR(EGLDisplay dpy,
                                         EGLContext ctx,
                                         EGLenum target,
                                         EGLClientBuffer buffer,
                                         const EGLint* attrib_list) {
  MakeFunctionUnique("eglCreateImageKHR");
  return interface_->CreateImageKHR(dpy, ctx, target, buffer, attrib_list);
}

EGLSurface GL_BINDING_CALL
MockEGLInterface::Mock_eglCreatePbufferFromClientBuffer(
    EGLDisplay dpy,
    EGLenum buftype,
    void* buffer,
    EGLConfig config,
    const EGLint* attrib_list) {
  MakeFunctionUnique("eglCreatePbufferFromClientBuffer");
  return interface_->CreatePbufferFromClientBuffer(dpy, buftype, buffer, config,
                                                   attrib_list);
}

EGLSurface GL_BINDING_CALL
MockEGLInterface::Mock_eglCreatePbufferSurface(EGLDisplay dpy,
                                               EGLConfig config,
                                               const EGLint* attrib_list) {
  MakeFunctionUnique("eglCreatePbufferSurface");
  return interface_->CreatePbufferSurface(dpy, config, attrib_list);
}

EGLSurface GL_BINDING_CALL
MockEGLInterface::Mock_eglCreatePixmapSurface(EGLDisplay dpy,
                                              EGLConfig config,
                                              EGLNativePixmapType pixmap,
                                              const EGLint* attrib_list) {
  MakeFunctionUnique("eglCreatePixmapSurface");
  return interface_->CreatePixmapSurface(dpy, config, pixmap, attrib_list);
}

EGLStreamKHR GL_BINDING_CALL
MockEGLInterface::Mock_eglCreateStreamKHR(EGLDisplay dpy,
                                          const EGLint* attrib_list) {
  MakeFunctionUnique("eglCreateStreamKHR");
  return interface_->CreateStreamKHR(dpy, attrib_list);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglCreateStreamProducerD3DTextureANGLE(
    EGLDisplay dpy,
    EGLStreamKHR stream,
    EGLAttrib* attrib_list) {
  MakeFunctionUnique("eglCreateStreamProducerD3DTextureANGLE");
  return interface_->CreateStreamProducerD3DTextureANGLE(dpy, stream,
                                                         attrib_list);
}

EGLSyncKHR GL_BINDING_CALL
MockEGLInterface::Mock_eglCreateSyncKHR(EGLDisplay dpy,
                                        EGLenum type,
                                        const EGLint* attrib_list) {
  MakeFunctionUnique("eglCreateSyncKHR");
  return interface_->CreateSyncKHR(dpy, type, attrib_list);
}

EGLSurface GL_BINDING_CALL
MockEGLInterface::Mock_eglCreateWindowSurface(EGLDisplay dpy,
                                              EGLConfig config,
                                              EGLNativeWindowType win,
                                              const EGLint* attrib_list) {
  MakeFunctionUnique("eglCreateWindowSurface");
  return interface_->CreateWindowSurface(dpy, config, win, attrib_list);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
  MakeFunctionUnique("eglDestroyContext");
  return interface_->DestroyContext(dpy, ctx);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image) {
  MakeFunctionUnique("eglDestroyImageKHR");
  return interface_->DestroyImageKHR(dpy, image);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglDestroyStreamKHR(EGLDisplay dpy,
                                           EGLStreamKHR stream) {
  MakeFunctionUnique("eglDestroyStreamKHR");
  return interface_->DestroyStreamKHR(dpy, stream);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglDestroySurface(EGLDisplay dpy, EGLSurface surface) {
  MakeFunctionUnique("eglDestroySurface");
  return interface_->DestroySurface(dpy, surface);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync) {
  MakeFunctionUnique("eglDestroySyncKHR");
  return interface_->DestroySyncKHR(dpy, sync);
}

EGLint GL_BINDING_CALL
MockEGLInterface::Mock_eglDupNativeFenceFDANDROID(EGLDisplay dpy,
                                                  EGLSyncKHR sync) {
  MakeFunctionUnique("eglDupNativeFenceFDANDROID");
  return interface_->DupNativeFenceFDANDROID(dpy, sync);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglExportDMABUFImageMESA(EGLDisplay dpy,
                                                EGLImageKHR image,
                                                int* fds,
                                                EGLint* strides,
                                                EGLint* offsets) {
  MakeFunctionUnique("eglExportDMABUFImageMESA");
  return interface_->ExportDMABUFImageMESA(dpy, image, fds, strides, offsets);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglExportDMABUFImageQueryMESA(EGLDisplay dpy,
                                                     EGLImageKHR image,
                                                     int* fourcc,
                                                     int* num_planes,
                                                     EGLuint64KHR* modifiers) {
  MakeFunctionUnique("eglExportDMABUFImageQueryMESA");
  return interface_->ExportDMABUFImageQueryMESA(dpy, image, fourcc, num_planes,
                                                modifiers);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglGetCompositorTimingANDROID(EGLDisplay dpy,
                                                     EGLSurface surface,
                                                     EGLint numTimestamps,
                                                     EGLint* names,
                                                     EGLnsecsANDROID* values) {
  MakeFunctionUnique("eglGetCompositorTimingANDROID");
  return interface_->GetCompositorTimingANDROID(dpy, surface, numTimestamps,
                                                names, values);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglGetCompositorTimingSupportedANDROID(
    EGLDisplay dpy,
    EGLSurface surface,
    EGLint timestamp) {
  MakeFunctionUnique("eglGetCompositorTimingSupportedANDROID");
  return interface_->GetCompositorTimingSupportedANDROID(dpy, surface,
                                                         timestamp);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglGetConfigAttrib(EGLDisplay dpy,
                                          EGLConfig config,
                                          EGLint attribute,
                                          EGLint* value) {
  MakeFunctionUnique("eglGetConfigAttrib");
  return interface_->GetConfigAttrib(dpy, config, attribute, value);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglGetConfigs(EGLDisplay dpy,
                                     EGLConfig* configs,
                                     EGLint config_size,
                                     EGLint* num_config) {
  MakeFunctionUnique("eglGetConfigs");
  return interface_->GetConfigs(dpy, configs, config_size, num_config);
}

EGLContext GL_BINDING_CALL MockEGLInterface::Mock_eglGetCurrentContext(void) {
  MakeFunctionUnique("eglGetCurrentContext");
  return interface_->GetCurrentContext();
}

EGLDisplay GL_BINDING_CALL MockEGLInterface::Mock_eglGetCurrentDisplay(void) {
  MakeFunctionUnique("eglGetCurrentDisplay");
  return interface_->GetCurrentDisplay();
}

EGLSurface GL_BINDING_CALL
MockEGLInterface::Mock_eglGetCurrentSurface(EGLint readdraw) {
  MakeFunctionUnique("eglGetCurrentSurface");
  return interface_->GetCurrentSurface(readdraw);
}

EGLDisplay GL_BINDING_CALL
MockEGLInterface::Mock_eglGetDisplay(EGLNativeDisplayType display_id) {
  MakeFunctionUnique("eglGetDisplay");
  return interface_->GetDisplay(display_id);
}

EGLint GL_BINDING_CALL MockEGLInterface::Mock_eglGetError(void) {
  MakeFunctionUnique("eglGetError");
  return interface_->GetError();
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglGetFrameTimestampSupportedANDROID(EGLDisplay dpy,
                                                            EGLSurface surface,
                                                            EGLint timestamp) {
  MakeFunctionUnique("eglGetFrameTimestampSupportedANDROID");
  return interface_->GetFrameTimestampSupportedANDROID(dpy, surface, timestamp);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglGetFrameTimestampsANDROID(EGLDisplay dpy,
                                                    EGLSurface surface,
                                                    EGLuint64KHR frameId,
                                                    EGLint numTimestamps,
                                                    EGLint* timestamps,
                                                    EGLnsecsANDROID* values) {
  MakeFunctionUnique("eglGetFrameTimestampsANDROID");
  return interface_->GetFrameTimestampsANDROID(
      dpy, surface, frameId, numTimestamps, timestamps, values);
}

EGLClientBuffer GL_BINDING_CALL
MockEGLInterface::Mock_eglGetNativeClientBufferANDROID(
    const struct AHardwareBuffer* ahardwarebuffer) {
  MakeFunctionUnique("eglGetNativeClientBufferANDROID");
  return interface_->GetNativeClientBufferANDROID(ahardwarebuffer);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglGetNextFrameIdANDROID(EGLDisplay dpy,
                                                EGLSurface surface,
                                                EGLuint64KHR* frameId) {
  MakeFunctionUnique("eglGetNextFrameIdANDROID");
  return interface_->GetNextFrameIdANDROID(dpy, surface, frameId);
}

EGLDisplay GL_BINDING_CALL
MockEGLInterface::Mock_eglGetPlatformDisplayEXT(EGLenum platform,
                                                void* native_display,
                                                const EGLint* attrib_list) {
  MakeFunctionUnique("eglGetPlatformDisplayEXT");
  return interface_->GetPlatformDisplayEXT(platform, native_display,
                                           attrib_list);
}

__eglMustCastToProperFunctionPointerType GL_BINDING_CALL
MockEGLInterface::Mock_eglGetProcAddress(const char* procname) {
  MakeFunctionUnique("eglGetProcAddress");
  return interface_->GetProcAddress(procname);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglGetSyncAttribKHR(EGLDisplay dpy,
                                           EGLSyncKHR sync,
                                           EGLint attribute,
                                           EGLint* value) {
  MakeFunctionUnique("eglGetSyncAttribKHR");
  return interface_->GetSyncAttribKHR(dpy, sync, attribute, value);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglGetSyncValuesCHROMIUM(EGLDisplay dpy,
                                                EGLSurface surface,
                                                EGLuint64CHROMIUM* ust,
                                                EGLuint64CHROMIUM* msc,
                                                EGLuint64CHROMIUM* sbc) {
  MakeFunctionUnique("eglGetSyncValuesCHROMIUM");
  return interface_->GetSyncValuesCHROMIUM(dpy, surface, ust, msc, sbc);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglImageFlushExternalEXT(EGLDisplay dpy,
                                                EGLImageKHR image,
                                                const EGLAttrib* attrib_list) {
  MakeFunctionUnique("eglImageFlushExternalEXT");
  return interface_->ImageFlushExternalEXT(dpy, image, attrib_list);
}

EGLBoolean GL_BINDING_CALL MockEGLInterface::Mock_eglInitialize(EGLDisplay dpy,
                                                                EGLint* major,
                                                                EGLint* minor) {
  MakeFunctionUnique("eglInitialize");
  return interface_->Initialize(dpy, major, minor);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglMakeCurrent(EGLDisplay dpy,
                                      EGLSurface draw,
                                      EGLSurface read,
                                      EGLContext ctx) {
  MakeFunctionUnique("eglMakeCurrent");
  return interface_->MakeCurrent(dpy, draw, read, ctx);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglPostSubBufferNV(EGLDisplay dpy,
                                          EGLSurface surface,
                                          EGLint x,
                                          EGLint y,
                                          EGLint width,
                                          EGLint height) {
  MakeFunctionUnique("eglPostSubBufferNV");
  return interface_->PostSubBufferNV(dpy, surface, x, y, width, height);
}

EGLint GL_BINDING_CALL
MockEGLInterface::Mock_eglProgramCacheGetAttribANGLE(EGLDisplay dpy,
                                                     EGLenum attrib) {
  MakeFunctionUnique("eglProgramCacheGetAttribANGLE");
  return interface_->ProgramCacheGetAttribANGLE(dpy, attrib);
}

void GL_BINDING_CALL
MockEGLInterface::Mock_eglProgramCachePopulateANGLE(EGLDisplay dpy,
                                                    const void* key,
                                                    EGLint keysize,
                                                    const void* binary,
                                                    EGLint binarysize) {
  MakeFunctionUnique("eglProgramCachePopulateANGLE");
  interface_->ProgramCachePopulateANGLE(dpy, key, keysize, binary, binarysize);
}

void GL_BINDING_CALL
MockEGLInterface::Mock_eglProgramCacheQueryANGLE(EGLDisplay dpy,
                                                 EGLint index,
                                                 void* key,
                                                 EGLint* keysize,
                                                 void* binary,
                                                 EGLint* binarysize) {
  MakeFunctionUnique("eglProgramCacheQueryANGLE");
  interface_->ProgramCacheQueryANGLE(dpy, index, key, keysize, binary,
                                     binarysize);
}

EGLint GL_BINDING_CALL
MockEGLInterface::Mock_eglProgramCacheResizeANGLE(EGLDisplay dpy,
                                                  EGLint limit,
                                                  EGLenum mode) {
  MakeFunctionUnique("eglProgramCacheResizeANGLE");
  return interface_->ProgramCacheResizeANGLE(dpy, limit, mode);
}

EGLenum GL_BINDING_CALL MockEGLInterface::Mock_eglQueryAPI(void) {
  MakeFunctionUnique("eglQueryAPI");
  return interface_->QueryAPI();
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglQueryContext(EGLDisplay dpy,
                                       EGLContext ctx,
                                       EGLint attribute,
                                       EGLint* value) {
  MakeFunctionUnique("eglQueryContext");
  return interface_->QueryContext(dpy, ctx, attribute, value);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglQueryStreamKHR(EGLDisplay dpy,
                                         EGLStreamKHR stream,
                                         EGLenum attribute,
                                         EGLint* value) {
  MakeFunctionUnique("eglQueryStreamKHR");
  return interface_->QueryStreamKHR(dpy, stream, attribute, value);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglQueryStreamu64KHR(EGLDisplay dpy,
                                            EGLStreamKHR stream,
                                            EGLenum attribute,
                                            EGLuint64KHR* value) {
  MakeFunctionUnique("eglQueryStreamu64KHR");
  return interface_->QueryStreamu64KHR(dpy, stream, attribute, value);
}

const char* GL_BINDING_CALL
MockEGLInterface::Mock_eglQueryString(EGLDisplay dpy, EGLint name) {
  MakeFunctionUnique("eglQueryString");
  return interface_->QueryString(dpy, name);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglQuerySurface(EGLDisplay dpy,
                                       EGLSurface surface,
                                       EGLint attribute,
                                       EGLint* value) {
  MakeFunctionUnique("eglQuerySurface");
  return interface_->QuerySurface(dpy, surface, attribute, value);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglQuerySurfacePointerANGLE(EGLDisplay dpy,
                                                   EGLSurface surface,
                                                   EGLint attribute,
                                                   void** value) {
  MakeFunctionUnique("eglQuerySurfacePointerANGLE");
  return interface_->QuerySurfacePointerANGLE(dpy, surface, attribute, value);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglReleaseTexImage(EGLDisplay dpy,
                                          EGLSurface surface,
                                          EGLint buffer) {
  MakeFunctionUnique("eglReleaseTexImage");
  return interface_->ReleaseTexImage(dpy, surface, buffer);
}

EGLBoolean GL_BINDING_CALL MockEGLInterface::Mock_eglReleaseThread(void) {
  MakeFunctionUnique("eglReleaseThread");
  return interface_->ReleaseThread();
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglStreamAttribKHR(EGLDisplay dpy,
                                          EGLStreamKHR stream,
                                          EGLenum attribute,
                                          EGLint value) {
  MakeFunctionUnique("eglStreamAttribKHR");
  return interface_->StreamAttribKHR(dpy, stream, attribute, value);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglStreamConsumerAcquireKHR(EGLDisplay dpy,
                                                   EGLStreamKHR stream) {
  MakeFunctionUnique("eglStreamConsumerAcquireKHR");
  return interface_->StreamConsumerAcquireKHR(dpy, stream);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglStreamConsumerGLTextureExternalAttribsNV(
    EGLDisplay dpy,
    EGLStreamKHR stream,
    EGLAttrib* attrib_list) {
  MakeFunctionUnique("eglStreamConsumerGLTextureExternalAttribsNV");
  return interface_->StreamConsumerGLTextureExternalAttribsNV(dpy, stream,
                                                              attrib_list);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglStreamConsumerGLTextureExternalKHR(
    EGLDisplay dpy,
    EGLStreamKHR stream) {
  MakeFunctionUnique("eglStreamConsumerGLTextureExternalKHR");
  return interface_->StreamConsumerGLTextureExternalKHR(dpy, stream);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglStreamConsumerReleaseKHR(EGLDisplay dpy,
                                                   EGLStreamKHR stream) {
  MakeFunctionUnique("eglStreamConsumerReleaseKHR");
  return interface_->StreamConsumerReleaseKHR(dpy, stream);
}

EGLBoolean GL_BINDING_CALL MockEGLInterface::Mock_eglStreamPostD3DTextureANGLE(
    EGLDisplay dpy,
    EGLStreamKHR stream,
    void* texture,
    const EGLAttrib* attrib_list) {
  MakeFunctionUnique("eglStreamPostD3DTextureANGLE");
  return interface_->StreamPostD3DTextureANGLE(dpy, stream, texture,
                                               attrib_list);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglSurfaceAttrib(EGLDisplay dpy,
                                        EGLSurface surface,
                                        EGLint attribute,
                                        EGLint value) {
  MakeFunctionUnique("eglSurfaceAttrib");
  return interface_->SurfaceAttrib(dpy, surface, attribute, value);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
  MakeFunctionUnique("eglSwapBuffers");
  return interface_->SwapBuffers(dpy, surface);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglSwapBuffersWithDamageKHR(EGLDisplay dpy,
                                                   EGLSurface surface,
                                                   EGLint* rects,
                                                   EGLint n_rects) {
  MakeFunctionUnique("eglSwapBuffersWithDamageKHR");
  return interface_->SwapBuffersWithDamageKHR(dpy, surface, rects, n_rects);
}

EGLBoolean GL_BINDING_CALL
MockEGLInterface::Mock_eglSwapInterval(EGLDisplay dpy, EGLint interval) {
  MakeFunctionUnique("eglSwapInterval");
  return interface_->SwapInterval(dpy, interval);
}

EGLBoolean GL_BINDING_CALL MockEGLInterface::Mock_eglTerminate(EGLDisplay dpy) {
  MakeFunctionUnique("eglTerminate");
  return interface_->Terminate(dpy);
}

EGLBoolean GL_BINDING_CALL MockEGLInterface::Mock_eglWaitClient(void) {
  MakeFunctionUnique("eglWaitClient");
  return interface_->WaitClient();
}

EGLBoolean GL_BINDING_CALL MockEGLInterface::Mock_eglWaitGL(void) {
  MakeFunctionUnique("eglWaitGL");
  return interface_->WaitGL();
}

EGLBoolean GL_BINDING_CALL MockEGLInterface::Mock_eglWaitNative(EGLint engine) {
  MakeFunctionUnique("eglWaitNative");
  return interface_->WaitNative(engine);
}

EGLint GL_BINDING_CALL MockEGLInterface::Mock_eglWaitSyncKHR(EGLDisplay dpy,
                                                             EGLSyncKHR sync,
                                                             EGLint flags) {
  MakeFunctionUnique("eglWaitSyncKHR");
  return interface_->WaitSyncKHR(dpy, sync, flags);
}

static void MockInvalidFunction() {
  NOTREACHED();
}

GLFunctionPointerType GL_BINDING_CALL
MockEGLInterface::GetGLProcAddress(const char* name) {
  if (strcmp(name, "eglBindAPI") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglBindAPI);
  if (strcmp(name, "eglBindTexImage") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglBindTexImage);
  if (strcmp(name, "eglChooseConfig") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglChooseConfig);
  if (strcmp(name, "eglClientWaitSyncKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglClientWaitSyncKHR);
  if (strcmp(name, "eglCopyBuffers") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglCopyBuffers);
  if (strcmp(name, "eglCreateContext") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglCreateContext);
  if (strcmp(name, "eglCreateImageKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglCreateImageKHR);
  if (strcmp(name, "eglCreatePbufferFromClientBuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglCreatePbufferFromClientBuffer);
  if (strcmp(name, "eglCreatePbufferSurface") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglCreatePbufferSurface);
  if (strcmp(name, "eglCreatePixmapSurface") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglCreatePixmapSurface);
  if (strcmp(name, "eglCreateStreamKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglCreateStreamKHR);
  if (strcmp(name, "eglCreateStreamProducerD3DTextureANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglCreateStreamProducerD3DTextureANGLE);
  if (strcmp(name, "eglCreateSyncKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglCreateSyncKHR);
  if (strcmp(name, "eglCreateWindowSurface") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglCreateWindowSurface);
  if (strcmp(name, "eglDestroyContext") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglDestroyContext);
  if (strcmp(name, "eglDestroyImageKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglDestroyImageKHR);
  if (strcmp(name, "eglDestroyStreamKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglDestroyStreamKHR);
  if (strcmp(name, "eglDestroySurface") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglDestroySurface);
  if (strcmp(name, "eglDestroySyncKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglDestroySyncKHR);
  if (strcmp(name, "eglDupNativeFenceFDANDROID") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglDupNativeFenceFDANDROID);
  if (strcmp(name, "eglExportDMABUFImageMESA") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglExportDMABUFImageMESA);
  if (strcmp(name, "eglExportDMABUFImageQueryMESA") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglExportDMABUFImageQueryMESA);
  if (strcmp(name, "eglGetCompositorTimingANDROID") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglGetCompositorTimingANDROID);
  if (strcmp(name, "eglGetCompositorTimingSupportedANDROID") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglGetCompositorTimingSupportedANDROID);
  if (strcmp(name, "eglGetConfigAttrib") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglGetConfigAttrib);
  if (strcmp(name, "eglGetConfigs") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglGetConfigs);
  if (strcmp(name, "eglGetCurrentContext") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglGetCurrentContext);
  if (strcmp(name, "eglGetCurrentDisplay") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglGetCurrentDisplay);
  if (strcmp(name, "eglGetCurrentSurface") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglGetCurrentSurface);
  if (strcmp(name, "eglGetDisplay") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglGetDisplay);
  if (strcmp(name, "eglGetError") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglGetError);
  if (strcmp(name, "eglGetFrameTimestampSupportedANDROID") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglGetFrameTimestampSupportedANDROID);
  if (strcmp(name, "eglGetFrameTimestampsANDROID") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglGetFrameTimestampsANDROID);
  if (strcmp(name, "eglGetNativeClientBufferANDROID") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglGetNativeClientBufferANDROID);
  if (strcmp(name, "eglGetNextFrameIdANDROID") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglGetNextFrameIdANDROID);
  if (strcmp(name, "eglGetPlatformDisplayEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglGetPlatformDisplayEXT);
  if (strcmp(name, "eglGetProcAddress") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglGetProcAddress);
  if (strcmp(name, "eglGetSyncAttribKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglGetSyncAttribKHR);
  if (strcmp(name, "eglGetSyncValuesCHROMIUM") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglGetSyncValuesCHROMIUM);
  if (strcmp(name, "eglImageFlushExternalEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglImageFlushExternalEXT);
  if (strcmp(name, "eglInitialize") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglInitialize);
  if (strcmp(name, "eglMakeCurrent") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglMakeCurrent);
  if (strcmp(name, "eglPostSubBufferNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglPostSubBufferNV);
  if (strcmp(name, "eglProgramCacheGetAttribANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglProgramCacheGetAttribANGLE);
  if (strcmp(name, "eglProgramCachePopulateANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglProgramCachePopulateANGLE);
  if (strcmp(name, "eglProgramCacheQueryANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglProgramCacheQueryANGLE);
  if (strcmp(name, "eglProgramCacheResizeANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglProgramCacheResizeANGLE);
  if (strcmp(name, "eglQueryAPI") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglQueryAPI);
  if (strcmp(name, "eglQueryContext") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglQueryContext);
  if (strcmp(name, "eglQueryStreamKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglQueryStreamKHR);
  if (strcmp(name, "eglQueryStreamu64KHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglQueryStreamu64KHR);
  if (strcmp(name, "eglQueryString") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglQueryString);
  if (strcmp(name, "eglQuerySurface") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglQuerySurface);
  if (strcmp(name, "eglQuerySurfacePointerANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglQuerySurfacePointerANGLE);
  if (strcmp(name, "eglReleaseTexImage") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglReleaseTexImage);
  if (strcmp(name, "eglReleaseThread") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglReleaseThread);
  if (strcmp(name, "eglStreamAttribKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglStreamAttribKHR);
  if (strcmp(name, "eglStreamConsumerAcquireKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglStreamConsumerAcquireKHR);
  if (strcmp(name, "eglStreamConsumerGLTextureExternalAttribsNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglStreamConsumerGLTextureExternalAttribsNV);
  if (strcmp(name, "eglStreamConsumerGLTextureExternalKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglStreamConsumerGLTextureExternalKHR);
  if (strcmp(name, "eglStreamConsumerReleaseKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglStreamConsumerReleaseKHR);
  if (strcmp(name, "eglStreamPostD3DTextureANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglStreamPostD3DTextureANGLE);
  if (strcmp(name, "eglSurfaceAttrib") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglSurfaceAttrib);
  if (strcmp(name, "eglSwapBuffers") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglSwapBuffers);
  if (strcmp(name, "eglSwapBuffersWithDamageKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_eglSwapBuffersWithDamageKHR);
  if (strcmp(name, "eglSwapInterval") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglSwapInterval);
  if (strcmp(name, "eglTerminate") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglTerminate);
  if (strcmp(name, "eglWaitClient") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglWaitClient);
  if (strcmp(name, "eglWaitGL") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglWaitGL);
  if (strcmp(name, "eglWaitNative") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglWaitNative);
  if (strcmp(name, "eglWaitSyncKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_eglWaitSyncKHR);
  return reinterpret_cast<GLFunctionPointerType>(&MockInvalidFunction);
}

}  // namespace gl
