// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_TEST_TEST_WEB_GRAPHICS_CONTEXT_3D_H_
#define COMPONENTS_VIZ_TEST_TEST_WEB_GRAPHICS_CONTEXT_3D_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/stl_util.h"
#include "base/synchronization/lock.h"
#include "components/viz/test/ordered_texture_map.h"
#include "components/viz/test/test_texture.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "ui/gfx/geometry/rect.h"

extern "C" typedef struct _ClientBuffer* ClientBuffer;

namespace viz {

class TestContextSupport;

class TestWebGraphicsContext3D {
 public:
  static std::unique_ptr<TestWebGraphicsContext3D> Create();

  virtual ~TestWebGraphicsContext3D();

  void set_context_lost_callback(const base::Closure& callback) {
    context_lost_callback_ = callback;
  }

  virtual void reshapeWithScaleFactor(int width,
                                      int height,
                                      float scale_factor);

  virtual bool isContextLost();

  virtual void discardFramebufferEXT(GLenum target,
                                     GLsizei num_attachments,
                                     const GLenum* attachments) {}

  virtual void activeTexture(GLenum texture) {}
  virtual void attachShader(GLuint program, GLuint shader);
  virtual void bindFramebuffer(GLenum target, GLuint framebuffer);
  virtual void bindRenderbuffer(GLenum target, GLuint renderbuffer);
  virtual void bindTexture(GLenum target, GLuint texture_id);

  virtual void texParameteri(GLenum target, GLenum pname, GLint param);
  virtual void getTexParameteriv(GLenum target, GLenum pname, GLint* value);
  virtual void asyncTexImage2DCHROMIUM(GLenum target,
                                       GLint level,
                                       GLenum internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLint border,
                                       GLenum format,
                                       GLenum type,
                                       const void* pixels) {}
  virtual void asyncTexSubImage2DCHROMIUM(GLenum target,
                                          GLint level,
                                          GLint xoffset,
                                          GLint yoffset,
                                          GLsizei width,
                                          GLsizei height,
                                          GLenum format,
                                          GLenum type,
                                          const void* pixels) {}
  virtual void waitAsyncTexImage2DCHROMIUM(GLenum target) {}
  virtual void releaseTexImage2DCHROMIUM(GLenum target, GLint image_id) {}
  virtual void framebufferRenderbuffer(GLenum target,
                                       GLenum attachment,
                                       GLenum renderbuffertarget,
                                       GLuint renderbuffer) {}
  virtual void framebufferTexture2D(GLenum target,
                                    GLenum attachment,
                                    GLenum textarget,
                                    GLuint texture,
                                    GLint level) {}
  virtual void renderbufferStorage(GLenum target,
                                   GLenum internalformat,
                                   GLsizei width,
                                   GLsizei height) {}

  virtual GLenum checkFramebufferStatus(GLenum target);

  virtual void clear(GLbitfield mask) {}
  virtual void clearColor(GLclampf red,
                          GLclampf green,
                          GLclampf blue,
                          GLclampf alpha) {}
  virtual void clearStencil(GLint s) {}
  virtual void compressedTexImage2D(GLenum target,
                                    GLint level,
                                    GLenum internal_format,
                                    GLsizei width,
                                    GLsizei height,
                                    GLint border,
                                    GLsizei image_size,
                                    const void* data) {}
  virtual GLint getUniformLocation(GLuint program, const GLchar* name);
  virtual GLsizeiptr getVertexAttribOffset(GLuint index, GLenum pname);

  virtual GLboolean isBuffer(GLuint buffer);
  virtual GLboolean isEnabled(GLenum cap);
  virtual GLboolean isFramebuffer(GLuint framebuffer);
  virtual GLboolean isProgram(GLuint program);
  virtual GLboolean isRenderbuffer(GLuint renderbuffer);
  virtual GLboolean isShader(GLuint shader);
  virtual GLboolean isTexture(GLuint texture);

  virtual void useProgram(GLuint program);

  virtual void viewport(GLint x, GLint y, GLsizei width, GLsizei height) {}

  virtual void genBuffers(GLsizei count, GLuint* ids);
  virtual void genFramebuffers(GLsizei count, GLuint* ids);
  virtual void genRenderbuffers(GLsizei count, GLuint* ids);
  virtual void genTextures(GLsizei count, GLuint* ids);

  virtual void deleteBuffers(GLsizei count, const GLuint* ids);
  virtual void deleteFramebuffers(GLsizei count, const GLuint* ids);
  virtual void deleteRenderbuffers(GLsizei count, const GLuint* ids);
  virtual void deleteTextures(GLsizei count, const GLuint* ids);

  virtual GLuint createBuffer();
  virtual GLuint createFramebuffer();
  virtual GLuint createRenderbuffer();
  virtual GLuint createTexture();

  virtual void deleteBuffer(GLuint id);
  virtual void deleteFramebuffer(GLuint id);
  virtual void deleteRenderbuffer(GLuint id);
  virtual void deleteTexture(GLuint id);

  virtual GLuint createProgram();
  virtual GLuint createShader(GLenum);
  virtual GLuint createExternalTexture();

  virtual void deleteProgram(GLuint id);
  virtual void deleteShader(GLuint id);

  virtual void texStorage2DEXT(GLenum target,
                               GLsizei levels,
                               GLenum internalformat,
                               GLsizei width,
                               GLsizei height) {}
  virtual void texStorage2DImageCHROMIUM(GLenum target,
                                         GLenum internalformat,
                                         GLenum bufferusage,
                                         GLsizei width,
                                         GLsizei height) {}

  virtual GLuint createQueryEXT();
  virtual void deleteQueryEXT(GLuint query) {}
  virtual void beginQueryEXT(GLenum target, GLuint query) {}
  virtual void endQueryEXT(GLenum target);
  virtual void getQueryObjectuivEXT(GLuint query, GLenum pname, GLuint* params);

  virtual void scissor(GLint x, GLint y, GLsizei width, GLsizei height) {}

  virtual void texImage2D(GLenum target,
                          GLint level,
                          GLenum internalformat,
                          GLsizei width,
                          GLsizei height,
                          GLint border,
                          GLenum format,
                          GLenum type,
                          const void* pixels) {}

  virtual void texSubImage2D(GLenum target,
                             GLint level,
                             GLint xoffset,
                             GLint yoffset,
                             GLsizei width,
                             GLsizei height,
                             GLenum format,
                             GLenum type,
                             const void* pixels) {}

  virtual void genMailboxCHROMIUM(GLbyte* mailbox);
  virtual void produceTextureDirectCHROMIUM(GLuint texture,
                                            const GLbyte* mailbox) {}
  virtual GLuint createAndConsumeTextureCHROMIUM(const GLbyte* mailbox);

  virtual void loseContextCHROMIUM(GLenum current, GLenum other);

  virtual void bindTexImage2DCHROMIUM(GLenum target, GLint image_id) {}

  virtual void drawArrays(GLenum mode, GLint first, GLsizei count) {}
  virtual void drawElements(GLenum mode,
                            GLsizei count,
                            GLenum type,
                            GLintptr offset) {}
  virtual void disable(GLenum cap) {}
  virtual void enable(GLenum cap) {}
  virtual void finish();
  virtual void flush();
  virtual void shallowFinishCHROMIUM();
  virtual void shallowFlushCHROMIUM() {}

  virtual void getAttachedShaders(GLuint program,
                                  GLsizei max_count,
                                  GLsizei* count,
                                  GLuint* shaders) {}
  virtual GLint getAttribLocation(GLuint program, const GLchar* name);
  virtual void getBooleanv(GLenum pname, GLboolean* value) {}
  virtual void getBufferParameteriv(GLenum target, GLenum pname, GLint* value) {
  }
  virtual GLenum getError();
  virtual void getFloatv(GLenum pname, GLfloat* value) {}
  virtual void getFramebufferAttachmentParameteriv(GLenum target,
                                                   GLenum attachment,
                                                   GLenum pname,
                                                   GLint* value) {}

  virtual void getIntegerv(GLenum pname, GLint* value);

  virtual void getProgramiv(GLuint program, GLenum pname, GLint* value);

  virtual void getRenderbufferParameteriv(GLenum target,
                                          GLenum pname,
                                          GLint* value) {}

  virtual void getShaderiv(GLuint shader, GLenum pname, GLint* value);

  virtual void getShaderPrecisionFormat(GLenum shadertype,
                                        GLenum precisiontype,
                                        GLint* range,
                                        GLint* precision);

  virtual void getTexParameterfv(GLenum target, GLenum pname, GLfloat* value) {}
  virtual void getUniformfv(GLuint program, GLint location, GLfloat* value) {}
  virtual void getUniformiv(GLuint program, GLint location, GLint* value) {}
  virtual void getVertexAttribfv(GLuint index, GLenum pname, GLfloat* value) {}
  virtual void getVertexAttribiv(GLuint index, GLenum pname, GLint* value) {}

  virtual void bindBuffer(GLenum target, GLuint buffer);
  virtual void bufferData(GLenum target,
                          GLsizeiptr size,
                          const void* data,
                          GLenum usage);
  virtual void pixelStorei(GLenum pname, GLint param);
  virtual void* mapBufferCHROMIUM(GLenum target, GLenum access);
  virtual GLboolean unmapBufferCHROMIUM(GLenum target);

  virtual GLuint createImageCHROMIUM(ClientBuffer buffer,
                                     GLsizei width,
                                     GLsizei height,
                                     GLenum internalformat);
  virtual void destroyImageCHROMIUM(GLuint image_id);
  virtual GLuint createGpuMemoryBufferImageCHROMIUM(GLsizei width,
                                                    GLsizei height,
                                                    GLenum internalformat,
                                                    GLenum usage);

  virtual void genSyncToken(GLbyte* sync_token);
  virtual void waitSyncToken(const GLbyte* sync_token);
  virtual void verifySyncTokens(GLbyte** sync_tokens, GLsizei count);

  const gpu::SyncToken& last_waited_sync_token() const {
    return last_waited_sync_token_;
  }

  const gpu::Capabilities& test_capabilities() const {
    return test_capabilities_;
  }

  void set_capabilities(const gpu::Capabilities& caps) {
    test_capabilities_ = caps;
  }

  void set_context_lost(bool context_lost) { context_lost_ = context_lost; }
  void set_times_bind_texture_succeeds(int times) {
    times_bind_texture_succeeds_ = times;
  }
  void set_times_end_query_succeeds(int times) {
    times_end_query_succeeds_ = times;
  }

  // When set, mapBufferCHROMIUM will return NULL after this many times.
  void set_times_map_buffer_chromium_succeeds(int times) {
    times_map_buffer_chromium_succeeds_ = times;
  }

  size_t NumTextures() const;
  GLuint TextureAt(int i) const;

  size_t NumUsedTextures() const { return used_textures_.size(); }
  bool UsedTexture(int texture) const {
    return base::ContainsKey(used_textures_, texture);
  }
  void ResetUsedTextures() { used_textures_.clear(); }

  void set_have_extension_io_surface(bool have) {
    test_capabilities_.iosurface = have;
    test_capabilities_.texture_rectangle = have;
  }
  void set_have_extension_egl_image(bool have) {
    test_capabilities_.egl_image_external = have;
  }
  void set_have_post_sub_buffer(bool have) {
    test_capabilities_.post_sub_buffer = have;
  }
  void set_have_swap_buffers_with_bounds(bool have) {
    test_capabilities_.swap_buffers_with_bounds = have;
  }
  void set_have_commit_overlay_planes(bool have) {
    test_capabilities_.commit_overlay_planes = have;
  }
  void set_have_discard_framebuffer(bool have) {
    test_capabilities_.discard_framebuffer = have;
  }
  void set_support_compressed_texture_etc1(bool support) {
    test_capabilities_.texture_format_etc1 = support;
  }
  void set_support_texture_format_bgra8888(bool support) {
    test_capabilities_.texture_format_bgra8888 = support;
  }
  void set_support_texture_storage(bool support) {
    test_capabilities_.texture_storage = support;
  }
  void set_support_texture_usage(bool support) {
    test_capabilities_.texture_usage = support;
  }
  void set_support_sync_query(bool support) {
    test_capabilities_.sync_query = support;
  }
  void set_support_texture_rectangle(bool support) {
    test_capabilities_.texture_rectangle = support;
  }
  void set_support_texture_half_float_linear(bool support) {
    test_capabilities_.texture_half_float_linear = support;
  }
  void set_support_texture_norm16(bool support) {
    test_capabilities_.texture_norm16 = support;
  }
  void set_msaa_is_slow(bool msaa_is_slow) {
    test_capabilities_.msaa_is_slow = msaa_is_slow;
  }
  void set_gpu_rasterization(bool gpu_rasterization) {
    test_capabilities_.gpu_rasterization = gpu_rasterization;
  }
  void set_avoid_stencil_buffers(bool avoid_stencil_buffers) {
    test_capabilities_.avoid_stencil_buffers = avoid_stencil_buffers;
  }
  void set_enable_dc_layers(bool support) {
    test_capabilities_.dc_layers = support;
  }
  void set_support_multisample_compatibility(bool support) {
    test_capabilities_.multisample_compatibility = support;
  }
  void set_support_texture_storage_image(bool support) {
    test_capabilities_.texture_storage_image = support;
  }
  void set_support_texture_npot(bool support) {
    test_capabilities_.texture_npot = support;
  }

  // When this context is lost, all contexts in its share group are also lost.
  void add_share_group_context(TestWebGraphicsContext3D* context3d) {
    shared_contexts_.push_back(context3d);
  }

  void set_max_texture_size(int size) {
    test_capabilities_.max_texture_size = size;
  }

  static const GLuint kExternalTextureId;
  virtual GLuint NextTextureId();
  virtual void RetireTextureId(GLuint id);

  virtual GLuint NextBufferId();
  virtual void RetireBufferId(GLuint id);

  virtual GLuint NextImageId();
  virtual void RetireImageId(GLuint id);

  virtual GLuint NextFramebufferId();
  virtual void RetireFramebufferId(GLuint id);

  virtual GLuint NextRenderbufferId();
  virtual void RetireRenderbufferId(GLuint id);

  void SetMaxSamples(int max_samples);
  void set_test_support(TestContextSupport* test_support) {
    test_support_ = test_support;
  }

  int width() const { return width_; }
  int height() const { return height_; }
  bool reshape_called() const { return reshape_called_; }
  void clear_reshape_called() { reshape_called_ = false; }
  float scale_factor() const { return scale_factor_; }

  enum UpdateType { NO_UPDATE = 0, PREPARE_TEXTURE, POST_SUB_BUFFER };

  gfx::Rect update_rect() const { return update_rect_; }

  UpdateType last_update_type() { return last_update_type_; }

  size_t NumFramebuffers() const;
  size_t NumRenderbuffers() const;

 protected:
  struct TextureTargets {
    TextureTargets();
    ~TextureTargets();

    void BindTexture(GLenum target, GLuint id);
    void UnbindTexture(GLuint id);

    GLuint BoundTexture(GLenum target);

   private:
    using TargetTextureMap = std::unordered_map<GLenum, GLuint>;
    TargetTextureMap bound_textures_;
  };

  struct Buffer {
    Buffer();
    ~Buffer();

    GLenum target;
    std::unique_ptr<uint8_t[]> pixels;
    size_t size;

   private:
    DISALLOW_COPY_AND_ASSIGN(Buffer);
  };

  struct Image {
    Image();
    ~Image();

    std::unique_ptr<uint8_t[]> pixels;

   private:
    DISALLOW_COPY_AND_ASSIGN(Image);
  };

  struct Namespace : public base::RefCountedThreadSafe<Namespace> {
    Namespace();

    // Protects all fields.
    base::Lock lock;
    unsigned next_buffer_id;
    unsigned next_image_id;
    unsigned next_texture_id;
    unsigned next_renderbuffer_id;
    std::unordered_map<unsigned, std::unique_ptr<Buffer>> buffers;
    std::unordered_set<unsigned> images;
    OrderedTextureMap textures;
    std::unordered_set<unsigned> renderbuffer_set;

   private:
    friend class base::RefCountedThreadSafe<Namespace>;
    ~Namespace();
    DISALLOW_COPY_AND_ASSIGN(Namespace);
  };

  TestWebGraphicsContext3D();

  void CreateNamespace();
  GLuint BoundTextureId(GLenum target);
  scoped_refptr<TestTexture> BoundTexture(GLenum target);
  scoped_refptr<TestTexture> UnboundTexture(GLuint texture);
  void CheckTextureIsBound(GLenum target);

  unsigned context_id_;
  gpu::Capabilities test_capabilities_;
  int times_bind_texture_succeeds_;
  int times_end_query_succeeds_;
  bool context_lost_;
  int times_map_buffer_chromium_succeeds_;
  base::Closure context_lost_callback_;
  std::unordered_set<unsigned> used_textures_;
  unsigned next_program_id_;
  std::unordered_set<unsigned> program_set_;
  unsigned next_shader_id_;
  std::unordered_set<unsigned> shader_set_;
  unsigned next_framebuffer_id_;
  std::unordered_set<unsigned> framebuffer_set_;
  unsigned current_framebuffer_;
  std::vector<TestWebGraphicsContext3D*> shared_contexts_;
  bool reshape_called_;
  int width_;
  int height_;
  float scale_factor_;
  TestContextSupport* test_support_;
  gfx::Rect update_rect_;
  UpdateType last_update_type_;
  GLuint64 next_insert_fence_sync_;
  gpu::SyncToken last_waited_sync_token_;
  int unpack_alignment_;

  base::flat_map<unsigned, unsigned> bound_buffer_;
  TextureTargets texture_targets_;

  scoped_refptr<Namespace> namespace_;
  static Namespace* shared_namespace_;

  base::WeakPtrFactory<TestWebGraphicsContext3D> weak_ptr_factory_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_TEST_TEST_WEB_GRAPHICS_CONTEXT_3D_H_
