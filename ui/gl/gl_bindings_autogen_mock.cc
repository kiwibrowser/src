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

#include "ui/gl/gl_mock.h"

namespace {
// This is called mainly to prevent the compiler combining the code of mock
// functions with identical contents, so that their function pointers will be
// different.
void MakeFunctionUnique(const char* func_name) {
  VLOG(2) << "Calling mock " << func_name;
}
}  // namespace

namespace gl {

void GL_BINDING_CALL MockGLInterface::Mock_glActiveTexture(GLenum texture) {
  MakeFunctionUnique("glActiveTexture");
  interface_->ActiveTexture(texture);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glApplyFramebufferAttachmentCMAAINTEL(void) {
  MakeFunctionUnique("glApplyFramebufferAttachmentCMAAINTEL");
  interface_->ApplyFramebufferAttachmentCMAAINTEL();
}

void GL_BINDING_CALL MockGLInterface::Mock_glAttachShader(GLuint program,
                                                          GLuint shader) {
  MakeFunctionUnique("glAttachShader");
  interface_->AttachShader(program, shader);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBeginQuery(GLenum target,
                                                        GLuint id) {
  MakeFunctionUnique("glBeginQuery");
  interface_->BeginQuery(target, id);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBeginQueryARB(GLenum target,
                                                           GLuint id) {
  MakeFunctionUnique("glBeginQueryARB");
  interface_->BeginQuery(target, id);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBeginQueryEXT(GLenum target,
                                                           GLuint id) {
  MakeFunctionUnique("glBeginQueryEXT");
  interface_->BeginQuery(target, id);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBeginTransformFeedback(GLenum primitiveMode) {
  MakeFunctionUnique("glBeginTransformFeedback");
  interface_->BeginTransformFeedback(primitiveMode);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBeginTransformFeedbackEXT(GLenum primitiveMode) {
  MakeFunctionUnique("glBeginTransformFeedbackEXT");
  interface_->BeginTransformFeedback(primitiveMode);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindAttribLocation(GLuint program,
                                           GLuint index,
                                           const char* name) {
  MakeFunctionUnique("glBindAttribLocation");
  interface_->BindAttribLocation(program, index, name);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBindBuffer(GLenum target,
                                                        GLuint buffer) {
  MakeFunctionUnique("glBindBuffer");
  interface_->BindBuffer(target, buffer);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBindBufferBase(GLenum target,
                                                            GLuint index,
                                                            GLuint buffer) {
  MakeFunctionUnique("glBindBufferBase");
  interface_->BindBufferBase(target, index, buffer);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBindBufferBaseEXT(GLenum target,
                                                               GLuint index,
                                                               GLuint buffer) {
  MakeFunctionUnique("glBindBufferBaseEXT");
  interface_->BindBufferBase(target, index, buffer);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBindBufferRange(GLenum target,
                                                             GLuint index,
                                                             GLuint buffer,
                                                             GLintptr offset,
                                                             GLsizeiptr size) {
  MakeFunctionUnique("glBindBufferRange");
  interface_->BindBufferRange(target, index, buffer, offset, size);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindBufferRangeEXT(GLenum target,
                                           GLuint index,
                                           GLuint buffer,
                                           GLintptr offset,
                                           GLsizeiptr size) {
  MakeFunctionUnique("glBindBufferRangeEXT");
  interface_->BindBufferRange(target, index, buffer, offset, size);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindFragDataLocation(GLuint program,
                                             GLuint colorNumber,
                                             const char* name) {
  MakeFunctionUnique("glBindFragDataLocation");
  interface_->BindFragDataLocation(program, colorNumber, name);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindFragDataLocationEXT(GLuint program,
                                                GLuint colorNumber,
                                                const char* name) {
  MakeFunctionUnique("glBindFragDataLocationEXT");
  interface_->BindFragDataLocation(program, colorNumber, name);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindFragDataLocationIndexed(GLuint program,
                                                    GLuint colorNumber,
                                                    GLuint index,
                                                    const char* name) {
  MakeFunctionUnique("glBindFragDataLocationIndexed");
  interface_->BindFragDataLocationIndexed(program, colorNumber, index, name);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindFragDataLocationIndexedEXT(GLuint program,
                                                       GLuint colorNumber,
                                                       GLuint index,
                                                       const char* name) {
  MakeFunctionUnique("glBindFragDataLocationIndexedEXT");
  interface_->BindFragDataLocationIndexed(program, colorNumber, index, name);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindFramebuffer(GLenum target, GLuint framebuffer) {
  MakeFunctionUnique("glBindFramebuffer");
  interface_->BindFramebufferEXT(target, framebuffer);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindFramebufferEXT(GLenum target, GLuint framebuffer) {
  MakeFunctionUnique("glBindFramebufferEXT");
  interface_->BindFramebufferEXT(target, framebuffer);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBindImageTexture(GLuint index,
                                                              GLuint texture,
                                                              GLint level,
                                                              GLboolean layered,
                                                              GLint layer,
                                                              GLenum access,
                                                              GLint format) {
  MakeFunctionUnique("glBindImageTexture");
  interface_->BindImageTextureEXT(index, texture, level, layered, layer, access,
                                  format);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindImageTextureEXT(GLuint index,
                                            GLuint texture,
                                            GLint level,
                                            GLboolean layered,
                                            GLint layer,
                                            GLenum access,
                                            GLint format) {
  MakeFunctionUnique("glBindImageTextureEXT");
  interface_->BindImageTextureEXT(index, texture, level, layered, layer, access,
                                  format);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
  MakeFunctionUnique("glBindRenderbuffer");
  interface_->BindRenderbufferEXT(target, renderbuffer);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindRenderbufferEXT(GLenum target,
                                            GLuint renderbuffer) {
  MakeFunctionUnique("glBindRenderbufferEXT");
  interface_->BindRenderbufferEXT(target, renderbuffer);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBindSampler(GLuint unit,
                                                         GLuint sampler) {
  MakeFunctionUnique("glBindSampler");
  interface_->BindSampler(unit, sampler);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBindTexture(GLenum target,
                                                         GLuint texture) {
  MakeFunctionUnique("glBindTexture");
  interface_->BindTexture(target, texture);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindTransformFeedback(GLenum target, GLuint id) {
  MakeFunctionUnique("glBindTransformFeedback");
  interface_->BindTransformFeedback(target, id);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindUniformLocationCHROMIUM(GLuint program,
                                                    GLint location,
                                                    const char* name) {
  MakeFunctionUnique("glBindUniformLocationCHROMIUM");
  interface_->BindUniformLocationCHROMIUM(program, location, name);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBindVertexArray(GLuint array) {
  MakeFunctionUnique("glBindVertexArray");
  interface_->BindVertexArrayOES(array);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBindVertexArrayAPPLE(GLuint array) {
  MakeFunctionUnique("glBindVertexArrayAPPLE");
  interface_->BindVertexArrayOES(array);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBindVertexArrayOES(GLuint array) {
  MakeFunctionUnique("glBindVertexArrayOES");
  interface_->BindVertexArrayOES(array);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBlendBarrierKHR(void) {
  MakeFunctionUnique("glBlendBarrierKHR");
  interface_->BlendBarrierKHR();
}

void GL_BINDING_CALL MockGLInterface::Mock_glBlendBarrierNV(void) {
  MakeFunctionUnique("glBlendBarrierNV");
  interface_->BlendBarrierKHR();
}

void GL_BINDING_CALL MockGLInterface::Mock_glBlendColor(GLclampf red,
                                                        GLclampf green,
                                                        GLclampf blue,
                                                        GLclampf alpha) {
  MakeFunctionUnique("glBlendColor");
  interface_->BlendColor(red, green, blue, alpha);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBlendEquation(GLenum mode) {
  MakeFunctionUnique("glBlendEquation");
  interface_->BlendEquation(mode);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBlendEquationSeparate(GLenum modeRGB,
                                              GLenum modeAlpha) {
  MakeFunctionUnique("glBlendEquationSeparate");
  interface_->BlendEquationSeparate(modeRGB, modeAlpha);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBlendFunc(GLenum sfactor,
                                                       GLenum dfactor) {
  MakeFunctionUnique("glBlendFunc");
  interface_->BlendFunc(sfactor, dfactor);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBlendFuncSeparate(GLenum srcRGB,
                                          GLenum dstRGB,
                                          GLenum srcAlpha,
                                          GLenum dstAlpha) {
  MakeFunctionUnique("glBlendFuncSeparate");
  interface_->BlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBlitFramebuffer(GLint srcX0,
                                                             GLint srcY0,
                                                             GLint srcX1,
                                                             GLint srcY1,
                                                             GLint dstX0,
                                                             GLint dstY0,
                                                             GLint dstX1,
                                                             GLint dstY1,
                                                             GLbitfield mask,
                                                             GLenum filter) {
  MakeFunctionUnique("glBlitFramebuffer");
  interface_->BlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1,
                              dstY1, mask, filter);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glBlitFramebufferANGLE(GLint srcX0,
                                             GLint srcY0,
                                             GLint srcX1,
                                             GLint srcY1,
                                             GLint dstX0,
                                             GLint dstY0,
                                             GLint dstX1,
                                             GLint dstY1,
                                             GLbitfield mask,
                                             GLenum filter) {
  MakeFunctionUnique("glBlitFramebufferANGLE");
  interface_->BlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1,
                              dstY1, mask, filter);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBlitFramebufferEXT(GLint srcX0,
                                                                GLint srcY0,
                                                                GLint srcX1,
                                                                GLint srcY1,
                                                                GLint dstX0,
                                                                GLint dstY0,
                                                                GLint dstX1,
                                                                GLint dstY1,
                                                                GLbitfield mask,
                                                                GLenum filter) {
  MakeFunctionUnique("glBlitFramebufferEXT");
  interface_->BlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1,
                              dstY1, mask, filter);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBufferData(GLenum target,
                                                        GLsizeiptr size,
                                                        const void* data,
                                                        GLenum usage) {
  MakeFunctionUnique("glBufferData");
  interface_->BufferData(target, size, data, usage);
}

void GL_BINDING_CALL MockGLInterface::Mock_glBufferSubData(GLenum target,
                                                           GLintptr offset,
                                                           GLsizeiptr size,
                                                           const void* data) {
  MakeFunctionUnique("glBufferSubData");
  interface_->BufferSubData(target, offset, size, data);
}

GLenum GL_BINDING_CALL
MockGLInterface::Mock_glCheckFramebufferStatus(GLenum target) {
  MakeFunctionUnique("glCheckFramebufferStatus");
  return interface_->CheckFramebufferStatusEXT(target);
}

GLenum GL_BINDING_CALL
MockGLInterface::Mock_glCheckFramebufferStatusEXT(GLenum target) {
  MakeFunctionUnique("glCheckFramebufferStatusEXT");
  return interface_->CheckFramebufferStatusEXT(target);
}

void GL_BINDING_CALL MockGLInterface::Mock_glClear(GLbitfield mask) {
  MakeFunctionUnique("glClear");
  interface_->Clear(mask);
}

void GL_BINDING_CALL MockGLInterface::Mock_glClearBufferfi(GLenum buffer,
                                                           GLint drawbuffer,
                                                           const GLfloat depth,
                                                           GLint stencil) {
  MakeFunctionUnique("glClearBufferfi");
  interface_->ClearBufferfi(buffer, drawbuffer, depth, stencil);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glClearBufferfv(GLenum buffer,
                                      GLint drawbuffer,
                                      const GLfloat* value) {
  MakeFunctionUnique("glClearBufferfv");
  interface_->ClearBufferfv(buffer, drawbuffer, value);
}

void GL_BINDING_CALL MockGLInterface::Mock_glClearBufferiv(GLenum buffer,
                                                           GLint drawbuffer,
                                                           const GLint* value) {
  MakeFunctionUnique("glClearBufferiv");
  interface_->ClearBufferiv(buffer, drawbuffer, value);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glClearBufferuiv(GLenum buffer,
                                       GLint drawbuffer,
                                       const GLuint* value) {
  MakeFunctionUnique("glClearBufferuiv");
  interface_->ClearBufferuiv(buffer, drawbuffer, value);
}

void GL_BINDING_CALL MockGLInterface::Mock_glClearColor(GLclampf red,
                                                        GLclampf green,
                                                        GLclampf blue,
                                                        GLclampf alpha) {
  MakeFunctionUnique("glClearColor");
  interface_->ClearColor(red, green, blue, alpha);
}

void GL_BINDING_CALL MockGLInterface::Mock_glClearDepth(GLclampd depth) {
  MakeFunctionUnique("glClearDepth");
  interface_->ClearDepth(depth);
}

void GL_BINDING_CALL MockGLInterface::Mock_glClearDepthf(GLclampf depth) {
  MakeFunctionUnique("glClearDepthf");
  interface_->ClearDepthf(depth);
}

void GL_BINDING_CALL MockGLInterface::Mock_glClearStencil(GLint s) {
  MakeFunctionUnique("glClearStencil");
  interface_->ClearStencil(s);
}

GLenum GL_BINDING_CALL
MockGLInterface::Mock_glClientWaitSync(GLsync sync,
                                       GLbitfield flags,
                                       GLuint64 timeout) {
  MakeFunctionUnique("glClientWaitSync");
  return interface_->ClientWaitSync(sync, flags, timeout);
}

void GL_BINDING_CALL MockGLInterface::Mock_glColorMask(GLboolean red,
                                                       GLboolean green,
                                                       GLboolean blue,
                                                       GLboolean alpha) {
  MakeFunctionUnique("glColorMask");
  interface_->ColorMask(red, green, blue, alpha);
}

void GL_BINDING_CALL MockGLInterface::Mock_glCompileShader(GLuint shader) {
  MakeFunctionUnique("glCompileShader");
  interface_->CompileShader(shader);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCompressedCopyTextureCHROMIUM(GLuint sourceId,
                                                      GLuint destId) {
  MakeFunctionUnique("glCompressedCopyTextureCHROMIUM");
  interface_->CompressedCopyTextureCHROMIUM(sourceId, destId);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCompressedTexImage2D(GLenum target,
                                             GLint level,
                                             GLenum internalformat,
                                             GLsizei width,
                                             GLsizei height,
                                             GLint border,
                                             GLsizei imageSize,
                                             const void* data) {
  MakeFunctionUnique("glCompressedTexImage2D");
  interface_->CompressedTexImage2D(target, level, internalformat, width, height,
                                   border, imageSize, data);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCompressedTexImage2DRobustANGLE(GLenum target,
                                                        GLint level,
                                                        GLenum internalformat,
                                                        GLsizei width,
                                                        GLsizei height,
                                                        GLint border,
                                                        GLsizei imageSize,
                                                        GLsizei dataSize,
                                                        const void* data) {
  MakeFunctionUnique("glCompressedTexImage2DRobustANGLE");
  interface_->CompressedTexImage2DRobustANGLE(target, level, internalformat,
                                              width, height, border, imageSize,
                                              dataSize, data);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCompressedTexImage3D(GLenum target,
                                             GLint level,
                                             GLenum internalformat,
                                             GLsizei width,
                                             GLsizei height,
                                             GLsizei depth,
                                             GLint border,
                                             GLsizei imageSize,
                                             const void* data) {
  MakeFunctionUnique("glCompressedTexImage3D");
  interface_->CompressedTexImage3D(target, level, internalformat, width, height,
                                   depth, border, imageSize, data);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCompressedTexImage3DRobustANGLE(GLenum target,
                                                        GLint level,
                                                        GLenum internalformat,
                                                        GLsizei width,
                                                        GLsizei height,
                                                        GLsizei depth,
                                                        GLint border,
                                                        GLsizei imageSize,
                                                        GLsizei dataSize,
                                                        const void* data) {
  MakeFunctionUnique("glCompressedTexImage3DRobustANGLE");
  interface_->CompressedTexImage3DRobustANGLE(target, level, internalformat,
                                              width, height, depth, border,
                                              imageSize, dataSize, data);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCompressedTexSubImage2D(GLenum target,
                                                GLint level,
                                                GLint xoffset,
                                                GLint yoffset,
                                                GLsizei width,
                                                GLsizei height,
                                                GLenum format,
                                                GLsizei imageSize,
                                                const void* data) {
  MakeFunctionUnique("glCompressedTexSubImage2D");
  interface_->CompressedTexSubImage2D(target, level, xoffset, yoffset, width,
                                      height, format, imageSize, data);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCompressedTexSubImage2DRobustANGLE(GLenum target,
                                                           GLint level,
                                                           GLint xoffset,
                                                           GLint yoffset,
                                                           GLsizei width,
                                                           GLsizei height,
                                                           GLenum format,
                                                           GLsizei imageSize,
                                                           GLsizei dataSize,
                                                           const void* data) {
  MakeFunctionUnique("glCompressedTexSubImage2DRobustANGLE");
  interface_->CompressedTexSubImage2DRobustANGLE(target, level, xoffset,
                                                 yoffset, width, height, format,
                                                 imageSize, dataSize, data);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCompressedTexSubImage3D(GLenum target,
                                                GLint level,
                                                GLint xoffset,
                                                GLint yoffset,
                                                GLint zoffset,
                                                GLsizei width,
                                                GLsizei height,
                                                GLsizei depth,
                                                GLenum format,
                                                GLsizei imageSize,
                                                const void* data) {
  MakeFunctionUnique("glCompressedTexSubImage3D");
  interface_->CompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset,
                                      width, height, depth, format, imageSize,
                                      data);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCompressedTexSubImage3DRobustANGLE(GLenum target,
                                                           GLint level,
                                                           GLint xoffset,
                                                           GLint yoffset,
                                                           GLint zoffset,
                                                           GLsizei width,
                                                           GLsizei height,
                                                           GLsizei depth,
                                                           GLenum format,
                                                           GLsizei imageSize,
                                                           GLsizei dataSize,
                                                           const void* data) {
  MakeFunctionUnique("glCompressedTexSubImage3DRobustANGLE");
  interface_->CompressedTexSubImage3DRobustANGLE(
      target, level, xoffset, yoffset, zoffset, width, height, depth, format,
      imageSize, dataSize, data);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCopyBufferSubData(GLenum readTarget,
                                          GLenum writeTarget,
                                          GLintptr readOffset,
                                          GLintptr writeOffset,
                                          GLsizeiptr size) {
  MakeFunctionUnique("glCopyBufferSubData");
  interface_->CopyBufferSubData(readTarget, writeTarget, readOffset,
                                writeOffset, size);
}

void GL_BINDING_CALL MockGLInterface::Mock_glCopySubTextureCHROMIUM(
    GLuint sourceId,
    GLint sourceLevel,
    GLenum destTarget,
    GLuint destId,
    GLint destLevel,
    GLint xoffset,
    GLint yoffset,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLboolean unpackFlipY,
    GLboolean unpackPremultiplyAlpha,
    GLboolean unpackUnmultiplyAlpha) {
  MakeFunctionUnique("glCopySubTextureCHROMIUM");
  interface_->CopySubTextureCHROMIUM(
      sourceId, sourceLevel, destTarget, destId, destLevel, xoffset, yoffset, x,
      y, width, height, unpackFlipY, unpackPremultiplyAlpha,
      unpackUnmultiplyAlpha);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCopyTexImage2D(GLenum target,
                                       GLint level,
                                       GLenum internalformat,
                                       GLint x,
                                       GLint y,
                                       GLsizei width,
                                       GLsizei height,
                                       GLint border) {
  MakeFunctionUnique("glCopyTexImage2D");
  interface_->CopyTexImage2D(target, level, internalformat, x, y, width, height,
                             border);
}

void GL_BINDING_CALL MockGLInterface::Mock_glCopyTexSubImage2D(GLenum target,
                                                               GLint level,
                                                               GLint xoffset,
                                                               GLint yoffset,
                                                               GLint x,
                                                               GLint y,
                                                               GLsizei width,
                                                               GLsizei height) {
  MakeFunctionUnique("glCopyTexSubImage2D");
  interface_->CopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width,
                                height);
}

void GL_BINDING_CALL MockGLInterface::Mock_glCopyTexSubImage3D(GLenum target,
                                                               GLint level,
                                                               GLint xoffset,
                                                               GLint yoffset,
                                                               GLint zoffset,
                                                               GLint x,
                                                               GLint y,
                                                               GLsizei width,
                                                               GLsizei height) {
  MakeFunctionUnique("glCopyTexSubImage3D");
  interface_->CopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y,
                                width, height);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCopyTextureCHROMIUM(GLuint sourceId,
                                            GLint sourceLevel,
                                            GLenum destTarget,
                                            GLuint destId,
                                            GLint destLevel,
                                            GLint internalFormat,
                                            GLenum destType,
                                            GLboolean unpackFlipY,
                                            GLboolean unpackPremultiplyAlpha,
                                            GLboolean unpackUnmultiplyAlpha) {
  MakeFunctionUnique("glCopyTextureCHROMIUM");
  interface_->CopyTextureCHROMIUM(
      sourceId, sourceLevel, destTarget, destId, destLevel, internalFormat,
      destType, unpackFlipY, unpackPremultiplyAlpha, unpackUnmultiplyAlpha);
}

void GL_BINDING_CALL MockGLInterface::Mock_glCoverFillPathInstancedNV(
    GLsizei numPaths,
    GLenum pathNameType,
    const void* paths,
    GLuint pathBase,
    GLenum coverMode,
    GLenum transformType,
    const GLfloat* transformValues) {
  MakeFunctionUnique("glCoverFillPathInstancedNV");
  interface_->CoverFillPathInstancedNV(numPaths, pathNameType, paths, pathBase,
                                       coverMode, transformType,
                                       transformValues);
}

void GL_BINDING_CALL MockGLInterface::Mock_glCoverFillPathNV(GLuint path,
                                                             GLenum coverMode) {
  MakeFunctionUnique("glCoverFillPathNV");
  interface_->CoverFillPathNV(path, coverMode);
}

void GL_BINDING_CALL MockGLInterface::Mock_glCoverStrokePathInstancedNV(
    GLsizei numPaths,
    GLenum pathNameType,
    const void* paths,
    GLuint pathBase,
    GLenum coverMode,
    GLenum transformType,
    const GLfloat* transformValues) {
  MakeFunctionUnique("glCoverStrokePathInstancedNV");
  interface_->CoverStrokePathInstancedNV(numPaths, pathNameType, paths,
                                         pathBase, coverMode, transformType,
                                         transformValues);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCoverStrokePathNV(GLuint name, GLenum coverMode) {
  MakeFunctionUnique("glCoverStrokePathNV");
  interface_->CoverStrokePathNV(name, coverMode);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glCoverageModulationNV(GLenum components) {
  MakeFunctionUnique("glCoverageModulationNV");
  interface_->CoverageModulationNV(components);
}

GLuint GL_BINDING_CALL MockGLInterface::Mock_glCreateProgram(void) {
  MakeFunctionUnique("glCreateProgram");
  return interface_->CreateProgram();
}

GLuint GL_BINDING_CALL MockGLInterface::Mock_glCreateShader(GLenum type) {
  MakeFunctionUnique("glCreateShader");
  return interface_->CreateShader(type);
}

void GL_BINDING_CALL MockGLInterface::Mock_glCullFace(GLenum mode) {
  MakeFunctionUnique("glCullFace");
  interface_->CullFace(mode);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDebugMessageCallback(GLDEBUGPROC callback,
                                             const void* userParam) {
  MakeFunctionUnique("glDebugMessageCallback");
  interface_->DebugMessageCallback(callback, userParam);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDebugMessageCallbackKHR(GLDEBUGPROC callback,
                                                const void* userParam) {
  MakeFunctionUnique("glDebugMessageCallbackKHR");
  interface_->DebugMessageCallback(callback, userParam);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDebugMessageControl(GLenum source,
                                            GLenum type,
                                            GLenum severity,
                                            GLsizei count,
                                            const GLuint* ids,
                                            GLboolean enabled) {
  MakeFunctionUnique("glDebugMessageControl");
  interface_->DebugMessageControl(source, type, severity, count, ids, enabled);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDebugMessageControlKHR(GLenum source,
                                               GLenum type,
                                               GLenum severity,
                                               GLsizei count,
                                               const GLuint* ids,
                                               GLboolean enabled) {
  MakeFunctionUnique("glDebugMessageControlKHR");
  interface_->DebugMessageControl(source, type, severity, count, ids, enabled);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDebugMessageInsert(GLenum source,
                                           GLenum type,
                                           GLuint id,
                                           GLenum severity,
                                           GLsizei length,
                                           const char* buf) {
  MakeFunctionUnique("glDebugMessageInsert");
  interface_->DebugMessageInsert(source, type, id, severity, length, buf);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDebugMessageInsertKHR(GLenum source,
                                              GLenum type,
                                              GLuint id,
                                              GLenum severity,
                                              GLsizei length,
                                              const char* buf) {
  MakeFunctionUnique("glDebugMessageInsertKHR");
  interface_->DebugMessageInsert(source, type, id, severity, length, buf);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteBuffers(GLsizei n, const GLuint* buffers) {
  MakeFunctionUnique("glDeleteBuffers");
  interface_->DeleteBuffersARB(n, buffers);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteFencesAPPLE(GLsizei n, const GLuint* fences) {
  MakeFunctionUnique("glDeleteFencesAPPLE");
  interface_->DeleteFencesAPPLE(n, fences);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteFencesNV(GLsizei n, const GLuint* fences) {
  MakeFunctionUnique("glDeleteFencesNV");
  interface_->DeleteFencesNV(n, fences);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteFramebuffers(GLsizei n,
                                           const GLuint* framebuffers) {
  MakeFunctionUnique("glDeleteFramebuffers");
  interface_->DeleteFramebuffersEXT(n, framebuffers);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteFramebuffersEXT(GLsizei n,
                                              const GLuint* framebuffers) {
  MakeFunctionUnique("glDeleteFramebuffersEXT");
  interface_->DeleteFramebuffersEXT(n, framebuffers);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDeletePathsNV(GLuint path,
                                                           GLsizei range) {
  MakeFunctionUnique("glDeletePathsNV");
  interface_->DeletePathsNV(path, range);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDeleteProgram(GLuint program) {
  MakeFunctionUnique("glDeleteProgram");
  interface_->DeleteProgram(program);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDeleteQueries(GLsizei n,
                                                           const GLuint* ids) {
  MakeFunctionUnique("glDeleteQueries");
  interface_->DeleteQueries(n, ids);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteQueriesARB(GLsizei n, const GLuint* ids) {
  MakeFunctionUnique("glDeleteQueriesARB");
  interface_->DeleteQueries(n, ids);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteQueriesEXT(GLsizei n, const GLuint* ids) {
  MakeFunctionUnique("glDeleteQueriesEXT");
  interface_->DeleteQueries(n, ids);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteRenderbuffers(GLsizei n,
                                            const GLuint* renderbuffers) {
  MakeFunctionUnique("glDeleteRenderbuffers");
  interface_->DeleteRenderbuffersEXT(n, renderbuffers);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteRenderbuffersEXT(GLsizei n,
                                               const GLuint* renderbuffers) {
  MakeFunctionUnique("glDeleteRenderbuffersEXT");
  interface_->DeleteRenderbuffersEXT(n, renderbuffers);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteSamplers(GLsizei n, const GLuint* samplers) {
  MakeFunctionUnique("glDeleteSamplers");
  interface_->DeleteSamplers(n, samplers);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDeleteShader(GLuint shader) {
  MakeFunctionUnique("glDeleteShader");
  interface_->DeleteShader(shader);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDeleteSync(GLsync sync) {
  MakeFunctionUnique("glDeleteSync");
  interface_->DeleteSync(sync);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteTextures(GLsizei n, const GLuint* textures) {
  MakeFunctionUnique("glDeleteTextures");
  interface_->DeleteTextures(n, textures);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteTransformFeedbacks(GLsizei n, const GLuint* ids) {
  MakeFunctionUnique("glDeleteTransformFeedbacks");
  interface_->DeleteTransformFeedbacks(n, ids);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {
  MakeFunctionUnique("glDeleteVertexArrays");
  interface_->DeleteVertexArraysOES(n, arrays);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteVertexArraysAPPLE(GLsizei n,
                                                const GLuint* arrays) {
  MakeFunctionUnique("glDeleteVertexArraysAPPLE");
  interface_->DeleteVertexArraysOES(n, arrays);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDeleteVertexArraysOES(GLsizei n, const GLuint* arrays) {
  MakeFunctionUnique("glDeleteVertexArraysOES");
  interface_->DeleteVertexArraysOES(n, arrays);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDepthFunc(GLenum func) {
  MakeFunctionUnique("glDepthFunc");
  interface_->DepthFunc(func);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDepthMask(GLboolean flag) {
  MakeFunctionUnique("glDepthMask");
  interface_->DepthMask(flag);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDepthRange(GLclampd zNear,
                                                        GLclampd zFar) {
  MakeFunctionUnique("glDepthRange");
  interface_->DepthRange(zNear, zFar);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDepthRangef(GLclampf zNear,
                                                         GLclampf zFar) {
  MakeFunctionUnique("glDepthRangef");
  interface_->DepthRangef(zNear, zFar);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDetachShader(GLuint program,
                                                          GLuint shader) {
  MakeFunctionUnique("glDetachShader");
  interface_->DetachShader(program, shader);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDisable(GLenum cap) {
  MakeFunctionUnique("glDisable");
  interface_->Disable(cap);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDisableVertexAttribArray(GLuint index) {
  MakeFunctionUnique("glDisableVertexAttribArray");
  interface_->DisableVertexAttribArray(index);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDiscardFramebufferEXT(GLenum target,
                                              GLsizei numAttachments,
                                              const GLenum* attachments) {
  MakeFunctionUnique("glDiscardFramebufferEXT");
  interface_->DiscardFramebufferEXT(target, numAttachments, attachments);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDrawArrays(GLenum mode,
                                                        GLint first,
                                                        GLsizei count) {
  MakeFunctionUnique("glDrawArrays");
  interface_->DrawArrays(mode, first, count);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDrawArraysInstanced(GLenum mode,
                                            GLint first,
                                            GLsizei count,
                                            GLsizei primcount) {
  MakeFunctionUnique("glDrawArraysInstanced");
  interface_->DrawArraysInstancedANGLE(mode, first, count, primcount);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDrawArraysInstancedANGLE(GLenum mode,
                                                 GLint first,
                                                 GLsizei count,
                                                 GLsizei primcount) {
  MakeFunctionUnique("glDrawArraysInstancedANGLE");
  interface_->DrawArraysInstancedANGLE(mode, first, count, primcount);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDrawArraysInstancedARB(GLenum mode,
                                               GLint first,
                                               GLsizei count,
                                               GLsizei primcount) {
  MakeFunctionUnique("glDrawArraysInstancedARB");
  interface_->DrawArraysInstancedANGLE(mode, first, count, primcount);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDrawBuffer(GLenum mode) {
  MakeFunctionUnique("glDrawBuffer");
  interface_->DrawBuffer(mode);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDrawBuffers(GLsizei n,
                                                         const GLenum* bufs) {
  MakeFunctionUnique("glDrawBuffers");
  interface_->DrawBuffersARB(n, bufs);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDrawBuffersARB(GLsizei n, const GLenum* bufs) {
  MakeFunctionUnique("glDrawBuffersARB");
  interface_->DrawBuffersARB(n, bufs);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDrawBuffersEXT(GLsizei n, const GLenum* bufs) {
  MakeFunctionUnique("glDrawBuffersEXT");
  interface_->DrawBuffersARB(n, bufs);
}

void GL_BINDING_CALL MockGLInterface::Mock_glDrawElements(GLenum mode,
                                                          GLsizei count,
                                                          GLenum type,
                                                          const void* indices) {
  MakeFunctionUnique("glDrawElements");
  interface_->DrawElements(mode, count, type, indices);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDrawElementsInstanced(GLenum mode,
                                              GLsizei count,
                                              GLenum type,
                                              const void* indices,
                                              GLsizei primcount) {
  MakeFunctionUnique("glDrawElementsInstanced");
  interface_->DrawElementsInstancedANGLE(mode, count, type, indices, primcount);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDrawElementsInstancedANGLE(GLenum mode,
                                                   GLsizei count,
                                                   GLenum type,
                                                   const void* indices,
                                                   GLsizei primcount) {
  MakeFunctionUnique("glDrawElementsInstancedANGLE");
  interface_->DrawElementsInstancedANGLE(mode, count, type, indices, primcount);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDrawElementsInstancedARB(GLenum mode,
                                                 GLsizei count,
                                                 GLenum type,
                                                 const void* indices,
                                                 GLsizei primcount) {
  MakeFunctionUnique("glDrawElementsInstancedARB");
  interface_->DrawElementsInstancedANGLE(mode, count, type, indices, primcount);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glDrawRangeElements(GLenum mode,
                                          GLuint start,
                                          GLuint end,
                                          GLsizei count,
                                          GLenum type,
                                          const void* indices) {
  MakeFunctionUnique("glDrawRangeElements");
  interface_->DrawRangeElements(mode, start, end, count, type, indices);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glEGLImageTargetRenderbufferStorageOES(
    GLenum target,
    GLeglImageOES image) {
  MakeFunctionUnique("glEGLImageTargetRenderbufferStorageOES");
  interface_->EGLImageTargetRenderbufferStorageOES(target, image);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glEGLImageTargetTexture2DOES(GLenum target,
                                                   GLeglImageOES image) {
  MakeFunctionUnique("glEGLImageTargetTexture2DOES");
  interface_->EGLImageTargetTexture2DOES(target, image);
}

void GL_BINDING_CALL MockGLInterface::Mock_glEnable(GLenum cap) {
  MakeFunctionUnique("glEnable");
  interface_->Enable(cap);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glEnableVertexAttribArray(GLuint index) {
  MakeFunctionUnique("glEnableVertexAttribArray");
  interface_->EnableVertexAttribArray(index);
}

void GL_BINDING_CALL MockGLInterface::Mock_glEndQuery(GLenum target) {
  MakeFunctionUnique("glEndQuery");
  interface_->EndQuery(target);
}

void GL_BINDING_CALL MockGLInterface::Mock_glEndQueryARB(GLenum target) {
  MakeFunctionUnique("glEndQueryARB");
  interface_->EndQuery(target);
}

void GL_BINDING_CALL MockGLInterface::Mock_glEndQueryEXT(GLenum target) {
  MakeFunctionUnique("glEndQueryEXT");
  interface_->EndQuery(target);
}

void GL_BINDING_CALL MockGLInterface::Mock_glEndTransformFeedback(void) {
  MakeFunctionUnique("glEndTransformFeedback");
  interface_->EndTransformFeedback();
}

void GL_BINDING_CALL MockGLInterface::Mock_glEndTransformFeedbackEXT(void) {
  MakeFunctionUnique("glEndTransformFeedbackEXT");
  interface_->EndTransformFeedback();
}

GLsync GL_BINDING_CALL MockGLInterface::Mock_glFenceSync(GLenum condition,
                                                         GLbitfield flags) {
  MakeFunctionUnique("glFenceSync");
  return interface_->FenceSync(condition, flags);
}

void GL_BINDING_CALL MockGLInterface::Mock_glFinish(void) {
  MakeFunctionUnique("glFinish");
  interface_->Finish();
}

void GL_BINDING_CALL MockGLInterface::Mock_glFinishFenceAPPLE(GLuint fence) {
  MakeFunctionUnique("glFinishFenceAPPLE");
  interface_->FinishFenceAPPLE(fence);
}

void GL_BINDING_CALL MockGLInterface::Mock_glFinishFenceNV(GLuint fence) {
  MakeFunctionUnique("glFinishFenceNV");
  interface_->FinishFenceNV(fence);
}

void GL_BINDING_CALL MockGLInterface::Mock_glFlush(void) {
  MakeFunctionUnique("glFlush");
  interface_->Flush();
}

void GL_BINDING_CALL
MockGLInterface::Mock_glFlushMappedBufferRange(GLenum target,
                                               GLintptr offset,
                                               GLsizeiptr length) {
  MakeFunctionUnique("glFlushMappedBufferRange");
  interface_->FlushMappedBufferRange(target, offset, length);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glFlushMappedBufferRangeEXT(GLenum target,
                                                  GLintptr offset,
                                                  GLsizeiptr length) {
  MakeFunctionUnique("glFlushMappedBufferRangeEXT");
  interface_->FlushMappedBufferRange(target, offset, length);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glFramebufferRenderbuffer(GLenum target,
                                                GLenum attachment,
                                                GLenum renderbuffertarget,
                                                GLuint renderbuffer) {
  MakeFunctionUnique("glFramebufferRenderbuffer");
  interface_->FramebufferRenderbufferEXT(target, attachment, renderbuffertarget,
                                         renderbuffer);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glFramebufferRenderbufferEXT(GLenum target,
                                                   GLenum attachment,
                                                   GLenum renderbuffertarget,
                                                   GLuint renderbuffer) {
  MakeFunctionUnique("glFramebufferRenderbufferEXT");
  interface_->FramebufferRenderbufferEXT(target, attachment, renderbuffertarget,
                                         renderbuffer);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glFramebufferTexture2D(GLenum target,
                                             GLenum attachment,
                                             GLenum textarget,
                                             GLuint texture,
                                             GLint level) {
  MakeFunctionUnique("glFramebufferTexture2D");
  interface_->FramebufferTexture2DEXT(target, attachment, textarget, texture,
                                      level);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glFramebufferTexture2DEXT(GLenum target,
                                                GLenum attachment,
                                                GLenum textarget,
                                                GLuint texture,
                                                GLint level) {
  MakeFunctionUnique("glFramebufferTexture2DEXT");
  interface_->FramebufferTexture2DEXT(target, attachment, textarget, texture,
                                      level);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glFramebufferTexture2DMultisampleEXT(GLenum target,
                                                           GLenum attachment,
                                                           GLenum textarget,
                                                           GLuint texture,
                                                           GLint level,
                                                           GLsizei samples) {
  MakeFunctionUnique("glFramebufferTexture2DMultisampleEXT");
  interface_->FramebufferTexture2DMultisampleEXT(target, attachment, textarget,
                                                 texture, level, samples);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glFramebufferTexture2DMultisampleIMG(GLenum target,
                                                           GLenum attachment,
                                                           GLenum textarget,
                                                           GLuint texture,
                                                           GLint level,
                                                           GLsizei samples) {
  MakeFunctionUnique("glFramebufferTexture2DMultisampleIMG");
  interface_->FramebufferTexture2DMultisampleEXT(target, attachment, textarget,
                                                 texture, level, samples);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glFramebufferTextureLayer(GLenum target,
                                                GLenum attachment,
                                                GLuint texture,
                                                GLint level,
                                                GLint layer) {
  MakeFunctionUnique("glFramebufferTextureLayer");
  interface_->FramebufferTextureLayer(target, attachment, texture, level,
                                      layer);
}

void GL_BINDING_CALL MockGLInterface::Mock_glFrontFace(GLenum mode) {
  MakeFunctionUnique("glFrontFace");
  interface_->FrontFace(mode);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGenBuffers(GLsizei n,
                                                        GLuint* buffers) {
  MakeFunctionUnique("glGenBuffers");
  interface_->GenBuffersARB(n, buffers);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGenFencesAPPLE(GLsizei n,
                                                            GLuint* fences) {
  MakeFunctionUnique("glGenFencesAPPLE");
  interface_->GenFencesAPPLE(n, fences);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGenFencesNV(GLsizei n,
                                                         GLuint* fences) {
  MakeFunctionUnique("glGenFencesNV");
  interface_->GenFencesNV(n, fences);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGenFramebuffers(GLsizei n, GLuint* framebuffers) {
  MakeFunctionUnique("glGenFramebuffers");
  interface_->GenFramebuffersEXT(n, framebuffers);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGenFramebuffersEXT(GLsizei n, GLuint* framebuffers) {
  MakeFunctionUnique("glGenFramebuffersEXT");
  interface_->GenFramebuffersEXT(n, framebuffers);
}

GLuint GL_BINDING_CALL MockGLInterface::Mock_glGenPathsNV(GLsizei range) {
  MakeFunctionUnique("glGenPathsNV");
  return interface_->GenPathsNV(range);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGenQueries(GLsizei n,
                                                        GLuint* ids) {
  MakeFunctionUnique("glGenQueries");
  interface_->GenQueries(n, ids);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGenQueriesARB(GLsizei n,
                                                           GLuint* ids) {
  MakeFunctionUnique("glGenQueriesARB");
  interface_->GenQueries(n, ids);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGenQueriesEXT(GLsizei n,
                                                           GLuint* ids) {
  MakeFunctionUnique("glGenQueriesEXT");
  interface_->GenQueries(n, ids);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGenRenderbuffers(GLsizei n, GLuint* renderbuffers) {
  MakeFunctionUnique("glGenRenderbuffers");
  interface_->GenRenderbuffersEXT(n, renderbuffers);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGenRenderbuffersEXT(GLsizei n, GLuint* renderbuffers) {
  MakeFunctionUnique("glGenRenderbuffersEXT");
  interface_->GenRenderbuffersEXT(n, renderbuffers);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGenSamplers(GLsizei n,
                                                         GLuint* samplers) {
  MakeFunctionUnique("glGenSamplers");
  interface_->GenSamplers(n, samplers);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGenTextures(GLsizei n,
                                                         GLuint* textures) {
  MakeFunctionUnique("glGenTextures");
  interface_->GenTextures(n, textures);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGenTransformFeedbacks(GLsizei n, GLuint* ids) {
  MakeFunctionUnique("glGenTransformFeedbacks");
  interface_->GenTransformFeedbacks(n, ids);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGenVertexArrays(GLsizei n,
                                                             GLuint* arrays) {
  MakeFunctionUnique("glGenVertexArrays");
  interface_->GenVertexArraysOES(n, arrays);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGenVertexArraysAPPLE(GLsizei n, GLuint* arrays) {
  MakeFunctionUnique("glGenVertexArraysAPPLE");
  interface_->GenVertexArraysOES(n, arrays);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGenVertexArraysOES(GLsizei n, GLuint* arrays) {
  MakeFunctionUnique("glGenVertexArraysOES");
  interface_->GenVertexArraysOES(n, arrays);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGenerateMipmap(GLenum target) {
  MakeFunctionUnique("glGenerateMipmap");
  interface_->GenerateMipmapEXT(target);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGenerateMipmapEXT(GLenum target) {
  MakeFunctionUnique("glGenerateMipmapEXT");
  interface_->GenerateMipmapEXT(target);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetActiveAttrib(GLuint program,
                                                             GLuint index,
                                                             GLsizei bufsize,
                                                             GLsizei* length,
                                                             GLint* size,
                                                             GLenum* type,
                                                             char* name) {
  MakeFunctionUnique("glGetActiveAttrib");
  interface_->GetActiveAttrib(program, index, bufsize, length, size, type,
                              name);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetActiveUniform(GLuint program,
                                                              GLuint index,
                                                              GLsizei bufsize,
                                                              GLsizei* length,
                                                              GLint* size,
                                                              GLenum* type,
                                                              char* name) {
  MakeFunctionUnique("glGetActiveUniform");
  interface_->GetActiveUniform(program, index, bufsize, length, size, type,
                               name);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetActiveUniformBlockName(GLuint program,
                                                  GLuint uniformBlockIndex,
                                                  GLsizei bufSize,
                                                  GLsizei* length,
                                                  char* uniformBlockName) {
  MakeFunctionUnique("glGetActiveUniformBlockName");
  interface_->GetActiveUniformBlockName(program, uniformBlockIndex, bufSize,
                                        length, uniformBlockName);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetActiveUniformBlockiv(GLuint program,
                                                GLuint uniformBlockIndex,
                                                GLenum pname,
                                                GLint* params) {
  MakeFunctionUnique("glGetActiveUniformBlockiv");
  interface_->GetActiveUniformBlockiv(program, uniformBlockIndex, pname,
                                      params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetActiveUniformBlockivRobustANGLE(
    GLuint program,
    GLuint uniformBlockIndex,
    GLenum pname,
    GLsizei bufSize,
    GLsizei* length,
    GLint* params) {
  MakeFunctionUnique("glGetActiveUniformBlockivRobustANGLE");
  interface_->GetActiveUniformBlockivRobustANGLE(
      program, uniformBlockIndex, pname, bufSize, length, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetActiveUniformsiv(GLuint program,
                                            GLsizei uniformCount,
                                            const GLuint* uniformIndices,
                                            GLenum pname,
                                            GLint* params) {
  MakeFunctionUnique("glGetActiveUniformsiv");
  interface_->GetActiveUniformsiv(program, uniformCount, uniformIndices, pname,
                                  params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetAttachedShaders(GLuint program,
                                           GLsizei maxcount,
                                           GLsizei* count,
                                           GLuint* shaders) {
  MakeFunctionUnique("glGetAttachedShaders");
  interface_->GetAttachedShaders(program, maxcount, count, shaders);
}

GLint GL_BINDING_CALL
MockGLInterface::Mock_glGetAttribLocation(GLuint program, const char* name) {
  MakeFunctionUnique("glGetAttribLocation");
  return interface_->GetAttribLocation(program, name);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetBooleani_vRobustANGLE(GLenum target,
                                                 GLuint index,
                                                 GLsizei bufSize,
                                                 GLsizei* length,
                                                 GLboolean* data) {
  MakeFunctionUnique("glGetBooleani_vRobustANGLE");
  interface_->GetBooleani_vRobustANGLE(target, index, bufSize, length, data);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetBooleanv(GLenum pname,
                                                         GLboolean* params) {
  MakeFunctionUnique("glGetBooleanv");
  interface_->GetBooleanv(pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetBooleanvRobustANGLE(GLenum pname,
                                               GLsizei bufSize,
                                               GLsizei* length,
                                               GLboolean* data) {
  MakeFunctionUnique("glGetBooleanvRobustANGLE");
  interface_->GetBooleanvRobustANGLE(pname, bufSize, length, data);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetBufferParameteri64vRobustANGLE(GLenum target,
                                                          GLenum pname,
                                                          GLsizei bufSize,
                                                          GLsizei* length,
                                                          GLint64* params) {
  MakeFunctionUnique("glGetBufferParameteri64vRobustANGLE");
  interface_->GetBufferParameteri64vRobustANGLE(target, pname, bufSize, length,
                                                params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetBufferParameteriv(GLenum target,
                                             GLenum pname,
                                             GLint* params) {
  MakeFunctionUnique("glGetBufferParameteriv");
  interface_->GetBufferParameteriv(target, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetBufferParameterivRobustANGLE(GLenum target,
                                                        GLenum pname,
                                                        GLsizei bufSize,
                                                        GLsizei* length,
                                                        GLint* params) {
  MakeFunctionUnique("glGetBufferParameterivRobustANGLE");
  interface_->GetBufferParameterivRobustANGLE(target, pname, bufSize, length,
                                              params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetBufferPointervRobustANGLE(GLenum target,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei* length,
                                                     void** params) {
  MakeFunctionUnique("glGetBufferPointervRobustANGLE");
  interface_->GetBufferPointervRobustANGLE(target, pname, bufSize, length,
                                           params);
}

GLuint GL_BINDING_CALL
MockGLInterface::Mock_glGetDebugMessageLog(GLuint count,
                                           GLsizei bufSize,
                                           GLenum* sources,
                                           GLenum* types,
                                           GLuint* ids,
                                           GLenum* severities,
                                           GLsizei* lengths,
                                           char* messageLog) {
  MakeFunctionUnique("glGetDebugMessageLog");
  return interface_->GetDebugMessageLog(count, bufSize, sources, types, ids,
                                        severities, lengths, messageLog);
}

GLuint GL_BINDING_CALL
MockGLInterface::Mock_glGetDebugMessageLogKHR(GLuint count,
                                              GLsizei bufSize,
                                              GLenum* sources,
                                              GLenum* types,
                                              GLuint* ids,
                                              GLenum* severities,
                                              GLsizei* lengths,
                                              char* messageLog) {
  MakeFunctionUnique("glGetDebugMessageLogKHR");
  return interface_->GetDebugMessageLog(count, bufSize, sources, types, ids,
                                        severities, lengths, messageLog);
}

GLenum GL_BINDING_CALL MockGLInterface::Mock_glGetError(void) {
  MakeFunctionUnique("glGetError");
  return interface_->GetError();
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetFenceivNV(GLuint fence,
                                                          GLenum pname,
                                                          GLint* params) {
  MakeFunctionUnique("glGetFenceivNV");
  interface_->GetFenceivNV(fence, pname, params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetFloatv(GLenum pname,
                                                       GLfloat* params) {
  MakeFunctionUnique("glGetFloatv");
  interface_->GetFloatv(pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetFloatvRobustANGLE(GLenum pname,
                                             GLsizei bufSize,
                                             GLsizei* length,
                                             GLfloat* data) {
  MakeFunctionUnique("glGetFloatvRobustANGLE");
  interface_->GetFloatvRobustANGLE(pname, bufSize, length, data);
}

GLint GL_BINDING_CALL
MockGLInterface::Mock_glGetFragDataIndex(GLuint program, const char* name) {
  MakeFunctionUnique("glGetFragDataIndex");
  return interface_->GetFragDataIndex(program, name);
}

GLint GL_BINDING_CALL
MockGLInterface::Mock_glGetFragDataIndexEXT(GLuint program, const char* name) {
  MakeFunctionUnique("glGetFragDataIndexEXT");
  return interface_->GetFragDataIndex(program, name);
}

GLint GL_BINDING_CALL
MockGLInterface::Mock_glGetFragDataLocation(GLuint program, const char* name) {
  MakeFunctionUnique("glGetFragDataLocation");
  return interface_->GetFragDataLocation(program, name);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetFramebufferAttachmentParameteriv(GLenum target,
                                                            GLenum attachment,
                                                            GLenum pname,
                                                            GLint* params) {
  MakeFunctionUnique("glGetFramebufferAttachmentParameteriv");
  interface_->GetFramebufferAttachmentParameterivEXT(target, attachment, pname,
                                                     params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetFramebufferAttachmentParameterivEXT(
    GLenum target,
    GLenum attachment,
    GLenum pname,
    GLint* params) {
  MakeFunctionUnique("glGetFramebufferAttachmentParameterivEXT");
  interface_->GetFramebufferAttachmentParameterivEXT(target, attachment, pname,
                                                     params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetFramebufferAttachmentParameterivRobustANGLE(
    GLenum target,
    GLenum attachment,
    GLenum pname,
    GLsizei bufSize,
    GLsizei* length,
    GLint* params) {
  MakeFunctionUnique("glGetFramebufferAttachmentParameterivRobustANGLE");
  interface_->GetFramebufferAttachmentParameterivRobustANGLE(
      target, attachment, pname, bufSize, length, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetFramebufferParameterivRobustANGLE(GLenum target,
                                                             GLenum pname,
                                                             GLsizei bufSize,
                                                             GLsizei* length,
                                                             GLint* params) {
  MakeFunctionUnique("glGetFramebufferParameterivRobustANGLE");
  interface_->GetFramebufferParameterivRobustANGLE(target, pname, bufSize,
                                                   length, params);
}

GLenum GL_BINDING_CALL MockGLInterface::Mock_glGetGraphicsResetStatus(void) {
  MakeFunctionUnique("glGetGraphicsResetStatus");
  return interface_->GetGraphicsResetStatusARB();
}

GLenum GL_BINDING_CALL MockGLInterface::Mock_glGetGraphicsResetStatusARB(void) {
  MakeFunctionUnique("glGetGraphicsResetStatusARB");
  return interface_->GetGraphicsResetStatusARB();
}

GLenum GL_BINDING_CALL MockGLInterface::Mock_glGetGraphicsResetStatusEXT(void) {
  MakeFunctionUnique("glGetGraphicsResetStatusEXT");
  return interface_->GetGraphicsResetStatusARB();
}

GLenum GL_BINDING_CALL MockGLInterface::Mock_glGetGraphicsResetStatusKHR(void) {
  MakeFunctionUnique("glGetGraphicsResetStatusKHR");
  return interface_->GetGraphicsResetStatusARB();
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetInteger64i_v(GLenum target,
                                                             GLuint index,
                                                             GLint64* data) {
  MakeFunctionUnique("glGetInteger64i_v");
  interface_->GetInteger64i_v(target, index, data);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetInteger64i_vRobustANGLE(GLenum target,
                                                   GLuint index,
                                                   GLsizei bufSize,
                                                   GLsizei* length,
                                                   GLint64* data) {
  MakeFunctionUnique("glGetInteger64i_vRobustANGLE");
  interface_->GetInteger64i_vRobustANGLE(target, index, bufSize, length, data);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetInteger64v(GLenum pname,
                                                           GLint64* params) {
  MakeFunctionUnique("glGetInteger64v");
  interface_->GetInteger64v(pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetInteger64vRobustANGLE(GLenum pname,
                                                 GLsizei bufSize,
                                                 GLsizei* length,
                                                 GLint64* data) {
  MakeFunctionUnique("glGetInteger64vRobustANGLE");
  interface_->GetInteger64vRobustANGLE(pname, bufSize, length, data);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetIntegeri_v(GLenum target,
                                                           GLuint index,
                                                           GLint* data) {
  MakeFunctionUnique("glGetIntegeri_v");
  interface_->GetIntegeri_v(target, index, data);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetIntegeri_vRobustANGLE(GLenum target,
                                                 GLuint index,
                                                 GLsizei bufSize,
                                                 GLsizei* length,
                                                 GLint* data) {
  MakeFunctionUnique("glGetIntegeri_vRobustANGLE");
  interface_->GetIntegeri_vRobustANGLE(target, index, bufSize, length, data);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetIntegerv(GLenum pname,
                                                         GLint* params) {
  MakeFunctionUnique("glGetIntegerv");
  interface_->GetIntegerv(pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetIntegervRobustANGLE(GLenum pname,
                                               GLsizei bufSize,
                                               GLsizei* length,
                                               GLint* data) {
  MakeFunctionUnique("glGetIntegervRobustANGLE");
  interface_->GetIntegervRobustANGLE(pname, bufSize, length, data);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetInternalformativ(GLenum target,
                                            GLenum internalformat,
                                            GLenum pname,
                                            GLsizei bufSize,
                                            GLint* params) {
  MakeFunctionUnique("glGetInternalformativ");
  interface_->GetInternalformativ(target, internalformat, pname, bufSize,
                                  params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetInternalformativRobustANGLE(GLenum target,
                                                       GLenum internalformat,
                                                       GLenum pname,
                                                       GLsizei bufSize,
                                                       GLsizei* length,
                                                       GLint* params) {
  MakeFunctionUnique("glGetInternalformativRobustANGLE");
  interface_->GetInternalformativRobustANGLE(target, internalformat, pname,
                                             bufSize, length, params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetMultisamplefv(GLenum pname,
                                                              GLuint index,
                                                              GLfloat* val) {
  MakeFunctionUnique("glGetMultisamplefv");
  interface_->GetMultisamplefv(pname, index, val);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetMultisamplefvRobustANGLE(GLenum pname,
                                                    GLuint index,
                                                    GLsizei bufSize,
                                                    GLsizei* length,
                                                    GLfloat* val) {
  MakeFunctionUnique("glGetMultisamplefvRobustANGLE");
  interface_->GetMultisamplefvRobustANGLE(pname, index, bufSize, length, val);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetObjectLabel(GLenum identifier,
                                                            GLuint name,
                                                            GLsizei bufSize,
                                                            GLsizei* length,
                                                            char* label) {
  MakeFunctionUnique("glGetObjectLabel");
  interface_->GetObjectLabel(identifier, name, bufSize, length, label);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetObjectLabelKHR(GLenum identifier,
                                          GLuint name,
                                          GLsizei bufSize,
                                          GLsizei* length,
                                          char* label) {
  MakeFunctionUnique("glGetObjectLabelKHR");
  interface_->GetObjectLabel(identifier, name, bufSize, length, label);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetObjectPtrLabel(void* ptr,
                                                               GLsizei bufSize,
                                                               GLsizei* length,
                                                               char* label) {
  MakeFunctionUnique("glGetObjectPtrLabel");
  interface_->GetObjectPtrLabel(ptr, bufSize, length, label);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetObjectPtrLabelKHR(void* ptr,
                                             GLsizei bufSize,
                                             GLsizei* length,
                                             char* label) {
  MakeFunctionUnique("glGetObjectPtrLabelKHR");
  interface_->GetObjectPtrLabel(ptr, bufSize, length, label);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetPointerv(GLenum pname,
                                                         void** params) {
  MakeFunctionUnique("glGetPointerv");
  interface_->GetPointerv(pname, params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetPointervKHR(GLenum pname,
                                                            void** params) {
  MakeFunctionUnique("glGetPointervKHR");
  interface_->GetPointerv(pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetPointervRobustANGLERobustANGLE(GLenum pname,
                                                          GLsizei bufSize,
                                                          GLsizei* length,
                                                          void** params) {
  MakeFunctionUnique("glGetPointervRobustANGLERobustANGLE");
  interface_->GetPointervRobustANGLERobustANGLE(pname, bufSize, length, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetProgramBinary(GLuint program,
                                         GLsizei bufSize,
                                         GLsizei* length,
                                         GLenum* binaryFormat,
                                         GLvoid* binary) {
  MakeFunctionUnique("glGetProgramBinary");
  interface_->GetProgramBinary(program, bufSize, length, binaryFormat, binary);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetProgramBinaryOES(GLuint program,
                                            GLsizei bufSize,
                                            GLsizei* length,
                                            GLenum* binaryFormat,
                                            GLvoid* binary) {
  MakeFunctionUnique("glGetProgramBinaryOES");
  interface_->GetProgramBinary(program, bufSize, length, binaryFormat, binary);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetProgramInfoLog(GLuint program,
                                                               GLsizei bufsize,
                                                               GLsizei* length,
                                                               char* infolog) {
  MakeFunctionUnique("glGetProgramInfoLog");
  interface_->GetProgramInfoLog(program, bufsize, length, infolog);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetProgramInterfaceiv(GLuint program,
                                              GLenum programInterface,
                                              GLenum pname,
                                              GLint* params) {
  MakeFunctionUnique("glGetProgramInterfaceiv");
  interface_->GetProgramInterfaceiv(program, programInterface, pname, params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetProgramInterfaceivRobustANGLE(
    GLuint program,
    GLenum programInterface,
    GLenum pname,
    GLsizei bufSize,
    GLsizei* length,
    GLint* params) {
  MakeFunctionUnique("glGetProgramInterfaceivRobustANGLE");
  interface_->GetProgramInterfaceivRobustANGLE(program, programInterface, pname,
                                               bufSize, length, params);
}

GLint GL_BINDING_CALL
MockGLInterface::Mock_glGetProgramResourceLocation(GLuint program,
                                                   GLenum programInterface,
                                                   const char* name) {
  MakeFunctionUnique("glGetProgramResourceLocation");
  return interface_->GetProgramResourceLocation(program, programInterface,
                                                name);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetProgramResourceName(GLuint program,
                                               GLenum programInterface,
                                               GLuint index,
                                               GLsizei bufSize,
                                               GLsizei* length,
                                               GLchar* name) {
  MakeFunctionUnique("glGetProgramResourceName");
  interface_->GetProgramResourceName(program, programInterface, index, bufSize,
                                     length, name);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetProgramResourceiv(GLuint program,
                                             GLenum programInterface,
                                             GLuint index,
                                             GLsizei propCount,
                                             const GLenum* props,
                                             GLsizei bufSize,
                                             GLsizei* length,
                                             GLint* params) {
  MakeFunctionUnique("glGetProgramResourceiv");
  interface_->GetProgramResourceiv(program, programInterface, index, propCount,
                                   props, bufSize, length, params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetProgramiv(GLuint program,
                                                          GLenum pname,
                                                          GLint* params) {
  MakeFunctionUnique("glGetProgramiv");
  interface_->GetProgramiv(program, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetProgramivRobustANGLE(GLuint program,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei* length,
                                                GLint* params) {
  MakeFunctionUnique("glGetProgramivRobustANGLE");
  interface_->GetProgramivRobustANGLE(program, pname, bufSize, length, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryObjecti64v(GLuint id,
                                           GLenum pname,
                                           GLint64* params) {
  MakeFunctionUnique("glGetQueryObjecti64v");
  interface_->GetQueryObjecti64v(id, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryObjecti64vEXT(GLuint id,
                                              GLenum pname,
                                              GLint64* params) {
  MakeFunctionUnique("glGetQueryObjecti64vEXT");
  interface_->GetQueryObjecti64v(id, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryObjecti64vRobustANGLE(GLuint id,
                                                      GLenum pname,
                                                      GLsizei bufSize,
                                                      GLsizei* length,
                                                      GLint64* params) {
  MakeFunctionUnique("glGetQueryObjecti64vRobustANGLE");
  interface_->GetQueryObjecti64vRobustANGLE(id, pname, bufSize, length, params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetQueryObjectiv(GLuint id,
                                                              GLenum pname,
                                                              GLint* params) {
  MakeFunctionUnique("glGetQueryObjectiv");
  interface_->GetQueryObjectiv(id, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryObjectivARB(GLuint id,
                                            GLenum pname,
                                            GLint* params) {
  MakeFunctionUnique("glGetQueryObjectivARB");
  interface_->GetQueryObjectiv(id, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryObjectivEXT(GLuint id,
                                            GLenum pname,
                                            GLint* params) {
  MakeFunctionUnique("glGetQueryObjectivEXT");
  interface_->GetQueryObjectiv(id, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryObjectivRobustANGLE(GLuint id,
                                                    GLenum pname,
                                                    GLsizei bufSize,
                                                    GLsizei* length,
                                                    GLint* params) {
  MakeFunctionUnique("glGetQueryObjectivRobustANGLE");
  interface_->GetQueryObjectivRobustANGLE(id, pname, bufSize, length, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryObjectui64v(GLuint id,
                                            GLenum pname,
                                            GLuint64* params) {
  MakeFunctionUnique("glGetQueryObjectui64v");
  interface_->GetQueryObjectui64v(id, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryObjectui64vEXT(GLuint id,
                                               GLenum pname,
                                               GLuint64* params) {
  MakeFunctionUnique("glGetQueryObjectui64vEXT");
  interface_->GetQueryObjectui64v(id, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryObjectui64vRobustANGLE(GLuint id,
                                                       GLenum pname,
                                                       GLsizei bufSize,
                                                       GLsizei* length,
                                                       GLuint64* params) {
  MakeFunctionUnique("glGetQueryObjectui64vRobustANGLE");
  interface_->GetQueryObjectui64vRobustANGLE(id, pname, bufSize, length,
                                             params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetQueryObjectuiv(GLuint id,
                                                               GLenum pname,
                                                               GLuint* params) {
  MakeFunctionUnique("glGetQueryObjectuiv");
  interface_->GetQueryObjectuiv(id, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryObjectuivARB(GLuint id,
                                             GLenum pname,
                                             GLuint* params) {
  MakeFunctionUnique("glGetQueryObjectuivARB");
  interface_->GetQueryObjectuiv(id, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryObjectuivEXT(GLuint id,
                                             GLenum pname,
                                             GLuint* params) {
  MakeFunctionUnique("glGetQueryObjectuivEXT");
  interface_->GetQueryObjectuiv(id, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryObjectuivRobustANGLE(GLuint id,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei* length,
                                                     GLuint* params) {
  MakeFunctionUnique("glGetQueryObjectuivRobustANGLE");
  interface_->GetQueryObjectuivRobustANGLE(id, pname, bufSize, length, params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetQueryiv(GLenum target,
                                                        GLenum pname,
                                                        GLint* params) {
  MakeFunctionUnique("glGetQueryiv");
  interface_->GetQueryiv(target, pname, params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetQueryivARB(GLenum target,
                                                           GLenum pname,
                                                           GLint* params) {
  MakeFunctionUnique("glGetQueryivARB");
  interface_->GetQueryiv(target, pname, params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetQueryivEXT(GLenum target,
                                                           GLenum pname,
                                                           GLint* params) {
  MakeFunctionUnique("glGetQueryivEXT");
  interface_->GetQueryiv(target, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetQueryivRobustANGLE(GLenum target,
                                              GLenum pname,
                                              GLsizei bufSize,
                                              GLsizei* length,
                                              GLint* params) {
  MakeFunctionUnique("glGetQueryivRobustANGLE");
  interface_->GetQueryivRobustANGLE(target, pname, bufSize, length, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetRenderbufferParameteriv(GLenum target,
                                                   GLenum pname,
                                                   GLint* params) {
  MakeFunctionUnique("glGetRenderbufferParameteriv");
  interface_->GetRenderbufferParameterivEXT(target, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetRenderbufferParameterivEXT(GLenum target,
                                                      GLenum pname,
                                                      GLint* params) {
  MakeFunctionUnique("glGetRenderbufferParameterivEXT");
  interface_->GetRenderbufferParameterivEXT(target, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetRenderbufferParameterivRobustANGLE(GLenum target,
                                                              GLenum pname,
                                                              GLsizei bufSize,
                                                              GLsizei* length,
                                                              GLint* params) {
  MakeFunctionUnique("glGetRenderbufferParameterivRobustANGLE");
  interface_->GetRenderbufferParameterivRobustANGLE(target, pname, bufSize,
                                                    length, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetSamplerParameterIivRobustANGLE(GLuint sampler,
                                                          GLenum pname,
                                                          GLsizei bufSize,
                                                          GLsizei* length,
                                                          GLint* params) {
  MakeFunctionUnique("glGetSamplerParameterIivRobustANGLE");
  interface_->GetSamplerParameterIivRobustANGLE(sampler, pname, bufSize, length,
                                                params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetSamplerParameterIuivRobustANGLE(GLuint sampler,
                                                           GLenum pname,
                                                           GLsizei bufSize,
                                                           GLsizei* length,
                                                           GLuint* params) {
  MakeFunctionUnique("glGetSamplerParameterIuivRobustANGLE");
  interface_->GetSamplerParameterIuivRobustANGLE(sampler, pname, bufSize,
                                                 length, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetSamplerParameterfv(GLuint sampler,
                                              GLenum pname,
                                              GLfloat* params) {
  MakeFunctionUnique("glGetSamplerParameterfv");
  interface_->GetSamplerParameterfv(sampler, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetSamplerParameterfvRobustANGLE(GLuint sampler,
                                                         GLenum pname,
                                                         GLsizei bufSize,
                                                         GLsizei* length,
                                                         GLfloat* params) {
  MakeFunctionUnique("glGetSamplerParameterfvRobustANGLE");
  interface_->GetSamplerParameterfvRobustANGLE(sampler, pname, bufSize, length,
                                               params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetSamplerParameteriv(GLuint sampler,
                                              GLenum pname,
                                              GLint* params) {
  MakeFunctionUnique("glGetSamplerParameteriv");
  interface_->GetSamplerParameteriv(sampler, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetSamplerParameterivRobustANGLE(GLuint sampler,
                                                         GLenum pname,
                                                         GLsizei bufSize,
                                                         GLsizei* length,
                                                         GLint* params) {
  MakeFunctionUnique("glGetSamplerParameterivRobustANGLE");
  interface_->GetSamplerParameterivRobustANGLE(sampler, pname, bufSize, length,
                                               params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetShaderInfoLog(GLuint shader,
                                                              GLsizei bufsize,
                                                              GLsizei* length,
                                                              char* infolog) {
  MakeFunctionUnique("glGetShaderInfoLog");
  interface_->GetShaderInfoLog(shader, bufsize, length, infolog);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetShaderPrecisionFormat(GLenum shadertype,
                                                 GLenum precisiontype,
                                                 GLint* range,
                                                 GLint* precision) {
  MakeFunctionUnique("glGetShaderPrecisionFormat");
  interface_->GetShaderPrecisionFormat(shadertype, precisiontype, range,
                                       precision);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetShaderSource(GLuint shader,
                                                             GLsizei bufsize,
                                                             GLsizei* length,
                                                             char* source) {
  MakeFunctionUnique("glGetShaderSource");
  interface_->GetShaderSource(shader, bufsize, length, source);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetShaderiv(GLuint shader,
                                                         GLenum pname,
                                                         GLint* params) {
  MakeFunctionUnique("glGetShaderiv");
  interface_->GetShaderiv(shader, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetShaderivRobustANGLE(GLuint shader,
                                               GLenum pname,
                                               GLsizei bufSize,
                                               GLsizei* length,
                                               GLint* params) {
  MakeFunctionUnique("glGetShaderivRobustANGLE");
  interface_->GetShaderivRobustANGLE(shader, pname, bufSize, length, params);
}

const GLubyte* GL_BINDING_CALL MockGLInterface::Mock_glGetString(GLenum name) {
  MakeFunctionUnique("glGetString");
  return interface_->GetString(name);
}

const GLubyte* GL_BINDING_CALL
MockGLInterface::Mock_glGetStringi(GLenum name, GLuint index) {
  MakeFunctionUnique("glGetStringi");
  return interface_->GetStringi(name, index);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetSynciv(GLsync sync,
                                                       GLenum pname,
                                                       GLsizei bufSize,
                                                       GLsizei* length,
                                                       GLint* values) {
  MakeFunctionUnique("glGetSynciv");
  interface_->GetSynciv(sync, pname, bufSize, length, values);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetTexLevelParameterfv(GLenum target,
                                               GLint level,
                                               GLenum pname,
                                               GLfloat* params) {
  MakeFunctionUnique("glGetTexLevelParameterfv");
  interface_->GetTexLevelParameterfv(target, level, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetTexLevelParameterfvRobustANGLE(GLenum target,
                                                          GLint level,
                                                          GLenum pname,
                                                          GLsizei bufSize,
                                                          GLsizei* length,
                                                          GLfloat* params) {
  MakeFunctionUnique("glGetTexLevelParameterfvRobustANGLE");
  interface_->GetTexLevelParameterfvRobustANGLE(target, level, pname, bufSize,
                                                length, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetTexLevelParameteriv(GLenum target,
                                               GLint level,
                                               GLenum pname,
                                               GLint* params) {
  MakeFunctionUnique("glGetTexLevelParameteriv");
  interface_->GetTexLevelParameteriv(target, level, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetTexLevelParameterivRobustANGLE(GLenum target,
                                                          GLint level,
                                                          GLenum pname,
                                                          GLsizei bufSize,
                                                          GLsizei* length,
                                                          GLint* params) {
  MakeFunctionUnique("glGetTexLevelParameterivRobustANGLE");
  interface_->GetTexLevelParameterivRobustANGLE(target, level, pname, bufSize,
                                                length, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetTexParameterIivRobustANGLE(GLenum target,
                                                      GLenum pname,
                                                      GLsizei bufSize,
                                                      GLsizei* length,
                                                      GLint* params) {
  MakeFunctionUnique("glGetTexParameterIivRobustANGLE");
  interface_->GetTexParameterIivRobustANGLE(target, pname, bufSize, length,
                                            params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetTexParameterIuivRobustANGLE(GLenum target,
                                                       GLenum pname,
                                                       GLsizei bufSize,
                                                       GLsizei* length,
                                                       GLuint* params) {
  MakeFunctionUnique("glGetTexParameterIuivRobustANGLE");
  interface_->GetTexParameterIuivRobustANGLE(target, pname, bufSize, length,
                                             params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetTexParameterfv(GLenum target,
                                          GLenum pname,
                                          GLfloat* params) {
  MakeFunctionUnique("glGetTexParameterfv");
  interface_->GetTexParameterfv(target, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetTexParameterfvRobustANGLE(GLenum target,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei* length,
                                                     GLfloat* params) {
  MakeFunctionUnique("glGetTexParameterfvRobustANGLE");
  interface_->GetTexParameterfvRobustANGLE(target, pname, bufSize, length,
                                           params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetTexParameteriv(GLenum target,
                                                               GLenum pname,
                                                               GLint* params) {
  MakeFunctionUnique("glGetTexParameteriv");
  interface_->GetTexParameteriv(target, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetTexParameterivRobustANGLE(GLenum target,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei* length,
                                                     GLint* params) {
  MakeFunctionUnique("glGetTexParameterivRobustANGLE");
  interface_->GetTexParameterivRobustANGLE(target, pname, bufSize, length,
                                           params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetTransformFeedbackVarying(GLuint program,
                                                    GLuint index,
                                                    GLsizei bufSize,
                                                    GLsizei* length,
                                                    GLsizei* size,
                                                    GLenum* type,
                                                    char* name) {
  MakeFunctionUnique("glGetTransformFeedbackVarying");
  interface_->GetTransformFeedbackVarying(program, index, bufSize, length, size,
                                          type, name);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetTransformFeedbackVaryingEXT(GLuint program,
                                                       GLuint index,
                                                       GLsizei bufSize,
                                                       GLsizei* length,
                                                       GLsizei* size,
                                                       GLenum* type,
                                                       char* name) {
  MakeFunctionUnique("glGetTransformFeedbackVaryingEXT");
  interface_->GetTransformFeedbackVarying(program, index, bufSize, length, size,
                                          type, name);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetTranslatedShaderSourceANGLE(GLuint shader,
                                                       GLsizei bufsize,
                                                       GLsizei* length,
                                                       char* source) {
  MakeFunctionUnique("glGetTranslatedShaderSourceANGLE");
  interface_->GetTranslatedShaderSourceANGLE(shader, bufsize, length, source);
}

GLuint GL_BINDING_CALL
MockGLInterface::Mock_glGetUniformBlockIndex(GLuint program,
                                             const char* uniformBlockName) {
  MakeFunctionUnique("glGetUniformBlockIndex");
  return interface_->GetUniformBlockIndex(program, uniformBlockName);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetUniformIndices(GLuint program,
                                          GLsizei uniformCount,
                                          const char* const* uniformNames,
                                          GLuint* uniformIndices) {
  MakeFunctionUnique("glGetUniformIndices");
  interface_->GetUniformIndices(program, uniformCount, uniformNames,
                                uniformIndices);
}

GLint GL_BINDING_CALL
MockGLInterface::Mock_glGetUniformLocation(GLuint program, const char* name) {
  MakeFunctionUnique("glGetUniformLocation");
  return interface_->GetUniformLocation(program, name);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetUniformfv(GLuint program,
                                                          GLint location,
                                                          GLfloat* params) {
  MakeFunctionUnique("glGetUniformfv");
  interface_->GetUniformfv(program, location, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetUniformfvRobustANGLE(GLuint program,
                                                GLint location,
                                                GLsizei bufSize,
                                                GLsizei* length,
                                                GLfloat* params) {
  MakeFunctionUnique("glGetUniformfvRobustANGLE");
  interface_->GetUniformfvRobustANGLE(program, location, bufSize, length,
                                      params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetUniformiv(GLuint program,
                                                          GLint location,
                                                          GLint* params) {
  MakeFunctionUnique("glGetUniformiv");
  interface_->GetUniformiv(program, location, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetUniformivRobustANGLE(GLuint program,
                                                GLint location,
                                                GLsizei bufSize,
                                                GLsizei* length,
                                                GLint* params) {
  MakeFunctionUnique("glGetUniformivRobustANGLE");
  interface_->GetUniformivRobustANGLE(program, location, bufSize, length,
                                      params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetUniformuiv(GLuint program,
                                                           GLint location,
                                                           GLuint* params) {
  MakeFunctionUnique("glGetUniformuiv");
  interface_->GetUniformuiv(program, location, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetUniformuivRobustANGLE(GLuint program,
                                                 GLint location,
                                                 GLsizei bufSize,
                                                 GLsizei* length,
                                                 GLuint* params) {
  MakeFunctionUnique("glGetUniformuivRobustANGLE");
  interface_->GetUniformuivRobustANGLE(program, location, bufSize, length,
                                       params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetVertexAttribIivRobustANGLE(GLuint index,
                                                      GLenum pname,
                                                      GLsizei bufSize,
                                                      GLsizei* length,
                                                      GLint* params) {
  MakeFunctionUnique("glGetVertexAttribIivRobustANGLE");
  interface_->GetVertexAttribIivRobustANGLE(index, pname, bufSize, length,
                                            params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetVertexAttribIuivRobustANGLE(GLuint index,
                                                       GLenum pname,
                                                       GLsizei bufSize,
                                                       GLsizei* length,
                                                       GLuint* params) {
  MakeFunctionUnique("glGetVertexAttribIuivRobustANGLE");
  interface_->GetVertexAttribIuivRobustANGLE(index, pname, bufSize, length,
                                             params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetVertexAttribPointerv(GLuint index,
                                                GLenum pname,
                                                void** pointer) {
  MakeFunctionUnique("glGetVertexAttribPointerv");
  interface_->GetVertexAttribPointerv(index, pname, pointer);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetVertexAttribPointervRobustANGLE(GLuint index,
                                                           GLenum pname,
                                                           GLsizei bufSize,
                                                           GLsizei* length,
                                                           void** pointer) {
  MakeFunctionUnique("glGetVertexAttribPointervRobustANGLE");
  interface_->GetVertexAttribPointervRobustANGLE(index, pname, bufSize, length,
                                                 pointer);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetVertexAttribfv(GLuint index,
                                          GLenum pname,
                                          GLfloat* params) {
  MakeFunctionUnique("glGetVertexAttribfv");
  interface_->GetVertexAttribfv(index, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetVertexAttribfvRobustANGLE(GLuint index,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei* length,
                                                     GLfloat* params) {
  MakeFunctionUnique("glGetVertexAttribfvRobustANGLE");
  interface_->GetVertexAttribfvRobustANGLE(index, pname, bufSize, length,
                                           params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glGetVertexAttribiv(GLuint index,
                                                               GLenum pname,
                                                               GLint* params) {
  MakeFunctionUnique("glGetVertexAttribiv");
  interface_->GetVertexAttribiv(index, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetVertexAttribivRobustANGLE(GLuint index,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei* length,
                                                     GLint* params) {
  MakeFunctionUnique("glGetVertexAttribivRobustANGLE");
  interface_->GetVertexAttribivRobustANGLE(index, pname, bufSize, length,
                                           params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetnUniformfvRobustANGLE(GLuint program,
                                                 GLint location,
                                                 GLsizei bufSize,
                                                 GLsizei* length,
                                                 GLfloat* params) {
  MakeFunctionUnique("glGetnUniformfvRobustANGLE");
  interface_->GetnUniformfvRobustANGLE(program, location, bufSize, length,
                                       params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetnUniformivRobustANGLE(GLuint program,
                                                 GLint location,
                                                 GLsizei bufSize,
                                                 GLsizei* length,
                                                 GLint* params) {
  MakeFunctionUnique("glGetnUniformivRobustANGLE");
  interface_->GetnUniformivRobustANGLE(program, location, bufSize, length,
                                       params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glGetnUniformuivRobustANGLE(GLuint program,
                                                  GLint location,
                                                  GLsizei bufSize,
                                                  GLsizei* length,
                                                  GLuint* params) {
  MakeFunctionUnique("glGetnUniformuivRobustANGLE");
  interface_->GetnUniformuivRobustANGLE(program, location, bufSize, length,
                                        params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glHint(GLenum target, GLenum mode) {
  MakeFunctionUnique("glHint");
  interface_->Hint(target, mode);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glInsertEventMarkerEXT(GLsizei length,
                                             const char* marker) {
  MakeFunctionUnique("glInsertEventMarkerEXT");
  interface_->InsertEventMarkerEXT(length, marker);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glInvalidateFramebuffer(GLenum target,
                                              GLsizei numAttachments,
                                              const GLenum* attachments) {
  MakeFunctionUnique("glInvalidateFramebuffer");
  interface_->InvalidateFramebuffer(target, numAttachments, attachments);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glInvalidateSubFramebuffer(GLenum target,
                                                 GLsizei numAttachments,
                                                 const GLenum* attachments,
                                                 GLint x,
                                                 GLint y,
                                                 GLint width,
                                                 GLint height) {
  MakeFunctionUnique("glInvalidateSubFramebuffer");
  interface_->InvalidateSubFramebuffer(target, numAttachments, attachments, x,
                                       y, width, height);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsBuffer(GLuint buffer) {
  MakeFunctionUnique("glIsBuffer");
  return interface_->IsBuffer(buffer);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsEnabled(GLenum cap) {
  MakeFunctionUnique("glIsEnabled");
  return interface_->IsEnabled(cap);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsFenceAPPLE(GLuint fence) {
  MakeFunctionUnique("glIsFenceAPPLE");
  return interface_->IsFenceAPPLE(fence);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsFenceNV(GLuint fence) {
  MakeFunctionUnique("glIsFenceNV");
  return interface_->IsFenceNV(fence);
}

GLboolean GL_BINDING_CALL
MockGLInterface::Mock_glIsFramebuffer(GLuint framebuffer) {
  MakeFunctionUnique("glIsFramebuffer");
  return interface_->IsFramebufferEXT(framebuffer);
}

GLboolean GL_BINDING_CALL
MockGLInterface::Mock_glIsFramebufferEXT(GLuint framebuffer) {
  MakeFunctionUnique("glIsFramebufferEXT");
  return interface_->IsFramebufferEXT(framebuffer);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsPathNV(GLuint path) {
  MakeFunctionUnique("glIsPathNV");
  return interface_->IsPathNV(path);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsProgram(GLuint program) {
  MakeFunctionUnique("glIsProgram");
  return interface_->IsProgram(program);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsQuery(GLuint query) {
  MakeFunctionUnique("glIsQuery");
  return interface_->IsQuery(query);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsQueryARB(GLuint query) {
  MakeFunctionUnique("glIsQueryARB");
  return interface_->IsQuery(query);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsQueryEXT(GLuint query) {
  MakeFunctionUnique("glIsQueryEXT");
  return interface_->IsQuery(query);
}

GLboolean GL_BINDING_CALL
MockGLInterface::Mock_glIsRenderbuffer(GLuint renderbuffer) {
  MakeFunctionUnique("glIsRenderbuffer");
  return interface_->IsRenderbufferEXT(renderbuffer);
}

GLboolean GL_BINDING_CALL
MockGLInterface::Mock_glIsRenderbufferEXT(GLuint renderbuffer) {
  MakeFunctionUnique("glIsRenderbufferEXT");
  return interface_->IsRenderbufferEXT(renderbuffer);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsSampler(GLuint sampler) {
  MakeFunctionUnique("glIsSampler");
  return interface_->IsSampler(sampler);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsShader(GLuint shader) {
  MakeFunctionUnique("glIsShader");
  return interface_->IsShader(shader);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsSync(GLsync sync) {
  MakeFunctionUnique("glIsSync");
  return interface_->IsSync(sync);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsTexture(GLuint texture) {
  MakeFunctionUnique("glIsTexture");
  return interface_->IsTexture(texture);
}

GLboolean GL_BINDING_CALL
MockGLInterface::Mock_glIsTransformFeedback(GLuint id) {
  MakeFunctionUnique("glIsTransformFeedback");
  return interface_->IsTransformFeedback(id);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glIsVertexArray(GLuint array) {
  MakeFunctionUnique("glIsVertexArray");
  return interface_->IsVertexArrayOES(array);
}

GLboolean GL_BINDING_CALL
MockGLInterface::Mock_glIsVertexArrayAPPLE(GLuint array) {
  MakeFunctionUnique("glIsVertexArrayAPPLE");
  return interface_->IsVertexArrayOES(array);
}

GLboolean GL_BINDING_CALL
MockGLInterface::Mock_glIsVertexArrayOES(GLuint array) {
  MakeFunctionUnique("glIsVertexArrayOES");
  return interface_->IsVertexArrayOES(array);
}

void GL_BINDING_CALL MockGLInterface::Mock_glLineWidth(GLfloat width) {
  MakeFunctionUnique("glLineWidth");
  interface_->LineWidth(width);
}

void GL_BINDING_CALL MockGLInterface::Mock_glLinkProgram(GLuint program) {
  MakeFunctionUnique("glLinkProgram");
  interface_->LinkProgram(program);
}

void* GL_BINDING_CALL MockGLInterface::Mock_glMapBuffer(GLenum target,
                                                        GLenum access) {
  MakeFunctionUnique("glMapBuffer");
  return interface_->MapBuffer(target, access);
}

void* GL_BINDING_CALL MockGLInterface::Mock_glMapBufferOES(GLenum target,
                                                           GLenum access) {
  MakeFunctionUnique("glMapBufferOES");
  return interface_->MapBuffer(target, access);
}

void* GL_BINDING_CALL
MockGLInterface::Mock_glMapBufferRange(GLenum target,
                                       GLintptr offset,
                                       GLsizeiptr length,
                                       GLbitfield access) {
  MakeFunctionUnique("glMapBufferRange");
  return interface_->MapBufferRange(target, offset, length, access);
}

void* GL_BINDING_CALL
MockGLInterface::Mock_glMapBufferRangeEXT(GLenum target,
                                          GLintptr offset,
                                          GLsizeiptr length,
                                          GLbitfield access) {
  MakeFunctionUnique("glMapBufferRangeEXT");
  return interface_->MapBufferRange(target, offset, length, access);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glMatrixLoadIdentityEXT(GLenum matrixMode) {
  MakeFunctionUnique("glMatrixLoadIdentityEXT");
  interface_->MatrixLoadIdentityEXT(matrixMode);
}

void GL_BINDING_CALL MockGLInterface::Mock_glMatrixLoadfEXT(GLenum matrixMode,
                                                            const GLfloat* m) {
  MakeFunctionUnique("glMatrixLoadfEXT");
  interface_->MatrixLoadfEXT(matrixMode, m);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glMemoryBarrier(GLbitfield barriers) {
  MakeFunctionUnique("glMemoryBarrier");
  interface_->MemoryBarrierEXT(barriers);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glMemoryBarrierEXT(GLbitfield barriers) {
  MakeFunctionUnique("glMemoryBarrierEXT");
  interface_->MemoryBarrierEXT(barriers);
}

void GL_BINDING_CALL MockGLInterface::Mock_glObjectLabel(GLenum identifier,
                                                         GLuint name,
                                                         GLsizei length,
                                                         const char* label) {
  MakeFunctionUnique("glObjectLabel");
  interface_->ObjectLabel(identifier, name, length, label);
}

void GL_BINDING_CALL MockGLInterface::Mock_glObjectLabelKHR(GLenum identifier,
                                                            GLuint name,
                                                            GLsizei length,
                                                            const char* label) {
  MakeFunctionUnique("glObjectLabelKHR");
  interface_->ObjectLabel(identifier, name, length, label);
}

void GL_BINDING_CALL MockGLInterface::Mock_glObjectPtrLabel(void* ptr,
                                                            GLsizei length,
                                                            const char* label) {
  MakeFunctionUnique("glObjectPtrLabel");
  interface_->ObjectPtrLabel(ptr, length, label);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glObjectPtrLabelKHR(void* ptr,
                                          GLsizei length,
                                          const char* label) {
  MakeFunctionUnique("glObjectPtrLabelKHR");
  interface_->ObjectPtrLabel(ptr, length, label);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glPathCommandsNV(GLuint path,
                                       GLsizei numCommands,
                                       const GLubyte* commands,
                                       GLsizei numCoords,
                                       GLenum coordType,
                                       const GLvoid* coords) {
  MakeFunctionUnique("glPathCommandsNV");
  interface_->PathCommandsNV(path, numCommands, commands, numCoords, coordType,
                             coords);
}

void GL_BINDING_CALL MockGLInterface::Mock_glPathParameterfNV(GLuint path,
                                                              GLenum pname,
                                                              GLfloat value) {
  MakeFunctionUnique("glPathParameterfNV");
  interface_->PathParameterfNV(path, pname, value);
}

void GL_BINDING_CALL MockGLInterface::Mock_glPathParameteriNV(GLuint path,
                                                              GLenum pname,
                                                              GLint value) {
  MakeFunctionUnique("glPathParameteriNV");
  interface_->PathParameteriNV(path, pname, value);
}

void GL_BINDING_CALL MockGLInterface::Mock_glPathStencilFuncNV(GLenum func,
                                                               GLint ref,
                                                               GLuint mask) {
  MakeFunctionUnique("glPathStencilFuncNV");
  interface_->PathStencilFuncNV(func, ref, mask);
}

void GL_BINDING_CALL MockGLInterface::Mock_glPauseTransformFeedback(void) {
  MakeFunctionUnique("glPauseTransformFeedback");
  interface_->PauseTransformFeedback();
}

void GL_BINDING_CALL MockGLInterface::Mock_glPixelStorei(GLenum pname,
                                                         GLint param) {
  MakeFunctionUnique("glPixelStorei");
  interface_->PixelStorei(pname, param);
}

void GL_BINDING_CALL MockGLInterface::Mock_glPointParameteri(GLenum pname,
                                                             GLint param) {
  MakeFunctionUnique("glPointParameteri");
  interface_->PointParameteri(pname, param);
}

void GL_BINDING_CALL MockGLInterface::Mock_glPolygonMode(GLenum face,
                                                         GLenum mode) {
  MakeFunctionUnique("glPolygonMode");
  interface_->PolygonMode(face, mode);
}

void GL_BINDING_CALL MockGLInterface::Mock_glPolygonOffset(GLfloat factor,
                                                           GLfloat units) {
  MakeFunctionUnique("glPolygonOffset");
  interface_->PolygonOffset(factor, units);
}

void GL_BINDING_CALL MockGLInterface::Mock_glPopDebugGroup() {
  MakeFunctionUnique("glPopDebugGroup");
  interface_->PopDebugGroup();
}

void GL_BINDING_CALL MockGLInterface::Mock_glPopDebugGroupKHR() {
  MakeFunctionUnique("glPopDebugGroupKHR");
  interface_->PopDebugGroup();
}

void GL_BINDING_CALL MockGLInterface::Mock_glPopGroupMarkerEXT(void) {
  MakeFunctionUnique("glPopGroupMarkerEXT");
  interface_->PopGroupMarkerEXT();
}

void GL_BINDING_CALL
MockGLInterface::Mock_glPrimitiveRestartIndex(GLuint index) {
  MakeFunctionUnique("glPrimitiveRestartIndex");
  interface_->PrimitiveRestartIndex(index);
}

void GL_BINDING_CALL MockGLInterface::Mock_glProgramBinary(GLuint program,
                                                           GLenum binaryFormat,
                                                           const GLvoid* binary,
                                                           GLsizei length) {
  MakeFunctionUnique("glProgramBinary");
  interface_->ProgramBinary(program, binaryFormat, binary, length);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glProgramBinaryOES(GLuint program,
                                         GLenum binaryFormat,
                                         const GLvoid* binary,
                                         GLsizei length) {
  MakeFunctionUnique("glProgramBinaryOES");
  interface_->ProgramBinary(program, binaryFormat, binary, length);
}

void GL_BINDING_CALL MockGLInterface::Mock_glProgramParameteri(GLuint program,
                                                               GLenum pname,
                                                               GLint value) {
  MakeFunctionUnique("glProgramParameteri");
  interface_->ProgramParameteri(program, pname, value);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glProgramPathFragmentInputGenNV(GLuint program,
                                                      GLint location,
                                                      GLenum genMode,
                                                      GLint components,
                                                      const GLfloat* coeffs) {
  MakeFunctionUnique("glProgramPathFragmentInputGenNV");
  interface_->ProgramPathFragmentInputGenNV(program, location, genMode,
                                            components, coeffs);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glPushDebugGroup(GLenum source,
                                       GLuint id,
                                       GLsizei length,
                                       const char* message) {
  MakeFunctionUnique("glPushDebugGroup");
  interface_->PushDebugGroup(source, id, length, message);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glPushDebugGroupKHR(GLenum source,
                                          GLuint id,
                                          GLsizei length,
                                          const char* message) {
  MakeFunctionUnique("glPushDebugGroupKHR");
  interface_->PushDebugGroup(source, id, length, message);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glPushGroupMarkerEXT(GLsizei length, const char* marker) {
  MakeFunctionUnique("glPushGroupMarkerEXT");
  interface_->PushGroupMarkerEXT(length, marker);
}

void GL_BINDING_CALL MockGLInterface::Mock_glQueryCounter(GLuint id,
                                                          GLenum target) {
  MakeFunctionUnique("glQueryCounter");
  interface_->QueryCounter(id, target);
}

void GL_BINDING_CALL MockGLInterface::Mock_glQueryCounterEXT(GLuint id,
                                                             GLenum target) {
  MakeFunctionUnique("glQueryCounterEXT");
  interface_->QueryCounter(id, target);
}

void GL_BINDING_CALL MockGLInterface::Mock_glReadBuffer(GLenum src) {
  MakeFunctionUnique("glReadBuffer");
  interface_->ReadBuffer(src);
}

void GL_BINDING_CALL MockGLInterface::Mock_glReadPixels(GLint x,
                                                        GLint y,
                                                        GLsizei width,
                                                        GLsizei height,
                                                        GLenum format,
                                                        GLenum type,
                                                        void* pixels) {
  MakeFunctionUnique("glReadPixels");
  interface_->ReadPixels(x, y, width, height, format, type, pixels);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glReadPixelsRobustANGLE(GLint x,
                                              GLint y,
                                              GLsizei width,
                                              GLsizei height,
                                              GLenum format,
                                              GLenum type,
                                              GLsizei bufSize,
                                              GLsizei* length,
                                              GLsizei* columns,
                                              GLsizei* rows,
                                              void* pixels) {
  MakeFunctionUnique("glReadPixelsRobustANGLE");
  interface_->ReadPixelsRobustANGLE(x, y, width, height, format, type, bufSize,
                                    length, columns, rows, pixels);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glReadnPixelsRobustANGLE(GLint x,
                                               GLint y,
                                               GLsizei width,
                                               GLsizei height,
                                               GLenum format,
                                               GLenum type,
                                               GLsizei bufSize,
                                               GLsizei* length,
                                               GLsizei* columns,
                                               GLsizei* rows,
                                               void* data) {
  MakeFunctionUnique("glReadnPixelsRobustANGLE");
  interface_->ReadnPixelsRobustANGLE(x, y, width, height, format, type, bufSize,
                                     length, columns, rows, data);
}

void GL_BINDING_CALL MockGLInterface::Mock_glReleaseShaderCompiler(void) {
  MakeFunctionUnique("glReleaseShaderCompiler");
  interface_->ReleaseShaderCompiler();
}

void GL_BINDING_CALL
MockGLInterface::Mock_glRenderbufferStorage(GLenum target,
                                            GLenum internalformat,
                                            GLsizei width,
                                            GLsizei height) {
  MakeFunctionUnique("glRenderbufferStorage");
  interface_->RenderbufferStorageEXT(target, internalformat, width, height);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glRenderbufferStorageEXT(GLenum target,
                                               GLenum internalformat,
                                               GLsizei width,
                                               GLsizei height) {
  MakeFunctionUnique("glRenderbufferStorageEXT");
  interface_->RenderbufferStorageEXT(target, internalformat, width, height);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glRenderbufferStorageMultisample(GLenum target,
                                                       GLsizei samples,
                                                       GLenum internalformat,
                                                       GLsizei width,
                                                       GLsizei height) {
  MakeFunctionUnique("glRenderbufferStorageMultisample");
  interface_->RenderbufferStorageMultisample(target, samples, internalformat,
                                             width, height);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glRenderbufferStorageMultisampleANGLE(
    GLenum target,
    GLsizei samples,
    GLenum internalformat,
    GLsizei width,
    GLsizei height) {
  MakeFunctionUnique("glRenderbufferStorageMultisampleANGLE");
  interface_->RenderbufferStorageMultisample(target, samples, internalformat,
                                             width, height);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glRenderbufferStorageMultisampleEXT(GLenum target,
                                                          GLsizei samples,
                                                          GLenum internalformat,
                                                          GLsizei width,
                                                          GLsizei height) {
  MakeFunctionUnique("glRenderbufferStorageMultisampleEXT");
  interface_->RenderbufferStorageMultisampleEXT(target, samples, internalformat,
                                                width, height);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glRenderbufferStorageMultisampleIMG(GLenum target,
                                                          GLsizei samples,
                                                          GLenum internalformat,
                                                          GLsizei width,
                                                          GLsizei height) {
  MakeFunctionUnique("glRenderbufferStorageMultisampleIMG");
  interface_->RenderbufferStorageMultisampleEXT(target, samples, internalformat,
                                                width, height);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glRequestExtensionANGLE(const char* name) {
  MakeFunctionUnique("glRequestExtensionANGLE");
  interface_->RequestExtensionANGLE(name);
}

void GL_BINDING_CALL MockGLInterface::Mock_glResumeTransformFeedback(void) {
  MakeFunctionUnique("glResumeTransformFeedback");
  interface_->ResumeTransformFeedback();
}

void GL_BINDING_CALL MockGLInterface::Mock_glSampleCoverage(GLclampf value,
                                                            GLboolean invert) {
  MakeFunctionUnique("glSampleCoverage");
  interface_->SampleCoverage(value, invert);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glSamplerParameterIivRobustANGLE(GLuint sampler,
                                                       GLenum pname,
                                                       GLsizei bufSize,
                                                       const GLint* param) {
  MakeFunctionUnique("glSamplerParameterIivRobustANGLE");
  interface_->SamplerParameterIivRobustANGLE(sampler, pname, bufSize, param);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glSamplerParameterIuivRobustANGLE(GLuint sampler,
                                                        GLenum pname,
                                                        GLsizei bufSize,
                                                        const GLuint* param) {
  MakeFunctionUnique("glSamplerParameterIuivRobustANGLE");
  interface_->SamplerParameterIuivRobustANGLE(sampler, pname, bufSize, param);
}

void GL_BINDING_CALL MockGLInterface::Mock_glSamplerParameterf(GLuint sampler,
                                                               GLenum pname,
                                                               GLfloat param) {
  MakeFunctionUnique("glSamplerParameterf");
  interface_->SamplerParameterf(sampler, pname, param);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glSamplerParameterfv(GLuint sampler,
                                           GLenum pname,
                                           const GLfloat* params) {
  MakeFunctionUnique("glSamplerParameterfv");
  interface_->SamplerParameterfv(sampler, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glSamplerParameterfvRobustANGLE(GLuint sampler,
                                                      GLenum pname,
                                                      GLsizei bufSize,
                                                      const GLfloat* param) {
  MakeFunctionUnique("glSamplerParameterfvRobustANGLE");
  interface_->SamplerParameterfvRobustANGLE(sampler, pname, bufSize, param);
}

void GL_BINDING_CALL MockGLInterface::Mock_glSamplerParameteri(GLuint sampler,
                                                               GLenum pname,
                                                               GLint param) {
  MakeFunctionUnique("glSamplerParameteri");
  interface_->SamplerParameteri(sampler, pname, param);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glSamplerParameteriv(GLuint sampler,
                                           GLenum pname,
                                           const GLint* params) {
  MakeFunctionUnique("glSamplerParameteriv");
  interface_->SamplerParameteriv(sampler, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glSamplerParameterivRobustANGLE(GLuint sampler,
                                                      GLenum pname,
                                                      GLsizei bufSize,
                                                      const GLint* param) {
  MakeFunctionUnique("glSamplerParameterivRobustANGLE");
  interface_->SamplerParameterivRobustANGLE(sampler, pname, bufSize, param);
}

void GL_BINDING_CALL MockGLInterface::Mock_glScissor(GLint x,
                                                     GLint y,
                                                     GLsizei width,
                                                     GLsizei height) {
  MakeFunctionUnique("glScissor");
  interface_->Scissor(x, y, width, height);
}

void GL_BINDING_CALL MockGLInterface::Mock_glSetFenceAPPLE(GLuint fence) {
  MakeFunctionUnique("glSetFenceAPPLE");
  interface_->SetFenceAPPLE(fence);
}

void GL_BINDING_CALL MockGLInterface::Mock_glSetFenceNV(GLuint fence,
                                                        GLenum condition) {
  MakeFunctionUnique("glSetFenceNV");
  interface_->SetFenceNV(fence, condition);
}

void GL_BINDING_CALL MockGLInterface::Mock_glShaderBinary(GLsizei n,
                                                          const GLuint* shaders,
                                                          GLenum binaryformat,
                                                          const void* binary,
                                                          GLsizei length) {
  MakeFunctionUnique("glShaderBinary");
  interface_->ShaderBinary(n, shaders, binaryformat, binary, length);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glShaderSource(GLuint shader,
                                     GLsizei count,
                                     const char* const* str,
                                     const GLint* length) {
  MakeFunctionUnique("glShaderSource");
  interface_->ShaderSource(shader, count, str, length);
}

void GL_BINDING_CALL MockGLInterface::Mock_glStencilFillPathInstancedNV(
    GLsizei numPaths,
    GLenum pathNameType,
    const void* paths,
    GLuint pathBase,
    GLenum fillMode,
    GLuint mask,
    GLenum transformType,
    const GLfloat* transformValues) {
  MakeFunctionUnique("glStencilFillPathInstancedNV");
  interface_->StencilFillPathInstancedNV(numPaths, pathNameType, paths,
                                         pathBase, fillMode, mask,
                                         transformType, transformValues);
}

void GL_BINDING_CALL MockGLInterface::Mock_glStencilFillPathNV(GLuint path,
                                                               GLenum fillMode,
                                                               GLuint mask) {
  MakeFunctionUnique("glStencilFillPathNV");
  interface_->StencilFillPathNV(path, fillMode, mask);
}

void GL_BINDING_CALL MockGLInterface::Mock_glStencilFunc(GLenum func,
                                                         GLint ref,
                                                         GLuint mask) {
  MakeFunctionUnique("glStencilFunc");
  interface_->StencilFunc(func, ref, mask);
}

void GL_BINDING_CALL MockGLInterface::Mock_glStencilFuncSeparate(GLenum face,
                                                                 GLenum func,
                                                                 GLint ref,
                                                                 GLuint mask) {
  MakeFunctionUnique("glStencilFuncSeparate");
  interface_->StencilFuncSeparate(face, func, ref, mask);
}

void GL_BINDING_CALL MockGLInterface::Mock_glStencilMask(GLuint mask) {
  MakeFunctionUnique("glStencilMask");
  interface_->StencilMask(mask);
}

void GL_BINDING_CALL MockGLInterface::Mock_glStencilMaskSeparate(GLenum face,
                                                                 GLuint mask) {
  MakeFunctionUnique("glStencilMaskSeparate");
  interface_->StencilMaskSeparate(face, mask);
}

void GL_BINDING_CALL MockGLInterface::Mock_glStencilOp(GLenum fail,
                                                       GLenum zfail,
                                                       GLenum zpass) {
  MakeFunctionUnique("glStencilOp");
  interface_->StencilOp(fail, zfail, zpass);
}

void GL_BINDING_CALL MockGLInterface::Mock_glStencilOpSeparate(GLenum face,
                                                               GLenum fail,
                                                               GLenum zfail,
                                                               GLenum zpass) {
  MakeFunctionUnique("glStencilOpSeparate");
  interface_->StencilOpSeparate(face, fail, zfail, zpass);
}

void GL_BINDING_CALL MockGLInterface::Mock_glStencilStrokePathInstancedNV(
    GLsizei numPaths,
    GLenum pathNameType,
    const void* paths,
    GLuint pathBase,
    GLint ref,
    GLuint mask,
    GLenum transformType,
    const GLfloat* transformValues) {
  MakeFunctionUnique("glStencilStrokePathInstancedNV");
  interface_->StencilStrokePathInstancedNV(numPaths, pathNameType, paths,
                                           pathBase, ref, mask, transformType,
                                           transformValues);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glStencilStrokePathNV(GLuint path,
                                            GLint reference,
                                            GLuint mask) {
  MakeFunctionUnique("glStencilStrokePathNV");
  interface_->StencilStrokePathNV(path, reference, mask);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glStencilThenCoverFillPathInstancedNV(
    GLsizei numPaths,
    GLenum pathNameType,
    const void* paths,
    GLuint pathBase,
    GLenum fillMode,
    GLuint mask,
    GLenum coverMode,
    GLenum transformType,
    const GLfloat* transformValues) {
  MakeFunctionUnique("glStencilThenCoverFillPathInstancedNV");
  interface_->StencilThenCoverFillPathInstancedNV(
      numPaths, pathNameType, paths, pathBase, fillMode, mask, coverMode,
      transformType, transformValues);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glStencilThenCoverFillPathNV(GLuint path,
                                                   GLenum fillMode,
                                                   GLuint mask,
                                                   GLenum coverMode) {
  MakeFunctionUnique("glStencilThenCoverFillPathNV");
  interface_->StencilThenCoverFillPathNV(path, fillMode, mask, coverMode);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glStencilThenCoverStrokePathInstancedNV(
    GLsizei numPaths,
    GLenum pathNameType,
    const void* paths,
    GLuint pathBase,
    GLint ref,
    GLuint mask,
    GLenum coverMode,
    GLenum transformType,
    const GLfloat* transformValues) {
  MakeFunctionUnique("glStencilThenCoverStrokePathInstancedNV");
  interface_->StencilThenCoverStrokePathInstancedNV(
      numPaths, pathNameType, paths, pathBase, ref, mask, coverMode,
      transformType, transformValues);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glStencilThenCoverStrokePathNV(GLuint path,
                                                     GLint reference,
                                                     GLuint mask,
                                                     GLenum coverMode) {
  MakeFunctionUnique("glStencilThenCoverStrokePathNV");
  interface_->StencilThenCoverStrokePathNV(path, reference, mask, coverMode);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glTestFenceAPPLE(GLuint fence) {
  MakeFunctionUnique("glTestFenceAPPLE");
  return interface_->TestFenceAPPLE(fence);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glTestFenceNV(GLuint fence) {
  MakeFunctionUnique("glTestFenceNV");
  return interface_->TestFenceNV(fence);
}

void GL_BINDING_CALL MockGLInterface::Mock_glTexBuffer(GLenum target,
                                                       GLenum internalformat,
                                                       GLuint buffer) {
  MakeFunctionUnique("glTexBuffer");
  interface_->TexBuffer(target, internalformat, buffer);
}

void GL_BINDING_CALL MockGLInterface::Mock_glTexBufferEXT(GLenum target,
                                                          GLenum internalformat,
                                                          GLuint buffer) {
  MakeFunctionUnique("glTexBufferEXT");
  interface_->TexBuffer(target, internalformat, buffer);
}

void GL_BINDING_CALL MockGLInterface::Mock_glTexBufferOES(GLenum target,
                                                          GLenum internalformat,
                                                          GLuint buffer) {
  MakeFunctionUnique("glTexBufferOES");
  interface_->TexBuffer(target, internalformat, buffer);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexBufferRange(GLenum target,
                                       GLenum internalformat,
                                       GLuint buffer,
                                       GLintptr offset,
                                       GLsizeiptr size) {
  MakeFunctionUnique("glTexBufferRange");
  interface_->TexBufferRange(target, internalformat, buffer, offset, size);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexBufferRangeEXT(GLenum target,
                                          GLenum internalformat,
                                          GLuint buffer,
                                          GLintptr offset,
                                          GLsizeiptr size) {
  MakeFunctionUnique("glTexBufferRangeEXT");
  interface_->TexBufferRange(target, internalformat, buffer, offset, size);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexBufferRangeOES(GLenum target,
                                          GLenum internalformat,
                                          GLuint buffer,
                                          GLintptr offset,
                                          GLsizeiptr size) {
  MakeFunctionUnique("glTexBufferRangeOES");
  interface_->TexBufferRange(target, internalformat, buffer, offset, size);
}

void GL_BINDING_CALL MockGLInterface::Mock_glTexImage2D(GLenum target,
                                                        GLint level,
                                                        GLint internalformat,
                                                        GLsizei width,
                                                        GLsizei height,
                                                        GLint border,
                                                        GLenum format,
                                                        GLenum type,
                                                        const void* pixels) {
  MakeFunctionUnique("glTexImage2D");
  interface_->TexImage2D(target, level, internalformat, width, height, border,
                         format, type, pixels);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexImage2DRobustANGLE(GLenum target,
                                              GLint level,
                                              GLint internalformat,
                                              GLsizei width,
                                              GLsizei height,
                                              GLint border,
                                              GLenum format,
                                              GLenum type,
                                              GLsizei bufSize,
                                              const void* pixels) {
  MakeFunctionUnique("glTexImage2DRobustANGLE");
  interface_->TexImage2DRobustANGLE(target, level, internalformat, width,
                                    height, border, format, type, bufSize,
                                    pixels);
}

void GL_BINDING_CALL MockGLInterface::Mock_glTexImage3D(GLenum target,
                                                        GLint level,
                                                        GLint internalformat,
                                                        GLsizei width,
                                                        GLsizei height,
                                                        GLsizei depth,
                                                        GLint border,
                                                        GLenum format,
                                                        GLenum type,
                                                        const void* pixels) {
  MakeFunctionUnique("glTexImage3D");
  interface_->TexImage3D(target, level, internalformat, width, height, depth,
                         border, format, type, pixels);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexImage3DRobustANGLE(GLenum target,
                                              GLint level,
                                              GLint internalformat,
                                              GLsizei width,
                                              GLsizei height,
                                              GLsizei depth,
                                              GLint border,
                                              GLenum format,
                                              GLenum type,
                                              GLsizei bufSize,
                                              const void* pixels) {
  MakeFunctionUnique("glTexImage3DRobustANGLE");
  interface_->TexImage3DRobustANGLE(target, level, internalformat, width,
                                    height, depth, border, format, type,
                                    bufSize, pixels);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexParameterIivRobustANGLE(GLenum target,
                                                   GLenum pname,
                                                   GLsizei bufSize,
                                                   const GLint* params) {
  MakeFunctionUnique("glTexParameterIivRobustANGLE");
  interface_->TexParameterIivRobustANGLE(target, pname, bufSize, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexParameterIuivRobustANGLE(GLenum target,
                                                    GLenum pname,
                                                    GLsizei bufSize,
                                                    const GLuint* params) {
  MakeFunctionUnique("glTexParameterIuivRobustANGLE");
  interface_->TexParameterIuivRobustANGLE(target, pname, bufSize, params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glTexParameterf(GLenum target,
                                                           GLenum pname,
                                                           GLfloat param) {
  MakeFunctionUnique("glTexParameterf");
  interface_->TexParameterf(target, pname, param);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexParameterfv(GLenum target,
                                       GLenum pname,
                                       const GLfloat* params) {
  MakeFunctionUnique("glTexParameterfv");
  interface_->TexParameterfv(target, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexParameterfvRobustANGLE(GLenum target,
                                                  GLenum pname,
                                                  GLsizei bufSize,
                                                  const GLfloat* params) {
  MakeFunctionUnique("glTexParameterfvRobustANGLE");
  interface_->TexParameterfvRobustANGLE(target, pname, bufSize, params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glTexParameteri(GLenum target,
                                                           GLenum pname,
                                                           GLint param) {
  MakeFunctionUnique("glTexParameteri");
  interface_->TexParameteri(target, pname, param);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexParameteriv(GLenum target,
                                       GLenum pname,
                                       const GLint* params) {
  MakeFunctionUnique("glTexParameteriv");
  interface_->TexParameteriv(target, pname, params);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexParameterivRobustANGLE(GLenum target,
                                                  GLenum pname,
                                                  GLsizei bufSize,
                                                  const GLint* params) {
  MakeFunctionUnique("glTexParameterivRobustANGLE");
  interface_->TexParameterivRobustANGLE(target, pname, bufSize, params);
}

void GL_BINDING_CALL MockGLInterface::Mock_glTexStorage2D(GLenum target,
                                                          GLsizei levels,
                                                          GLenum internalformat,
                                                          GLsizei width,
                                                          GLsizei height) {
  MakeFunctionUnique("glTexStorage2D");
  interface_->TexStorage2DEXT(target, levels, internalformat, width, height);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexStorage2DEXT(GLenum target,
                                        GLsizei levels,
                                        GLenum internalformat,
                                        GLsizei width,
                                        GLsizei height) {
  MakeFunctionUnique("glTexStorage2DEXT");
  interface_->TexStorage2DEXT(target, levels, internalformat, width, height);
}

void GL_BINDING_CALL MockGLInterface::Mock_glTexStorage3D(GLenum target,
                                                          GLsizei levels,
                                                          GLenum internalformat,
                                                          GLsizei width,
                                                          GLsizei height,
                                                          GLsizei depth) {
  MakeFunctionUnique("glTexStorage3D");
  interface_->TexStorage3D(target, levels, internalformat, width, height,
                           depth);
}

void GL_BINDING_CALL MockGLInterface::Mock_glTexSubImage2D(GLenum target,
                                                           GLint level,
                                                           GLint xoffset,
                                                           GLint yoffset,
                                                           GLsizei width,
                                                           GLsizei height,
                                                           GLenum format,
                                                           GLenum type,
                                                           const void* pixels) {
  MakeFunctionUnique("glTexSubImage2D");
  interface_->TexSubImage2D(target, level, xoffset, yoffset, width, height,
                            format, type, pixels);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexSubImage2DRobustANGLE(GLenum target,
                                                 GLint level,
                                                 GLint xoffset,
                                                 GLint yoffset,
                                                 GLsizei width,
                                                 GLsizei height,
                                                 GLenum format,
                                                 GLenum type,
                                                 GLsizei bufSize,
                                                 const void* pixels) {
  MakeFunctionUnique("glTexSubImage2DRobustANGLE");
  interface_->TexSubImage2DRobustANGLE(target, level, xoffset, yoffset, width,
                                       height, format, type, bufSize, pixels);
}

void GL_BINDING_CALL MockGLInterface::Mock_glTexSubImage3D(GLenum target,
                                                           GLint level,
                                                           GLint xoffset,
                                                           GLint yoffset,
                                                           GLint zoffset,
                                                           GLsizei width,
                                                           GLsizei height,
                                                           GLsizei depth,
                                                           GLenum format,
                                                           GLenum type,
                                                           const void* pixels) {
  MakeFunctionUnique("glTexSubImage3D");
  interface_->TexSubImage3D(target, level, xoffset, yoffset, zoffset, width,
                            height, depth, format, type, pixels);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTexSubImage3DRobustANGLE(GLenum target,
                                                 GLint level,
                                                 GLint xoffset,
                                                 GLint yoffset,
                                                 GLint zoffset,
                                                 GLsizei width,
                                                 GLsizei height,
                                                 GLsizei depth,
                                                 GLenum format,
                                                 GLenum type,
                                                 GLsizei bufSize,
                                                 const void* pixels) {
  MakeFunctionUnique("glTexSubImage3DRobustANGLE");
  interface_->TexSubImage3DRobustANGLE(target, level, xoffset, yoffset, zoffset,
                                       width, height, depth, format, type,
                                       bufSize, pixels);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glTransformFeedbackVaryings(GLuint program,
                                                  GLsizei count,
                                                  const char* const* varyings,
                                                  GLenum bufferMode) {
  MakeFunctionUnique("glTransformFeedbackVaryings");
  interface_->TransformFeedbackVaryings(program, count, varyings, bufferMode);
}

void GL_BINDING_CALL MockGLInterface::Mock_glTransformFeedbackVaryingsEXT(
    GLuint program,
    GLsizei count,
    const char* const* varyings,
    GLenum bufferMode) {
  MakeFunctionUnique("glTransformFeedbackVaryingsEXT");
  interface_->TransformFeedbackVaryings(program, count, varyings, bufferMode);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform1f(GLint location,
                                                       GLfloat x) {
  MakeFunctionUnique("glUniform1f");
  interface_->Uniform1f(location, x);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform1fv(GLint location,
                                                        GLsizei count,
                                                        const GLfloat* v) {
  MakeFunctionUnique("glUniform1fv");
  interface_->Uniform1fv(location, count, v);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform1i(GLint location,
                                                       GLint x) {
  MakeFunctionUnique("glUniform1i");
  interface_->Uniform1i(location, x);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform1iv(GLint location,
                                                        GLsizei count,
                                                        const GLint* v) {
  MakeFunctionUnique("glUniform1iv");
  interface_->Uniform1iv(location, count, v);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform1ui(GLint location,
                                                        GLuint v0) {
  MakeFunctionUnique("glUniform1ui");
  interface_->Uniform1ui(location, v0);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform1uiv(GLint location,
                                                         GLsizei count,
                                                         const GLuint* v) {
  MakeFunctionUnique("glUniform1uiv");
  interface_->Uniform1uiv(location, count, v);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform2f(GLint location,
                                                       GLfloat x,
                                                       GLfloat y) {
  MakeFunctionUnique("glUniform2f");
  interface_->Uniform2f(location, x, y);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform2fv(GLint location,
                                                        GLsizei count,
                                                        const GLfloat* v) {
  MakeFunctionUnique("glUniform2fv");
  interface_->Uniform2fv(location, count, v);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform2i(GLint location,
                                                       GLint x,
                                                       GLint y) {
  MakeFunctionUnique("glUniform2i");
  interface_->Uniform2i(location, x, y);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform2iv(GLint location,
                                                        GLsizei count,
                                                        const GLint* v) {
  MakeFunctionUnique("glUniform2iv");
  interface_->Uniform2iv(location, count, v);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform2ui(GLint location,
                                                        GLuint v0,
                                                        GLuint v1) {
  MakeFunctionUnique("glUniform2ui");
  interface_->Uniform2ui(location, v0, v1);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform2uiv(GLint location,
                                                         GLsizei count,
                                                         const GLuint* v) {
  MakeFunctionUnique("glUniform2uiv");
  interface_->Uniform2uiv(location, count, v);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform3f(GLint location,
                                                       GLfloat x,
                                                       GLfloat y,
                                                       GLfloat z) {
  MakeFunctionUnique("glUniform3f");
  interface_->Uniform3f(location, x, y, z);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform3fv(GLint location,
                                                        GLsizei count,
                                                        const GLfloat* v) {
  MakeFunctionUnique("glUniform3fv");
  interface_->Uniform3fv(location, count, v);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform3i(GLint location,
                                                       GLint x,
                                                       GLint y,
                                                       GLint z) {
  MakeFunctionUnique("glUniform3i");
  interface_->Uniform3i(location, x, y, z);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform3iv(GLint location,
                                                        GLsizei count,
                                                        const GLint* v) {
  MakeFunctionUnique("glUniform3iv");
  interface_->Uniform3iv(location, count, v);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform3ui(GLint location,
                                                        GLuint v0,
                                                        GLuint v1,
                                                        GLuint v2) {
  MakeFunctionUnique("glUniform3ui");
  interface_->Uniform3ui(location, v0, v1, v2);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform3uiv(GLint location,
                                                         GLsizei count,
                                                         const GLuint* v) {
  MakeFunctionUnique("glUniform3uiv");
  interface_->Uniform3uiv(location, count, v);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform4f(GLint location,
                                                       GLfloat x,
                                                       GLfloat y,
                                                       GLfloat z,
                                                       GLfloat w) {
  MakeFunctionUnique("glUniform4f");
  interface_->Uniform4f(location, x, y, z, w);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform4fv(GLint location,
                                                        GLsizei count,
                                                        const GLfloat* v) {
  MakeFunctionUnique("glUniform4fv");
  interface_->Uniform4fv(location, count, v);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform4i(GLint location,
                                                       GLint x,
                                                       GLint y,
                                                       GLint z,
                                                       GLint w) {
  MakeFunctionUnique("glUniform4i");
  interface_->Uniform4i(location, x, y, z, w);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform4iv(GLint location,
                                                        GLsizei count,
                                                        const GLint* v) {
  MakeFunctionUnique("glUniform4iv");
  interface_->Uniform4iv(location, count, v);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform4ui(GLint location,
                                                        GLuint v0,
                                                        GLuint v1,
                                                        GLuint v2,
                                                        GLuint v3) {
  MakeFunctionUnique("glUniform4ui");
  interface_->Uniform4ui(location, v0, v1, v2, v3);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUniform4uiv(GLint location,
                                                         GLsizei count,
                                                         const GLuint* v) {
  MakeFunctionUnique("glUniform4uiv");
  interface_->Uniform4uiv(location, count, v);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glUniformBlockBinding(GLuint program,
                                            GLuint uniformBlockIndex,
                                            GLuint uniformBlockBinding) {
  MakeFunctionUnique("glUniformBlockBinding");
  interface_->UniformBlockBinding(program, uniformBlockIndex,
                                  uniformBlockBinding);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glUniformMatrix2fv(GLint location,
                                         GLsizei count,
                                         GLboolean transpose,
                                         const GLfloat* value) {
  MakeFunctionUnique("glUniformMatrix2fv");
  interface_->UniformMatrix2fv(location, count, transpose, value);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glUniformMatrix2x3fv(GLint location,
                                           GLsizei count,
                                           GLboolean transpose,
                                           const GLfloat* value) {
  MakeFunctionUnique("glUniformMatrix2x3fv");
  interface_->UniformMatrix2x3fv(location, count, transpose, value);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glUniformMatrix2x4fv(GLint location,
                                           GLsizei count,
                                           GLboolean transpose,
                                           const GLfloat* value) {
  MakeFunctionUnique("glUniformMatrix2x4fv");
  interface_->UniformMatrix2x4fv(location, count, transpose, value);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glUniformMatrix3fv(GLint location,
                                         GLsizei count,
                                         GLboolean transpose,
                                         const GLfloat* value) {
  MakeFunctionUnique("glUniformMatrix3fv");
  interface_->UniformMatrix3fv(location, count, transpose, value);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glUniformMatrix3x2fv(GLint location,
                                           GLsizei count,
                                           GLboolean transpose,
                                           const GLfloat* value) {
  MakeFunctionUnique("glUniformMatrix3x2fv");
  interface_->UniformMatrix3x2fv(location, count, transpose, value);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glUniformMatrix3x4fv(GLint location,
                                           GLsizei count,
                                           GLboolean transpose,
                                           const GLfloat* value) {
  MakeFunctionUnique("glUniformMatrix3x4fv");
  interface_->UniformMatrix3x4fv(location, count, transpose, value);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glUniformMatrix4fv(GLint location,
                                         GLsizei count,
                                         GLboolean transpose,
                                         const GLfloat* value) {
  MakeFunctionUnique("glUniformMatrix4fv");
  interface_->UniformMatrix4fv(location, count, transpose, value);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glUniformMatrix4x2fv(GLint location,
                                           GLsizei count,
                                           GLboolean transpose,
                                           const GLfloat* value) {
  MakeFunctionUnique("glUniformMatrix4x2fv");
  interface_->UniformMatrix4x2fv(location, count, transpose, value);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glUniformMatrix4x3fv(GLint location,
                                           GLsizei count,
                                           GLboolean transpose,
                                           const GLfloat* value) {
  MakeFunctionUnique("glUniformMatrix4x3fv");
  interface_->UniformMatrix4x3fv(location, count, transpose, value);
}

GLboolean GL_BINDING_CALL MockGLInterface::Mock_glUnmapBuffer(GLenum target) {
  MakeFunctionUnique("glUnmapBuffer");
  return interface_->UnmapBuffer(target);
}

GLboolean GL_BINDING_CALL
MockGLInterface::Mock_glUnmapBufferOES(GLenum target) {
  MakeFunctionUnique("glUnmapBufferOES");
  return interface_->UnmapBuffer(target);
}

void GL_BINDING_CALL MockGLInterface::Mock_glUseProgram(GLuint program) {
  MakeFunctionUnique("glUseProgram");
  interface_->UseProgram(program);
}

void GL_BINDING_CALL MockGLInterface::Mock_glValidateProgram(GLuint program) {
  MakeFunctionUnique("glValidateProgram");
  interface_->ValidateProgram(program);
}

void GL_BINDING_CALL MockGLInterface::Mock_glVertexAttrib1f(GLuint indx,
                                                            GLfloat x) {
  MakeFunctionUnique("glVertexAttrib1f");
  interface_->VertexAttrib1f(indx, x);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glVertexAttrib1fv(GLuint indx, const GLfloat* values) {
  MakeFunctionUnique("glVertexAttrib1fv");
  interface_->VertexAttrib1fv(indx, values);
}

void GL_BINDING_CALL MockGLInterface::Mock_glVertexAttrib2f(GLuint indx,
                                                            GLfloat x,
                                                            GLfloat y) {
  MakeFunctionUnique("glVertexAttrib2f");
  interface_->VertexAttrib2f(indx, x, y);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glVertexAttrib2fv(GLuint indx, const GLfloat* values) {
  MakeFunctionUnique("glVertexAttrib2fv");
  interface_->VertexAttrib2fv(indx, values);
}

void GL_BINDING_CALL MockGLInterface::Mock_glVertexAttrib3f(GLuint indx,
                                                            GLfloat x,
                                                            GLfloat y,
                                                            GLfloat z) {
  MakeFunctionUnique("glVertexAttrib3f");
  interface_->VertexAttrib3f(indx, x, y, z);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glVertexAttrib3fv(GLuint indx, const GLfloat* values) {
  MakeFunctionUnique("glVertexAttrib3fv");
  interface_->VertexAttrib3fv(indx, values);
}

void GL_BINDING_CALL MockGLInterface::Mock_glVertexAttrib4f(GLuint indx,
                                                            GLfloat x,
                                                            GLfloat y,
                                                            GLfloat z,
                                                            GLfloat w) {
  MakeFunctionUnique("glVertexAttrib4f");
  interface_->VertexAttrib4f(indx, x, y, z, w);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glVertexAttrib4fv(GLuint indx, const GLfloat* values) {
  MakeFunctionUnique("glVertexAttrib4fv");
  interface_->VertexAttrib4fv(indx, values);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glVertexAttribDivisor(GLuint index, GLuint divisor) {
  MakeFunctionUnique("glVertexAttribDivisor");
  interface_->VertexAttribDivisorANGLE(index, divisor);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glVertexAttribDivisorANGLE(GLuint index, GLuint divisor) {
  MakeFunctionUnique("glVertexAttribDivisorANGLE");
  interface_->VertexAttribDivisorANGLE(index, divisor);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glVertexAttribDivisorARB(GLuint index, GLuint divisor) {
  MakeFunctionUnique("glVertexAttribDivisorARB");
  interface_->VertexAttribDivisorANGLE(index, divisor);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glVertexAttribDivisorEXT(GLuint index, GLuint divisor) {
  MakeFunctionUnique("glVertexAttribDivisorEXT");
  interface_->VertexAttribDivisorANGLE(index, divisor);
}

void GL_BINDING_CALL MockGLInterface::Mock_glVertexAttribI4i(GLuint indx,
                                                             GLint x,
                                                             GLint y,
                                                             GLint z,
                                                             GLint w) {
  MakeFunctionUnique("glVertexAttribI4i");
  interface_->VertexAttribI4i(indx, x, y, z, w);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glVertexAttribI4iv(GLuint indx, const GLint* values) {
  MakeFunctionUnique("glVertexAttribI4iv");
  interface_->VertexAttribI4iv(indx, values);
}

void GL_BINDING_CALL MockGLInterface::Mock_glVertexAttribI4ui(GLuint indx,
                                                              GLuint x,
                                                              GLuint y,
                                                              GLuint z,
                                                              GLuint w) {
  MakeFunctionUnique("glVertexAttribI4ui");
  interface_->VertexAttribI4ui(indx, x, y, z, w);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glVertexAttribI4uiv(GLuint indx, const GLuint* values) {
  MakeFunctionUnique("glVertexAttribI4uiv");
  interface_->VertexAttribI4uiv(indx, values);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glVertexAttribIPointer(GLuint indx,
                                             GLint size,
                                             GLenum type,
                                             GLsizei stride,
                                             const void* ptr) {
  MakeFunctionUnique("glVertexAttribIPointer");
  interface_->VertexAttribIPointer(indx, size, type, stride, ptr);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glVertexAttribPointer(GLuint indx,
                                            GLint size,
                                            GLenum type,
                                            GLboolean normalized,
                                            GLsizei stride,
                                            const void* ptr) {
  MakeFunctionUnique("glVertexAttribPointer");
  interface_->VertexAttribPointer(indx, size, type, normalized, stride, ptr);
}

void GL_BINDING_CALL MockGLInterface::Mock_glViewport(GLint x,
                                                      GLint y,
                                                      GLsizei width,
                                                      GLsizei height) {
  MakeFunctionUnique("glViewport");
  interface_->Viewport(x, y, width, height);
}

void GL_BINDING_CALL MockGLInterface::Mock_glWaitSync(GLsync sync,
                                                      GLbitfield flags,
                                                      GLuint64 timeout) {
  MakeFunctionUnique("glWaitSync");
  interface_->WaitSync(sync, flags, timeout);
}

void GL_BINDING_CALL
MockGLInterface::Mock_glWindowRectanglesEXT(GLenum mode,
                                            GLsizei n,
                                            const GLint* box) {
  MakeFunctionUnique("glWindowRectanglesEXT");
  interface_->WindowRectanglesEXT(mode, n, box);
}

static void MockInvalidFunction() {
  NOTREACHED();
}

GLFunctionPointerType GL_BINDING_CALL
MockGLInterface::GetGLProcAddress(const char* name) {
  if (strcmp(name, "glActiveTexture") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glActiveTexture);
  if (strcmp(name, "glApplyFramebufferAttachmentCMAAINTEL") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glApplyFramebufferAttachmentCMAAINTEL);
  if (strcmp(name, "glAttachShader") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glAttachShader);
  if (strcmp(name, "glBeginQuery") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBeginQuery);
  if (strcmp(name, "glBeginQueryARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBeginQueryARB);
  if (strcmp(name, "glBeginQueryEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBeginQueryEXT);
  if (strcmp(name, "glBeginTransformFeedback") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glBeginTransformFeedback);
  if (strcmp(name, "glBeginTransformFeedbackEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glBeginTransformFeedbackEXT);
  if (strcmp(name, "glBindAttribLocation") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindAttribLocation);
  if (strcmp(name, "glBindBuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindBuffer);
  if (strcmp(name, "glBindBufferBase") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindBufferBase);
  if (strcmp(name, "glBindBufferBaseEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindBufferBaseEXT);
  if (strcmp(name, "glBindBufferRange") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindBufferRange);
  if (strcmp(name, "glBindBufferRangeEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindBufferRangeEXT);
  if (strcmp(name, "glBindFragDataLocation") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindFragDataLocation);
  if (strcmp(name, "glBindFragDataLocationEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glBindFragDataLocationEXT);
  if (strcmp(name, "glBindFragDataLocationIndexed") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glBindFragDataLocationIndexed);
  if (strcmp(name, "glBindFragDataLocationIndexedEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glBindFragDataLocationIndexedEXT);
  if (strcmp(name, "glBindFramebuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindFramebuffer);
  if (strcmp(name, "glBindFramebufferEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindFramebufferEXT);
  if (strcmp(name, "glBindImageTexture") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindImageTexture);
  if (strcmp(name, "glBindImageTextureEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindImageTextureEXT);
  if (strcmp(name, "glBindRenderbuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindRenderbuffer);
  if (strcmp(name, "glBindRenderbufferEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindRenderbufferEXT);
  if (strcmp(name, "glBindSampler") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindSampler);
  if (strcmp(name, "glBindTexture") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindTexture);
  if (strcmp(name, "glBindTransformFeedback") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glBindTransformFeedback);
  if (strcmp(name, "glBindUniformLocationCHROMIUM") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glBindUniformLocationCHROMIUM);
  if (strcmp(name, "glBindVertexArray") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindVertexArray);
  if (strcmp(name, "glBindVertexArrayAPPLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindVertexArrayAPPLE);
  if (strcmp(name, "glBindVertexArrayOES") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBindVertexArrayOES);
  if (strcmp(name, "glBlendBarrierKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBlendBarrierKHR);
  if (strcmp(name, "glBlendBarrierNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBlendBarrierNV);
  if (strcmp(name, "glBlendColor") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBlendColor);
  if (strcmp(name, "glBlendEquation") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBlendEquation);
  if (strcmp(name, "glBlendEquationSeparate") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glBlendEquationSeparate);
  if (strcmp(name, "glBlendFunc") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBlendFunc);
  if (strcmp(name, "glBlendFuncSeparate") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBlendFuncSeparate);
  if (strcmp(name, "glBlitFramebuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBlitFramebuffer);
  if (strcmp(name, "glBlitFramebufferANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBlitFramebufferANGLE);
  if (strcmp(name, "glBlitFramebufferEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBlitFramebufferEXT);
  if (strcmp(name, "glBufferData") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBufferData);
  if (strcmp(name, "glBufferSubData") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glBufferSubData);
  if (strcmp(name, "glCheckFramebufferStatus") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glCheckFramebufferStatus);
  if (strcmp(name, "glCheckFramebufferStatusEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glCheckFramebufferStatusEXT);
  if (strcmp(name, "glClear") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glClear);
  if (strcmp(name, "glClearBufferfi") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glClearBufferfi);
  if (strcmp(name, "glClearBufferfv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glClearBufferfv);
  if (strcmp(name, "glClearBufferiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glClearBufferiv);
  if (strcmp(name, "glClearBufferuiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glClearBufferuiv);
  if (strcmp(name, "glClearColor") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glClearColor);
  if (strcmp(name, "glClearDepth") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glClearDepth);
  if (strcmp(name, "glClearDepthf") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glClearDepthf);
  if (strcmp(name, "glClearStencil") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glClearStencil);
  if (strcmp(name, "glClientWaitSync") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glClientWaitSync);
  if (strcmp(name, "glColorMask") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glColorMask);
  if (strcmp(name, "glCompileShader") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCompileShader);
  if (strcmp(name, "glCompressedCopyTextureCHROMIUM") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glCompressedCopyTextureCHROMIUM);
  if (strcmp(name, "glCompressedTexImage2D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCompressedTexImage2D);
  if (strcmp(name, "glCompressedTexImage2DRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glCompressedTexImage2DRobustANGLE);
  if (strcmp(name, "glCompressedTexImage3D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCompressedTexImage3D);
  if (strcmp(name, "glCompressedTexImage3DRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glCompressedTexImage3DRobustANGLE);
  if (strcmp(name, "glCompressedTexSubImage2D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glCompressedTexSubImage2D);
  if (strcmp(name, "glCompressedTexSubImage2DRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glCompressedTexSubImage2DRobustANGLE);
  if (strcmp(name, "glCompressedTexSubImage3D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glCompressedTexSubImage3D);
  if (strcmp(name, "glCompressedTexSubImage3DRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glCompressedTexSubImage3DRobustANGLE);
  if (strcmp(name, "glCopyBufferSubData") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCopyBufferSubData);
  if (strcmp(name, "glCopySubTextureCHROMIUM") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glCopySubTextureCHROMIUM);
  if (strcmp(name, "glCopyTexImage2D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCopyTexImage2D);
  if (strcmp(name, "glCopyTexSubImage2D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCopyTexSubImage2D);
  if (strcmp(name, "glCopyTexSubImage3D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCopyTexSubImage3D);
  if (strcmp(name, "glCopyTextureCHROMIUM") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCopyTextureCHROMIUM);
  if (strcmp(name, "glCoverFillPathInstancedNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glCoverFillPathInstancedNV);
  if (strcmp(name, "glCoverFillPathNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCoverFillPathNV);
  if (strcmp(name, "glCoverStrokePathInstancedNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glCoverStrokePathInstancedNV);
  if (strcmp(name, "glCoverStrokePathNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCoverStrokePathNV);
  if (strcmp(name, "glCoverageModulationNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCoverageModulationNV);
  if (strcmp(name, "glCreateProgram") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCreateProgram);
  if (strcmp(name, "glCreateShader") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCreateShader);
  if (strcmp(name, "glCullFace") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glCullFace);
  if (strcmp(name, "glDebugMessageCallback") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDebugMessageCallback);
  if (strcmp(name, "glDebugMessageCallbackKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDebugMessageCallbackKHR);
  if (strcmp(name, "glDebugMessageControl") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDebugMessageControl);
  if (strcmp(name, "glDebugMessageControlKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDebugMessageControlKHR);
  if (strcmp(name, "glDebugMessageInsert") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDebugMessageInsert);
  if (strcmp(name, "glDebugMessageInsertKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDebugMessageInsertKHR);
  if (strcmp(name, "glDeleteBuffers") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteBuffers);
  if (strcmp(name, "glDeleteFencesAPPLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteFencesAPPLE);
  if (strcmp(name, "glDeleteFencesNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteFencesNV);
  if (strcmp(name, "glDeleteFramebuffers") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteFramebuffers);
  if (strcmp(name, "glDeleteFramebuffersEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDeleteFramebuffersEXT);
  if (strcmp(name, "glDeletePathsNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeletePathsNV);
  if (strcmp(name, "glDeleteProgram") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteProgram);
  if (strcmp(name, "glDeleteQueries") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteQueries);
  if (strcmp(name, "glDeleteQueriesARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteQueriesARB);
  if (strcmp(name, "glDeleteQueriesEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteQueriesEXT);
  if (strcmp(name, "glDeleteRenderbuffers") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteRenderbuffers);
  if (strcmp(name, "glDeleteRenderbuffersEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDeleteRenderbuffersEXT);
  if (strcmp(name, "glDeleteSamplers") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteSamplers);
  if (strcmp(name, "glDeleteShader") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteShader);
  if (strcmp(name, "glDeleteSync") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteSync);
  if (strcmp(name, "glDeleteTextures") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteTextures);
  if (strcmp(name, "glDeleteTransformFeedbacks") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDeleteTransformFeedbacks);
  if (strcmp(name, "glDeleteVertexArrays") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDeleteVertexArrays);
  if (strcmp(name, "glDeleteVertexArraysAPPLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDeleteVertexArraysAPPLE);
  if (strcmp(name, "glDeleteVertexArraysOES") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDeleteVertexArraysOES);
  if (strcmp(name, "glDepthFunc") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDepthFunc);
  if (strcmp(name, "glDepthMask") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDepthMask);
  if (strcmp(name, "glDepthRange") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDepthRange);
  if (strcmp(name, "glDepthRangef") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDepthRangef);
  if (strcmp(name, "glDetachShader") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDetachShader);
  if (strcmp(name, "glDisable") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDisable);
  if (strcmp(name, "glDisableVertexAttribArray") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDisableVertexAttribArray);
  if (strcmp(name, "glDiscardFramebufferEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDiscardFramebufferEXT);
  if (strcmp(name, "glDrawArrays") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDrawArrays);
  if (strcmp(name, "glDrawArraysInstanced") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDrawArraysInstanced);
  if (strcmp(name, "glDrawArraysInstancedANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDrawArraysInstancedANGLE);
  if (strcmp(name, "glDrawArraysInstancedARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDrawArraysInstancedARB);
  if (strcmp(name, "glDrawBuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDrawBuffer);
  if (strcmp(name, "glDrawBuffers") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDrawBuffers);
  if (strcmp(name, "glDrawBuffersARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDrawBuffersARB);
  if (strcmp(name, "glDrawBuffersEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDrawBuffersEXT);
  if (strcmp(name, "glDrawElements") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDrawElements);
  if (strcmp(name, "glDrawElementsInstanced") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDrawElementsInstanced);
  if (strcmp(name, "glDrawElementsInstancedANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDrawElementsInstancedANGLE);
  if (strcmp(name, "glDrawElementsInstancedARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glDrawElementsInstancedARB);
  if (strcmp(name, "glDrawRangeElements") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glDrawRangeElements);
  if (strcmp(name, "glEGLImageTargetRenderbufferStorageOES") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glEGLImageTargetRenderbufferStorageOES);
  if (strcmp(name, "glEGLImageTargetTexture2DOES") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glEGLImageTargetTexture2DOES);
  if (strcmp(name, "glEnable") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glEnable);
  if (strcmp(name, "glEnableVertexAttribArray") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glEnableVertexAttribArray);
  if (strcmp(name, "glEndQuery") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glEndQuery);
  if (strcmp(name, "glEndQueryARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glEndQueryARB);
  if (strcmp(name, "glEndQueryEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glEndQueryEXT);
  if (strcmp(name, "glEndTransformFeedback") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glEndTransformFeedback);
  if (strcmp(name, "glEndTransformFeedbackEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glEndTransformFeedbackEXT);
  if (strcmp(name, "glFenceSync") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glFenceSync);
  if (strcmp(name, "glFinish") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glFinish);
  if (strcmp(name, "glFinishFenceAPPLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glFinishFenceAPPLE);
  if (strcmp(name, "glFinishFenceNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glFinishFenceNV);
  if (strcmp(name, "glFlush") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glFlush);
  if (strcmp(name, "glFlushMappedBufferRange") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glFlushMappedBufferRange);
  if (strcmp(name, "glFlushMappedBufferRangeEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glFlushMappedBufferRangeEXT);
  if (strcmp(name, "glFramebufferRenderbuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glFramebufferRenderbuffer);
  if (strcmp(name, "glFramebufferRenderbufferEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glFramebufferRenderbufferEXT);
  if (strcmp(name, "glFramebufferTexture2D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glFramebufferTexture2D);
  if (strcmp(name, "glFramebufferTexture2DEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glFramebufferTexture2DEXT);
  if (strcmp(name, "glFramebufferTexture2DMultisampleEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glFramebufferTexture2DMultisampleEXT);
  if (strcmp(name, "glFramebufferTexture2DMultisampleIMG") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glFramebufferTexture2DMultisampleIMG);
  if (strcmp(name, "glFramebufferTextureLayer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glFramebufferTextureLayer);
  if (strcmp(name, "glFrontFace") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glFrontFace);
  if (strcmp(name, "glGenBuffers") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenBuffers);
  if (strcmp(name, "glGenFencesAPPLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenFencesAPPLE);
  if (strcmp(name, "glGenFencesNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenFencesNV);
  if (strcmp(name, "glGenFramebuffers") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenFramebuffers);
  if (strcmp(name, "glGenFramebuffersEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenFramebuffersEXT);
  if (strcmp(name, "glGenPathsNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenPathsNV);
  if (strcmp(name, "glGenQueries") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenQueries);
  if (strcmp(name, "glGenQueriesARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenQueriesARB);
  if (strcmp(name, "glGenQueriesEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenQueriesEXT);
  if (strcmp(name, "glGenRenderbuffers") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenRenderbuffers);
  if (strcmp(name, "glGenRenderbuffersEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenRenderbuffersEXT);
  if (strcmp(name, "glGenSamplers") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenSamplers);
  if (strcmp(name, "glGenTextures") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenTextures);
  if (strcmp(name, "glGenTransformFeedbacks") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGenTransformFeedbacks);
  if (strcmp(name, "glGenVertexArrays") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenVertexArrays);
  if (strcmp(name, "glGenVertexArraysAPPLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenVertexArraysAPPLE);
  if (strcmp(name, "glGenVertexArraysOES") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenVertexArraysOES);
  if (strcmp(name, "glGenerateMipmap") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenerateMipmap);
  if (strcmp(name, "glGenerateMipmapEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGenerateMipmapEXT);
  if (strcmp(name, "glGetActiveAttrib") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetActiveAttrib);
  if (strcmp(name, "glGetActiveUniform") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetActiveUniform);
  if (strcmp(name, "glGetActiveUniformBlockName") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetActiveUniformBlockName);
  if (strcmp(name, "glGetActiveUniformBlockiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetActiveUniformBlockiv);
  if (strcmp(name, "glGetActiveUniformBlockivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetActiveUniformBlockivRobustANGLE);
  if (strcmp(name, "glGetActiveUniformsiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetActiveUniformsiv);
  if (strcmp(name, "glGetAttachedShaders") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetAttachedShaders);
  if (strcmp(name, "glGetAttribLocation") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetAttribLocation);
  if (strcmp(name, "glGetBooleani_vRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetBooleani_vRobustANGLE);
  if (strcmp(name, "glGetBooleanv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetBooleanv);
  if (strcmp(name, "glGetBooleanvRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetBooleanvRobustANGLE);
  if (strcmp(name, "glGetBufferParameteri64vRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetBufferParameteri64vRobustANGLE);
  if (strcmp(name, "glGetBufferParameteriv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetBufferParameteriv);
  if (strcmp(name, "glGetBufferParameterivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetBufferParameterivRobustANGLE);
  if (strcmp(name, "glGetBufferPointervRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetBufferPointervRobustANGLE);
  if (strcmp(name, "glGetDebugMessageLog") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetDebugMessageLog);
  if (strcmp(name, "glGetDebugMessageLogKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetDebugMessageLogKHR);
  if (strcmp(name, "glGetError") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetError);
  if (strcmp(name, "glGetFenceivNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetFenceivNV);
  if (strcmp(name, "glGetFloatv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetFloatv);
  if (strcmp(name, "glGetFloatvRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetFloatvRobustANGLE);
  if (strcmp(name, "glGetFragDataIndex") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetFragDataIndex);
  if (strcmp(name, "glGetFragDataIndexEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetFragDataIndexEXT);
  if (strcmp(name, "glGetFragDataLocation") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetFragDataLocation);
  if (strcmp(name, "glGetFramebufferAttachmentParameteriv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetFramebufferAttachmentParameteriv);
  if (strcmp(name, "glGetFramebufferAttachmentParameterivEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetFramebufferAttachmentParameterivEXT);
  if (strcmp(name, "glGetFramebufferAttachmentParameterivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetFramebufferAttachmentParameterivRobustANGLE);
  if (strcmp(name, "glGetFramebufferParameterivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetFramebufferParameterivRobustANGLE);
  if (strcmp(name, "glGetGraphicsResetStatus") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetGraphicsResetStatus);
  if (strcmp(name, "glGetGraphicsResetStatusARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetGraphicsResetStatusARB);
  if (strcmp(name, "glGetGraphicsResetStatusEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetGraphicsResetStatusEXT);
  if (strcmp(name, "glGetGraphicsResetStatusKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetGraphicsResetStatusKHR);
  if (strcmp(name, "glGetInteger64i_v") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetInteger64i_v);
  if (strcmp(name, "glGetInteger64i_vRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetInteger64i_vRobustANGLE);
  if (strcmp(name, "glGetInteger64v") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetInteger64v);
  if (strcmp(name, "glGetInteger64vRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetInteger64vRobustANGLE);
  if (strcmp(name, "glGetIntegeri_v") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetIntegeri_v);
  if (strcmp(name, "glGetIntegeri_vRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetIntegeri_vRobustANGLE);
  if (strcmp(name, "glGetIntegerv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetIntegerv);
  if (strcmp(name, "glGetIntegervRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetIntegervRobustANGLE);
  if (strcmp(name, "glGetInternalformativ") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetInternalformativ);
  if (strcmp(name, "glGetInternalformativRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetInternalformativRobustANGLE);
  if (strcmp(name, "glGetMultisamplefv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetMultisamplefv);
  if (strcmp(name, "glGetMultisamplefvRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetMultisamplefvRobustANGLE);
  if (strcmp(name, "glGetObjectLabel") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetObjectLabel);
  if (strcmp(name, "glGetObjectLabelKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetObjectLabelKHR);
  if (strcmp(name, "glGetObjectPtrLabel") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetObjectPtrLabel);
  if (strcmp(name, "glGetObjectPtrLabelKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetObjectPtrLabelKHR);
  if (strcmp(name, "glGetPointerv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetPointerv);
  if (strcmp(name, "glGetPointervKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetPointervKHR);
  if (strcmp(name, "glGetPointervRobustANGLERobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetPointervRobustANGLERobustANGLE);
  if (strcmp(name, "glGetProgramBinary") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetProgramBinary);
  if (strcmp(name, "glGetProgramBinaryOES") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetProgramBinaryOES);
  if (strcmp(name, "glGetProgramInfoLog") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetProgramInfoLog);
  if (strcmp(name, "glGetProgramInterfaceiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetProgramInterfaceiv);
  if (strcmp(name, "glGetProgramInterfaceivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetProgramInterfaceivRobustANGLE);
  if (strcmp(name, "glGetProgramResourceLocation") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetProgramResourceLocation);
  if (strcmp(name, "glGetProgramResourceName") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetProgramResourceName);
  if (strcmp(name, "glGetProgramResourceiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetProgramResourceiv);
  if (strcmp(name, "glGetProgramiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetProgramiv);
  if (strcmp(name, "glGetProgramivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetProgramivRobustANGLE);
  if (strcmp(name, "glGetQueryObjecti64v") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetQueryObjecti64v);
  if (strcmp(name, "glGetQueryObjecti64vEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetQueryObjecti64vEXT);
  if (strcmp(name, "glGetQueryObjecti64vRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetQueryObjecti64vRobustANGLE);
  if (strcmp(name, "glGetQueryObjectiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetQueryObjectiv);
  if (strcmp(name, "glGetQueryObjectivARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetQueryObjectivARB);
  if (strcmp(name, "glGetQueryObjectivEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetQueryObjectivEXT);
  if (strcmp(name, "glGetQueryObjectivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetQueryObjectivRobustANGLE);
  if (strcmp(name, "glGetQueryObjectui64v") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetQueryObjectui64v);
  if (strcmp(name, "glGetQueryObjectui64vEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetQueryObjectui64vEXT);
  if (strcmp(name, "glGetQueryObjectui64vRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetQueryObjectui64vRobustANGLE);
  if (strcmp(name, "glGetQueryObjectuiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetQueryObjectuiv);
  if (strcmp(name, "glGetQueryObjectuivARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetQueryObjectuivARB);
  if (strcmp(name, "glGetQueryObjectuivEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetQueryObjectuivEXT);
  if (strcmp(name, "glGetQueryObjectuivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetQueryObjectuivRobustANGLE);
  if (strcmp(name, "glGetQueryiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetQueryiv);
  if (strcmp(name, "glGetQueryivARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetQueryivARB);
  if (strcmp(name, "glGetQueryivEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetQueryivEXT);
  if (strcmp(name, "glGetQueryivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetQueryivRobustANGLE);
  if (strcmp(name, "glGetRenderbufferParameteriv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetRenderbufferParameteriv);
  if (strcmp(name, "glGetRenderbufferParameterivEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetRenderbufferParameterivEXT);
  if (strcmp(name, "glGetRenderbufferParameterivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetRenderbufferParameterivRobustANGLE);
  if (strcmp(name, "glGetSamplerParameterIivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetSamplerParameterIivRobustANGLE);
  if (strcmp(name, "glGetSamplerParameterIuivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetSamplerParameterIuivRobustANGLE);
  if (strcmp(name, "glGetSamplerParameterfv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetSamplerParameterfv);
  if (strcmp(name, "glGetSamplerParameterfvRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetSamplerParameterfvRobustANGLE);
  if (strcmp(name, "glGetSamplerParameteriv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetSamplerParameteriv);
  if (strcmp(name, "glGetSamplerParameterivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetSamplerParameterivRobustANGLE);
  if (strcmp(name, "glGetShaderInfoLog") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetShaderInfoLog);
  if (strcmp(name, "glGetShaderPrecisionFormat") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetShaderPrecisionFormat);
  if (strcmp(name, "glGetShaderSource") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetShaderSource);
  if (strcmp(name, "glGetShaderiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetShaderiv);
  if (strcmp(name, "glGetShaderivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetShaderivRobustANGLE);
  if (strcmp(name, "glGetString") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetString);
  if (strcmp(name, "glGetStringi") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetStringi);
  if (strcmp(name, "glGetSynciv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetSynciv);
  if (strcmp(name, "glGetTexLevelParameterfv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetTexLevelParameterfv);
  if (strcmp(name, "glGetTexLevelParameterfvRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetTexLevelParameterfvRobustANGLE);
  if (strcmp(name, "glGetTexLevelParameteriv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetTexLevelParameteriv);
  if (strcmp(name, "glGetTexLevelParameterivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetTexLevelParameterivRobustANGLE);
  if (strcmp(name, "glGetTexParameterIivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetTexParameterIivRobustANGLE);
  if (strcmp(name, "glGetTexParameterIuivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetTexParameterIuivRobustANGLE);
  if (strcmp(name, "glGetTexParameterfv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetTexParameterfv);
  if (strcmp(name, "glGetTexParameterfvRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetTexParameterfvRobustANGLE);
  if (strcmp(name, "glGetTexParameteriv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetTexParameteriv);
  if (strcmp(name, "glGetTexParameterivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetTexParameterivRobustANGLE);
  if (strcmp(name, "glGetTransformFeedbackVarying") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetTransformFeedbackVarying);
  if (strcmp(name, "glGetTransformFeedbackVaryingEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetTransformFeedbackVaryingEXT);
  if (strcmp(name, "glGetTranslatedShaderSourceANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetTranslatedShaderSourceANGLE);
  if (strcmp(name, "glGetUniformBlockIndex") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetUniformBlockIndex);
  if (strcmp(name, "glGetUniformIndices") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetUniformIndices);
  if (strcmp(name, "glGetUniformLocation") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetUniformLocation);
  if (strcmp(name, "glGetUniformfv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetUniformfv);
  if (strcmp(name, "glGetUniformfvRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetUniformfvRobustANGLE);
  if (strcmp(name, "glGetUniformiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetUniformiv);
  if (strcmp(name, "glGetUniformivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetUniformivRobustANGLE);
  if (strcmp(name, "glGetUniformuiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetUniformuiv);
  if (strcmp(name, "glGetUniformuivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetUniformuivRobustANGLE);
  if (strcmp(name, "glGetVertexAttribIivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetVertexAttribIivRobustANGLE);
  if (strcmp(name, "glGetVertexAttribIuivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetVertexAttribIuivRobustANGLE);
  if (strcmp(name, "glGetVertexAttribPointerv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetVertexAttribPointerv);
  if (strcmp(name, "glGetVertexAttribPointervRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetVertexAttribPointervRobustANGLE);
  if (strcmp(name, "glGetVertexAttribfv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetVertexAttribfv);
  if (strcmp(name, "glGetVertexAttribfvRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetVertexAttribfvRobustANGLE);
  if (strcmp(name, "glGetVertexAttribiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glGetVertexAttribiv);
  if (strcmp(name, "glGetVertexAttribivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetVertexAttribivRobustANGLE);
  if (strcmp(name, "glGetnUniformfvRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetnUniformfvRobustANGLE);
  if (strcmp(name, "glGetnUniformivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetnUniformivRobustANGLE);
  if (strcmp(name, "glGetnUniformuivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glGetnUniformuivRobustANGLE);
  if (strcmp(name, "glHint") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glHint);
  if (strcmp(name, "glInsertEventMarkerEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glInsertEventMarkerEXT);
  if (strcmp(name, "glInvalidateFramebuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glInvalidateFramebuffer);
  if (strcmp(name, "glInvalidateSubFramebuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glInvalidateSubFramebuffer);
  if (strcmp(name, "glIsBuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsBuffer);
  if (strcmp(name, "glIsEnabled") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsEnabled);
  if (strcmp(name, "glIsFenceAPPLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsFenceAPPLE);
  if (strcmp(name, "glIsFenceNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsFenceNV);
  if (strcmp(name, "glIsFramebuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsFramebuffer);
  if (strcmp(name, "glIsFramebufferEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsFramebufferEXT);
  if (strcmp(name, "glIsPathNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsPathNV);
  if (strcmp(name, "glIsProgram") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsProgram);
  if (strcmp(name, "glIsQuery") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsQuery);
  if (strcmp(name, "glIsQueryARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsQueryARB);
  if (strcmp(name, "glIsQueryEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsQueryEXT);
  if (strcmp(name, "glIsRenderbuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsRenderbuffer);
  if (strcmp(name, "glIsRenderbufferEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsRenderbufferEXT);
  if (strcmp(name, "glIsSampler") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsSampler);
  if (strcmp(name, "glIsShader") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsShader);
  if (strcmp(name, "glIsSync") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsSync);
  if (strcmp(name, "glIsTexture") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsTexture);
  if (strcmp(name, "glIsTransformFeedback") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsTransformFeedback);
  if (strcmp(name, "glIsVertexArray") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsVertexArray);
  if (strcmp(name, "glIsVertexArrayAPPLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsVertexArrayAPPLE);
  if (strcmp(name, "glIsVertexArrayOES") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glIsVertexArrayOES);
  if (strcmp(name, "glLineWidth") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glLineWidth);
  if (strcmp(name, "glLinkProgram") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glLinkProgram);
  if (strcmp(name, "glMapBuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glMapBuffer);
  if (strcmp(name, "glMapBufferOES") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glMapBufferOES);
  if (strcmp(name, "glMapBufferRange") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glMapBufferRange);
  if (strcmp(name, "glMapBufferRangeEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glMapBufferRangeEXT);
  if (strcmp(name, "glMatrixLoadIdentityEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glMatrixLoadIdentityEXT);
  if (strcmp(name, "glMatrixLoadfEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glMatrixLoadfEXT);
  if (strcmp(name, "glMemoryBarrier") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glMemoryBarrier);
  if (strcmp(name, "glMemoryBarrierEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glMemoryBarrierEXT);
  if (strcmp(name, "glObjectLabel") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glObjectLabel);
  if (strcmp(name, "glObjectLabelKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glObjectLabelKHR);
  if (strcmp(name, "glObjectPtrLabel") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glObjectPtrLabel);
  if (strcmp(name, "glObjectPtrLabelKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glObjectPtrLabelKHR);
  if (strcmp(name, "glPathCommandsNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPathCommandsNV);
  if (strcmp(name, "glPathParameterfNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPathParameterfNV);
  if (strcmp(name, "glPathParameteriNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPathParameteriNV);
  if (strcmp(name, "glPathStencilFuncNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPathStencilFuncNV);
  if (strcmp(name, "glPauseTransformFeedback") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glPauseTransformFeedback);
  if (strcmp(name, "glPixelStorei") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPixelStorei);
  if (strcmp(name, "glPointParameteri") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPointParameteri);
  if (strcmp(name, "glPolygonMode") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPolygonMode);
  if (strcmp(name, "glPolygonOffset") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPolygonOffset);
  if (strcmp(name, "glPopDebugGroup") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPopDebugGroup);
  if (strcmp(name, "glPopDebugGroupKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPopDebugGroupKHR);
  if (strcmp(name, "glPopGroupMarkerEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPopGroupMarkerEXT);
  if (strcmp(name, "glPrimitiveRestartIndex") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glPrimitiveRestartIndex);
  if (strcmp(name, "glProgramBinary") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glProgramBinary);
  if (strcmp(name, "glProgramBinaryOES") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glProgramBinaryOES);
  if (strcmp(name, "glProgramParameteri") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glProgramParameteri);
  if (strcmp(name, "glProgramPathFragmentInputGenNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glProgramPathFragmentInputGenNV);
  if (strcmp(name, "glPushDebugGroup") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPushDebugGroup);
  if (strcmp(name, "glPushDebugGroupKHR") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPushDebugGroupKHR);
  if (strcmp(name, "glPushGroupMarkerEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glPushGroupMarkerEXT);
  if (strcmp(name, "glQueryCounter") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glQueryCounter);
  if (strcmp(name, "glQueryCounterEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glQueryCounterEXT);
  if (strcmp(name, "glReadBuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glReadBuffer);
  if (strcmp(name, "glReadPixels") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glReadPixels);
  if (strcmp(name, "glReadPixelsRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glReadPixelsRobustANGLE);
  if (strcmp(name, "glReadnPixelsRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glReadnPixelsRobustANGLE);
  if (strcmp(name, "glReleaseShaderCompiler") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glReleaseShaderCompiler);
  if (strcmp(name, "glRenderbufferStorage") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glRenderbufferStorage);
  if (strcmp(name, "glRenderbufferStorageEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glRenderbufferStorageEXT);
  if (strcmp(name, "glRenderbufferStorageMultisample") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glRenderbufferStorageMultisample);
  if (strcmp(name, "glRenderbufferStorageMultisampleANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glRenderbufferStorageMultisampleANGLE);
  if (strcmp(name, "glRenderbufferStorageMultisampleEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glRenderbufferStorageMultisampleEXT);
  if (strcmp(name, "glRenderbufferStorageMultisampleIMG") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glRenderbufferStorageMultisampleIMG);
  if (strcmp(name, "glRequestExtensionANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glRequestExtensionANGLE);
  if (strcmp(name, "glResumeTransformFeedback") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glResumeTransformFeedback);
  if (strcmp(name, "glSampleCoverage") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glSampleCoverage);
  if (strcmp(name, "glSamplerParameterIivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glSamplerParameterIivRobustANGLE);
  if (strcmp(name, "glSamplerParameterIuivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glSamplerParameterIuivRobustANGLE);
  if (strcmp(name, "glSamplerParameterf") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glSamplerParameterf);
  if (strcmp(name, "glSamplerParameterfv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glSamplerParameterfv);
  if (strcmp(name, "glSamplerParameterfvRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glSamplerParameterfvRobustANGLE);
  if (strcmp(name, "glSamplerParameteri") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glSamplerParameteri);
  if (strcmp(name, "glSamplerParameteriv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glSamplerParameteriv);
  if (strcmp(name, "glSamplerParameterivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glSamplerParameterivRobustANGLE);
  if (strcmp(name, "glScissor") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glScissor);
  if (strcmp(name, "glSetFenceAPPLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glSetFenceAPPLE);
  if (strcmp(name, "glSetFenceNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glSetFenceNV);
  if (strcmp(name, "glShaderBinary") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glShaderBinary);
  if (strcmp(name, "glShaderSource") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glShaderSource);
  if (strcmp(name, "glStencilFillPathInstancedNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glStencilFillPathInstancedNV);
  if (strcmp(name, "glStencilFillPathNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glStencilFillPathNV);
  if (strcmp(name, "glStencilFunc") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glStencilFunc);
  if (strcmp(name, "glStencilFuncSeparate") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glStencilFuncSeparate);
  if (strcmp(name, "glStencilMask") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glStencilMask);
  if (strcmp(name, "glStencilMaskSeparate") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glStencilMaskSeparate);
  if (strcmp(name, "glStencilOp") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glStencilOp);
  if (strcmp(name, "glStencilOpSeparate") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glStencilOpSeparate);
  if (strcmp(name, "glStencilStrokePathInstancedNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glStencilStrokePathInstancedNV);
  if (strcmp(name, "glStencilStrokePathNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glStencilStrokePathNV);
  if (strcmp(name, "glStencilThenCoverFillPathInstancedNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glStencilThenCoverFillPathInstancedNV);
  if (strcmp(name, "glStencilThenCoverFillPathNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glStencilThenCoverFillPathNV);
  if (strcmp(name, "glStencilThenCoverStrokePathInstancedNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glStencilThenCoverStrokePathInstancedNV);
  if (strcmp(name, "glStencilThenCoverStrokePathNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glStencilThenCoverStrokePathNV);
  if (strcmp(name, "glTestFenceAPPLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTestFenceAPPLE);
  if (strcmp(name, "glTestFenceNV") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTestFenceNV);
  if (strcmp(name, "glTexBuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexBuffer);
  if (strcmp(name, "glTexBufferEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexBufferEXT);
  if (strcmp(name, "glTexBufferOES") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexBufferOES);
  if (strcmp(name, "glTexBufferRange") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexBufferRange);
  if (strcmp(name, "glTexBufferRangeEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexBufferRangeEXT);
  if (strcmp(name, "glTexBufferRangeOES") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexBufferRangeOES);
  if (strcmp(name, "glTexImage2D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexImage2D);
  if (strcmp(name, "glTexImage2DRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glTexImage2DRobustANGLE);
  if (strcmp(name, "glTexImage3D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexImage3D);
  if (strcmp(name, "glTexImage3DRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glTexImage3DRobustANGLE);
  if (strcmp(name, "glTexParameterIivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glTexParameterIivRobustANGLE);
  if (strcmp(name, "glTexParameterIuivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glTexParameterIuivRobustANGLE);
  if (strcmp(name, "glTexParameterf") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexParameterf);
  if (strcmp(name, "glTexParameterfv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexParameterfv);
  if (strcmp(name, "glTexParameterfvRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glTexParameterfvRobustANGLE);
  if (strcmp(name, "glTexParameteri") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexParameteri);
  if (strcmp(name, "glTexParameteriv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexParameteriv);
  if (strcmp(name, "glTexParameterivRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glTexParameterivRobustANGLE);
  if (strcmp(name, "glTexStorage2D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexStorage2D);
  if (strcmp(name, "glTexStorage2DEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexStorage2DEXT);
  if (strcmp(name, "glTexStorage3D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexStorage3D);
  if (strcmp(name, "glTexSubImage2D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexSubImage2D);
  if (strcmp(name, "glTexSubImage2DRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glTexSubImage2DRobustANGLE);
  if (strcmp(name, "glTexSubImage3D") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glTexSubImage3D);
  if (strcmp(name, "glTexSubImage3DRobustANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glTexSubImage3DRobustANGLE);
  if (strcmp(name, "glTransformFeedbackVaryings") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glTransformFeedbackVaryings);
  if (strcmp(name, "glTransformFeedbackVaryingsEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glTransformFeedbackVaryingsEXT);
  if (strcmp(name, "glUniform1f") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform1f);
  if (strcmp(name, "glUniform1fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform1fv);
  if (strcmp(name, "glUniform1i") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform1i);
  if (strcmp(name, "glUniform1iv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform1iv);
  if (strcmp(name, "glUniform1ui") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform1ui);
  if (strcmp(name, "glUniform1uiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform1uiv);
  if (strcmp(name, "glUniform2f") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform2f);
  if (strcmp(name, "glUniform2fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform2fv);
  if (strcmp(name, "glUniform2i") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform2i);
  if (strcmp(name, "glUniform2iv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform2iv);
  if (strcmp(name, "glUniform2ui") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform2ui);
  if (strcmp(name, "glUniform2uiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform2uiv);
  if (strcmp(name, "glUniform3f") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform3f);
  if (strcmp(name, "glUniform3fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform3fv);
  if (strcmp(name, "glUniform3i") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform3i);
  if (strcmp(name, "glUniform3iv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform3iv);
  if (strcmp(name, "glUniform3ui") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform3ui);
  if (strcmp(name, "glUniform3uiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform3uiv);
  if (strcmp(name, "glUniform4f") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform4f);
  if (strcmp(name, "glUniform4fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform4fv);
  if (strcmp(name, "glUniform4i") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform4i);
  if (strcmp(name, "glUniform4iv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform4iv);
  if (strcmp(name, "glUniform4ui") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform4ui);
  if (strcmp(name, "glUniform4uiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniform4uiv);
  if (strcmp(name, "glUniformBlockBinding") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniformBlockBinding);
  if (strcmp(name, "glUniformMatrix2fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniformMatrix2fv);
  if (strcmp(name, "glUniformMatrix2x3fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniformMatrix2x3fv);
  if (strcmp(name, "glUniformMatrix2x4fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniformMatrix2x4fv);
  if (strcmp(name, "glUniformMatrix3fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniformMatrix3fv);
  if (strcmp(name, "glUniformMatrix3x2fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniformMatrix3x2fv);
  if (strcmp(name, "glUniformMatrix3x4fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniformMatrix3x4fv);
  if (strcmp(name, "glUniformMatrix4fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniformMatrix4fv);
  if (strcmp(name, "glUniformMatrix4x2fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniformMatrix4x2fv);
  if (strcmp(name, "glUniformMatrix4x3fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUniformMatrix4x3fv);
  if (strcmp(name, "glUnmapBuffer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUnmapBuffer);
  if (strcmp(name, "glUnmapBufferOES") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUnmapBufferOES);
  if (strcmp(name, "glUseProgram") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glUseProgram);
  if (strcmp(name, "glValidateProgram") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glValidateProgram);
  if (strcmp(name, "glVertexAttrib1f") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttrib1f);
  if (strcmp(name, "glVertexAttrib1fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttrib1fv);
  if (strcmp(name, "glVertexAttrib2f") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttrib2f);
  if (strcmp(name, "glVertexAttrib2fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttrib2fv);
  if (strcmp(name, "glVertexAttrib3f") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttrib3f);
  if (strcmp(name, "glVertexAttrib3fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttrib3fv);
  if (strcmp(name, "glVertexAttrib4f") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttrib4f);
  if (strcmp(name, "glVertexAttrib4fv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttrib4fv);
  if (strcmp(name, "glVertexAttribDivisor") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttribDivisor);
  if (strcmp(name, "glVertexAttribDivisorANGLE") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glVertexAttribDivisorANGLE);
  if (strcmp(name, "glVertexAttribDivisorARB") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glVertexAttribDivisorARB);
  if (strcmp(name, "glVertexAttribDivisorEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(
        Mock_glVertexAttribDivisorEXT);
  if (strcmp(name, "glVertexAttribI4i") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttribI4i);
  if (strcmp(name, "glVertexAttribI4iv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttribI4iv);
  if (strcmp(name, "glVertexAttribI4ui") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttribI4ui);
  if (strcmp(name, "glVertexAttribI4uiv") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttribI4uiv);
  if (strcmp(name, "glVertexAttribIPointer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttribIPointer);
  if (strcmp(name, "glVertexAttribPointer") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glVertexAttribPointer);
  if (strcmp(name, "glViewport") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glViewport);
  if (strcmp(name, "glWaitSync") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glWaitSync);
  if (strcmp(name, "glWindowRectanglesEXT") == 0)
    return reinterpret_cast<GLFunctionPointerType>(Mock_glWindowRectanglesEXT);
  return reinterpret_cast<GLFunctionPointerType>(&MockInvalidFunction);
}

}  // namespace gl
