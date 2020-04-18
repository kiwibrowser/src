// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/test_gles2_interface.h"

#include "base/logging.h"
#include "components/viz/test/test_web_graphics_context_3d.h"
#include "gpu/GLES2/gl2extchromium.h"

namespace viz {

TestGLES2Interface::TestGLES2Interface() {
  set_have_extension_egl_image(true);  // For stream textures.
  set_max_texture_size(2048);
}

TestGLES2Interface::~TestGLES2Interface() = default;

void TestGLES2Interface::GenTextures(GLsizei n, GLuint* textures) {
  for (GLsizei i = 0; i < n; ++i) {
    textures[i] = test_context_->createTexture();
  }
}

void TestGLES2Interface::GenBuffers(GLsizei n, GLuint* buffers) {
  for (GLsizei i = 0; i < n; ++i) {
    buffers[i] = test_context_->createBuffer();
  }
}

void TestGLES2Interface::GenFramebuffers(GLsizei n, GLuint* framebuffers) {
  for (GLsizei i = 0; i < n; ++i) {
    framebuffers[i] = test_context_->createFramebuffer();
  }
}

void TestGLES2Interface::GenRenderbuffers(GLsizei n, GLuint* renderbuffers) {
  for (GLsizei i = 0; i < n; ++i) {
    renderbuffers[i] = test_context_->createRenderbuffer();
  }
}

void TestGLES2Interface::GenQueriesEXT(GLsizei n, GLuint* queries) {
  for (GLsizei i = 0; i < n; ++i) {
    queries[i] = test_context_->createQueryEXT();
  }
}

void TestGLES2Interface::DeleteTextures(GLsizei n, const GLuint* textures) {
  test_context_->deleteTextures(n, textures);
}

void TestGLES2Interface::DeleteBuffers(GLsizei n, const GLuint* buffers) {
  for (GLsizei i = 0; i < n; ++i) {
    test_context_->deleteBuffer(buffers[i]);
  }
}

void TestGLES2Interface::DeleteFramebuffers(GLsizei n,
                                            const GLuint* framebuffers) {
  for (GLsizei i = 0; i < n; ++i) {
    test_context_->deleteFramebuffer(framebuffers[i]);
  }
}

void TestGLES2Interface::DeleteQueriesEXT(GLsizei n, const GLuint* queries) {
  for (GLsizei i = 0; i < n; ++i) {
    test_context_->deleteQueryEXT(queries[i]);
  }
}

GLuint TestGLES2Interface::CreateShader(GLenum type) {
  return test_context_->createShader(type);
}

GLuint TestGLES2Interface::CreateProgram() {
  return test_context_->createProgram();
}

void TestGLES2Interface::BindTexture(GLenum target, GLuint texture) {
  test_context_->bindTexture(target, texture);
}

void TestGLES2Interface::GetIntegerv(GLenum pname, GLint* params) {
  test_context_->getIntegerv(pname, params);
}

void TestGLES2Interface::GetShaderiv(GLuint shader,
                                     GLenum pname,
                                     GLint* params) {
  test_context_->getShaderiv(shader, pname, params);
}

void TestGLES2Interface::GetProgramiv(GLuint program,
                                      GLenum pname,
                                      GLint* params) {
  test_context_->getProgramiv(program, pname, params);
}

void TestGLES2Interface::GetShaderPrecisionFormat(GLenum shadertype,
                                                  GLenum precisiontype,
                                                  GLint* range,
                                                  GLint* precision) {
  test_context_->getShaderPrecisionFormat(shadertype, precisiontype, range,
                                          precision);
}

void TestGLES2Interface::Viewport(GLint x,
                                  GLint y,
                                  GLsizei width,
                                  GLsizei height) {
  test_context_->viewport(x, y, width, height);
}

void TestGLES2Interface::ActiveTexture(GLenum target) {
  test_context_->activeTexture(target);
}

void TestGLES2Interface::UseProgram(GLuint program) {
  test_context_->useProgram(program);
}

GLenum TestGLES2Interface::CheckFramebufferStatus(GLenum target) {
  return test_context_->checkFramebufferStatus(target);
}

void TestGLES2Interface::Scissor(GLint x,
                                 GLint y,
                                 GLsizei width,
                                 GLsizei height) {
  test_context_->scissor(x, y, width, height);
}

void TestGLES2Interface::DrawElements(GLenum mode,
                                      GLsizei count,
                                      GLenum type,
                                      const void* indices) {
  test_context_->drawElements(mode, count, type,
                              reinterpret_cast<intptr_t>(indices));
}

void TestGLES2Interface::ClearColor(GLclampf red,
                                    GLclampf green,
                                    GLclampf blue,
                                    GLclampf alpha) {
  test_context_->clearColor(red, green, blue, alpha);
}

void TestGLES2Interface::ClearStencil(GLint s) {
  test_context_->clearStencil(s);
}

void TestGLES2Interface::Clear(GLbitfield mask) {
  test_context_->clear(mask);
}

void TestGLES2Interface::Flush() {
  test_context_->flush();
}

void TestGLES2Interface::Finish() {
  test_context_->finish();
}

void TestGLES2Interface::ShallowFinishCHROMIUM() {
  test_context_->shallowFinishCHROMIUM();
}

void TestGLES2Interface::ShallowFlushCHROMIUM() {
  test_context_->shallowFlushCHROMIUM();
}

void TestGLES2Interface::Enable(GLenum cap) {
  test_context_->enable(cap);
}

void TestGLES2Interface::Disable(GLenum cap) {
  test_context_->disable(cap);
}

void TestGLES2Interface::BindRenderbuffer(GLenum target, GLuint buffer) {
  test_context_->bindRenderbuffer(target, buffer);
}

void TestGLES2Interface::BindFramebuffer(GLenum target, GLuint buffer) {
  test_context_->bindFramebuffer(target, buffer);
}

void TestGLES2Interface::BindBuffer(GLenum target, GLuint buffer) {
  test_context_->bindBuffer(target, buffer);
}

void TestGLES2Interface::PixelStorei(GLenum pname, GLint param) {
  test_context_->pixelStorei(pname, param);
}

void TestGLES2Interface::TexImage2D(GLenum target,
                                    GLint level,
                                    GLint internalformat,
                                    GLsizei width,
                                    GLsizei height,
                                    GLint border,
                                    GLenum format,
                                    GLenum type,
                                    const void* pixels) {
  test_context_->texImage2D(target, level, internalformat, width, height,
                            border, format, type, pixels);
}

void TestGLES2Interface::TexSubImage2D(GLenum target,
                                       GLint level,
                                       GLint xoffset,
                                       GLint yoffset,
                                       GLsizei width,
                                       GLsizei height,
                                       GLenum format,
                                       GLenum type,
                                       const void* pixels) {
  test_context_->texSubImage2D(target, level, xoffset, yoffset, width, height,
                               format, type, pixels);
}

void TestGLES2Interface::TexStorage2DEXT(GLenum target,
                                         GLsizei levels,
                                         GLenum internalformat,
                                         GLsizei width,
                                         GLsizei height) {
  test_context_->texStorage2DEXT(target, levels, internalformat, width, height);
}

void TestGLES2Interface::TexStorage2DImageCHROMIUM(GLenum target,
                                                   GLenum internalformat,
                                                   GLenum bufferusage,
                                                   GLsizei width,
                                                   GLsizei height) {
  test_context_->texStorage2DImageCHROMIUM(target, internalformat, bufferusage,
                                           width, height);
}

void TestGLES2Interface::TexParameteri(GLenum target,
                                       GLenum pname,
                                       GLint param) {
  test_context_->texParameteri(target, pname, param);
}

void TestGLES2Interface::FramebufferRenderbuffer(GLenum target,
                                                 GLenum attachment,
                                                 GLenum renderbuffertarget,
                                                 GLuint renderbuffer) {
  test_context_->framebufferRenderbuffer(target, attachment, renderbuffertarget,
                                         renderbuffer);
}
void TestGLES2Interface::FramebufferTexture2D(GLenum target,
                                              GLenum attachment,
                                              GLenum textarget,
                                              GLuint texture,
                                              GLint level) {
  test_context_->framebufferTexture2D(target, attachment, textarget, texture,
                                      level);
}

void TestGLES2Interface::RenderbufferStorage(GLenum target,
                                             GLenum internalformat,
                                             GLsizei width,
                                             GLsizei height) {
  test_context_->renderbufferStorage(target, internalformat, width, height);
}

void TestGLES2Interface::CompressedTexImage2D(GLenum target,
                                              GLint level,
                                              GLenum internalformat,
                                              GLsizei width,
                                              GLsizei height,
                                              GLint border,
                                              GLsizei image_size,
                                              const void* data) {
  test_context_->compressedTexImage2D(target, level, internalformat, width,
                                      height, border, image_size, data);
}

GLuint TestGLES2Interface::CreateImageCHROMIUM(ClientBuffer buffer,
                                               GLsizei width,
                                               GLsizei height,
                                               GLenum internalformat) {
  return test_context_->createImageCHROMIUM(buffer, width, height,
                                            internalformat);
}

void TestGLES2Interface::DestroyImageCHROMIUM(GLuint image_id) {
  test_context_->destroyImageCHROMIUM(image_id);
}

void TestGLES2Interface::BindTexImage2DCHROMIUM(GLenum target, GLint image_id) {
  test_context_->bindTexImage2DCHROMIUM(target, image_id);
}

void TestGLES2Interface::ReleaseTexImage2DCHROMIUM(GLenum target,
                                                   GLint image_id) {
  test_context_->releaseTexImage2DCHROMIUM(target, image_id);
}

void* TestGLES2Interface::MapBufferCHROMIUM(GLuint target, GLenum access) {
  return test_context_->mapBufferCHROMIUM(target, access);
}

GLboolean TestGLES2Interface::UnmapBufferCHROMIUM(GLuint target) {
  return test_context_->unmapBufferCHROMIUM(target);
}

void TestGLES2Interface::BufferData(GLenum target,
                                    GLsizeiptr size,
                                    const void* data,
                                    GLenum usage) {
  test_context_->bufferData(target, size, data, usage);
}

void TestGLES2Interface::GenSyncTokenCHROMIUM(GLbyte* sync_token) {
  test_context_->genSyncToken(sync_token);
}

void TestGLES2Interface::GenUnverifiedSyncTokenCHROMIUM(GLbyte* sync_token) {
  test_context_->genSyncToken(sync_token);
}

void TestGLES2Interface::VerifySyncTokensCHROMIUM(GLbyte** sync_tokens,
                                                  GLsizei count) {
  test_context_->verifySyncTokens(sync_tokens, count);
}

void TestGLES2Interface::WaitSyncTokenCHROMIUM(const GLbyte* sync_token) {
  test_context_->waitSyncToken(sync_token);
}

void TestGLES2Interface::BeginQueryEXT(GLenum target, GLuint id) {
  test_context_->beginQueryEXT(target, id);
}

void TestGLES2Interface::EndQueryEXT(GLenum target) {
  test_context_->endQueryEXT(target);
}

void TestGLES2Interface::GetQueryObjectuivEXT(GLuint id,
                                              GLenum pname,
                                              GLuint* params) {
  test_context_->getQueryObjectuivEXT(id, pname, params);
}

void TestGLES2Interface::DiscardFramebufferEXT(GLenum target,
                                               GLsizei count,
                                               const GLenum* attachments) {
  test_context_->discardFramebufferEXT(target, count, attachments);
}

void TestGLES2Interface::GenMailboxCHROMIUM(GLbyte* mailbox) {
  test_context_->genMailboxCHROMIUM(mailbox);
}

void TestGLES2Interface::ProduceTextureDirectCHROMIUM(GLuint texture,
                                                      const GLbyte* mailbox) {
  test_context_->produceTextureDirectCHROMIUM(texture, mailbox);
}

GLuint TestGLES2Interface::CreateAndConsumeTextureCHROMIUM(
    const GLbyte* mailbox) {
  return test_context_->createAndConsumeTextureCHROMIUM(mailbox);
}

void TestGLES2Interface::ResizeCHROMIUM(GLuint width,
                                        GLuint height,
                                        float device_scale,
                                        GLenum color_space,
                                        GLboolean has_alpha) {
  test_context_->reshapeWithScaleFactor(width, height, device_scale);
}

void TestGLES2Interface::LoseContextCHROMIUM(GLenum current, GLenum other) {
  test_context_->loseContextCHROMIUM(current, other);
}

GLenum TestGLES2Interface::GetGraphicsResetStatusKHR() {
  if (test_context_->isContextLost())
    return GL_UNKNOWN_CONTEXT_RESET_KHR;
  return GL_NO_ERROR;
}

size_t TestGLES2Interface::NumTextures() const {
  return test_context_->NumTextures();
}

void TestGLES2Interface::set_test_context(TestWebGraphicsContext3D* context) {
  DCHECK(!test_context_);
  test_context_ = context;
  InitializeTestContext();
  test_context_->set_capabilities(test_capabilities_);
}

void TestGLES2Interface::set_times_bind_texture_succeeds(int times) {
  test_context_->set_times_bind_texture_succeeds(times);
}

void TestGLES2Interface::set_have_extension_io_surface(bool have) {
  test_capabilities_.iosurface = have;
  test_capabilities_.texture_rectangle = have;
}

void TestGLES2Interface::set_have_extension_egl_image(bool have) {
  test_capabilities_.egl_image_external = have;
}

void TestGLES2Interface::set_have_post_sub_buffer(bool have) {
  test_capabilities_.post_sub_buffer = have;
}

void TestGLES2Interface::set_have_swap_buffers_with_bounds(bool have) {
  test_capabilities_.swap_buffers_with_bounds = have;
}

void TestGLES2Interface::set_have_commit_overlay_planes(bool have) {
  test_capabilities_.commit_overlay_planes = have;
}

void TestGLES2Interface::set_have_discard_framebuffer(bool have) {
  test_capabilities_.discard_framebuffer = have;
}

void TestGLES2Interface::set_support_compressed_texture_etc1(bool support) {
  test_capabilities_.texture_format_etc1 = support;
}

void TestGLES2Interface::set_support_texture_format_bgra8888(bool support) {
  test_capabilities_.texture_format_bgra8888 = support;
}

void TestGLES2Interface::set_support_texture_storage(bool support) {
  test_capabilities_.texture_storage = support;
}

void TestGLES2Interface::set_support_texture_usage(bool support) {
  test_capabilities_.texture_usage = support;
}

void TestGLES2Interface::set_support_sync_query(bool support) {
  test_capabilities_.sync_query = support;
}

void TestGLES2Interface::set_support_texture_rectangle(bool support) {
  test_capabilities_.texture_rectangle = support;
}

void TestGLES2Interface::set_support_texture_half_float_linear(bool support) {
  test_capabilities_.texture_half_float_linear = support;
  // TODO(xing.xu) : Below workaround should be removed when set_test_context
  // removed.
  if (test_context_)
    test_context_->set_support_texture_half_float_linear(support);
}

void TestGLES2Interface::set_support_texture_norm16(bool support) {
  test_capabilities_.texture_norm16 = support;
  // TODO(xing.xu) : Below workaround should be removed when set_test_context
  // removed.
  if (test_context_)
    test_context_->set_support_texture_norm16(support);
}

void TestGLES2Interface::set_msaa_is_slow(bool msaa_is_slow) {
  test_capabilities_.msaa_is_slow = msaa_is_slow;
}

void TestGLES2Interface::set_gpu_rasterization(bool gpu_rasterization) {
  test_capabilities_.gpu_rasterization = gpu_rasterization;
}

void TestGLES2Interface::set_avoid_stencil_buffers(bool avoid_stencil_buffers) {
  test_capabilities_.avoid_stencil_buffers = avoid_stencil_buffers;
}

void TestGLES2Interface::set_enable_dc_layers(bool support) {
  test_capabilities_.dc_layers = support;
}

void TestGLES2Interface::set_support_multisample_compatibility(bool support) {
  test_capabilities_.multisample_compatibility = support;
}

void TestGLES2Interface::set_support_texture_storage_image(bool support) {
  test_capabilities_.texture_storage_image = support;
}

void TestGLES2Interface::set_support_texture_npot(bool support) {
  test_capabilities_.texture_npot = support;
}

void TestGLES2Interface::set_max_texture_size(int size) {
  test_capabilities_.max_texture_size = size;
}

}  // namespace viz
