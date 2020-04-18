// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_GL_HELPER_H_
#define COMPONENTS_VIZ_COMMON_GL_HELPER_H_

#include <memory>

#include "base/atomicops.h"
#include "base/callback.h"
#include "base/macros.h"
#include "components/viz/common/viz_common_export.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/size.h"

namespace gfx {
class Point;
class Rect;
class Vector2d;
class Vector2dF;
}  // namespace gfx

namespace gpu {
class ContextSupport;
struct Mailbox;
}  // namespace gpu

class SkRegion;

namespace viz {

class GLHelperScaling;

class ScopedGLuint {
 public:
  typedef void (gpu::gles2::GLES2Interface::*GenFunc)(GLsizei n, GLuint* ids);
  typedef void (gpu::gles2::GLES2Interface::*DeleteFunc)(GLsizei n,
                                                         const GLuint* ids);
  ScopedGLuint(gpu::gles2::GLES2Interface* gl,
               GenFunc gen_func,
               DeleteFunc delete_func)
      : gl_(gl), id_(0u), delete_func_(delete_func) {
    (gl_->*gen_func)(1, &id_);
  }

  operator GLuint() const { return id_; }

  GLuint id() const { return id_; }

  ~ScopedGLuint() {
    if (id_ != 0) {
      (gl_->*delete_func_)(1, &id_);
    }
  }

 private:
  gpu::gles2::GLES2Interface* gl_;
  GLuint id_;
  DeleteFunc delete_func_;

  DISALLOW_COPY_AND_ASSIGN(ScopedGLuint);
};

class ScopedBuffer : public ScopedGLuint {
 public:
  explicit ScopedBuffer(gpu::gles2::GLES2Interface* gl)
      : ScopedGLuint(gl,
                     &gpu::gles2::GLES2Interface::GenBuffers,
                     &gpu::gles2::GLES2Interface::DeleteBuffers) {}
};

class ScopedFramebuffer : public ScopedGLuint {
 public:
  explicit ScopedFramebuffer(gpu::gles2::GLES2Interface* gl)
      : ScopedGLuint(gl,
                     &gpu::gles2::GLES2Interface::GenFramebuffers,
                     &gpu::gles2::GLES2Interface::DeleteFramebuffers) {}
};

class ScopedTexture : public ScopedGLuint {
 public:
  explicit ScopedTexture(gpu::gles2::GLES2Interface* gl)
      : ScopedGLuint(gl,
                     &gpu::gles2::GLES2Interface::GenTextures,
                     &gpu::gles2::GLES2Interface::DeleteTextures) {}
};

template <GLenum Target>
class ScopedBinder {
 public:
  typedef void (gpu::gles2::GLES2Interface::*BindFunc)(GLenum target,
                                                       GLuint id);
  ScopedBinder(gpu::gles2::GLES2Interface* gl, GLuint id, BindFunc bind_func)
      : gl_(gl), bind_func_(bind_func) {
    (gl_->*bind_func_)(Target, id);
  }

  virtual ~ScopedBinder() { (gl_->*bind_func_)(Target, 0); }

 private:
  gpu::gles2::GLES2Interface* gl_;
  BindFunc bind_func_;

  DISALLOW_COPY_AND_ASSIGN(ScopedBinder);
};

template <GLenum Target>
class ScopedBufferBinder : ScopedBinder<Target> {
 public:
  ScopedBufferBinder(gpu::gles2::GLES2Interface* gl, GLuint id)
      : ScopedBinder<Target>(gl, id, &gpu::gles2::GLES2Interface::BindBuffer) {}
};

template <GLenum Target>
class ScopedFramebufferBinder : ScopedBinder<Target> {
 public:
  ScopedFramebufferBinder(gpu::gles2::GLES2Interface* gl, GLuint id)
      : ScopedBinder<Target>(gl,
                             id,
                             &gpu::gles2::GLES2Interface::BindFramebuffer) {}
};

template <GLenum Target>
class ScopedTextureBinder : ScopedBinder<Target> {
 public:
  ScopedTextureBinder(gpu::gles2::GLES2Interface* gl, GLuint id)
      : ScopedBinder<Target>(gl, id, &gpu::gles2::GLES2Interface::BindTexture) {
  }
};

class GLHelperReadbackSupport;
class I420Converter;
class ReadbackYUVInterface;

// Provides higher level operations on top of the gpu::gles2::GLES2Interface
// interfaces.
class VIZ_COMMON_EXPORT GLHelper {
 public:
  GLHelper(gpu::gles2::GLES2Interface* gl,
           gpu::ContextSupport* context_support);
  ~GLHelper();

  enum ScalerQuality {
    // Bilinear single pass, fastest possible.
    SCALER_QUALITY_FAST = 1,

    // Bilinear upscale + N * 50% bilinear downscales.
    // This is still fast enough for most purposes and
    // Image quality is nearly as good as the BEST option.
    SCALER_QUALITY_GOOD = 2,

    // Bicubic upscale + N * 50% bicubic downscales.
    // Produces very good quality scaled images, but it's
    // 2-8x slower than the "GOOD" quality, so it's not always
    // worth it.
    SCALER_QUALITY_BEST = 3,
  };

  // Copies the block of pixels specified with |src_subrect| from |src_texture|,
  // scales it to |dst_size|, and writes it into |out|.
  // |src_size| is the size of |src_texture|. The result is in |out_color_type|
  // format and is potentially flipped vertically to make it a correct image
  // representation.  |callback| is invoked with the copy result when the copy
  // operation has completed.
  // Note that the src_texture will have the min/mag filter set to GL_LINEAR
  // and wrap_s/t set to CLAMP_TO_EDGE in this call.
  void CropScaleReadbackAndCleanTexture(
      GLuint src_texture,
      const gfx::Size& src_size,
      const gfx::Size& dst_size,
      unsigned char* out,
      const SkColorType out_color_type,
      const base::Callback<void(bool)>& callback,
      GLHelper::ScalerQuality quality);

  // Copies the all pixels from the texture in |src_mailbox| of |src_size|,
  // scales it to |dst_size|, and writes it into |out|. The result is in
  // |out_color_type| format and is potentially flipped vertically to make it a
  // correct image representation. |callback| is invoked with the copy result
  // when the copy operation has completed.
  // Note that the texture bound to src_mailbox will have the min/mag filter set
  // to GL_LINEAR and wrap_s/t set to CLAMP_TO_EDGE in this call. src_mailbox is
  // assumed to be GL_TEXTURE_2D.
  void CropScaleReadbackAndCleanMailbox(
      const gpu::Mailbox& src_mailbox,
      const gpu::SyncToken& sync_token,
      const gfx::Size& src_size,
      const gfx::Size& dst_size,
      unsigned char* out,
      const SkColorType out_color_type,
      const base::Callback<void(bool)>& callback,
      GLHelper::ScalerQuality quality);

  // Copies the texture data out of |texture| into |out|.  |size| is the
  // size of the texture.  No post processing is applied to the pixels.  The
  // texture is assumed to have a format of GL_RGBA with a pixel type of
  // GL_UNSIGNED_BYTE.  This is a blocking call that calls glReadPixels on the
  // current OpenGL context.
  void ReadbackTextureSync(GLuint texture,
                           const gfx::Rect& src_rect,
                           unsigned char* out,
                           SkColorType format);

  void ReadbackTextureAsync(GLuint texture,
                            const gfx::Size& dst_size,
                            unsigned char* out,
                            SkColorType color_type,
                            const base::Callback<void(bool)>& callback);

  // Creates a scaled copy of the specified texture. |src_size| is the size of
  // the texture and |dst_size| is the size of the resulting copy.
  // Note that the |texture| will have the min/mag filter set to GL_LINEAR
  // and wrap_s/t set to CLAMP_TO_EDGE in this call. Returns 0 on invalid
  // arguments.
  GLuint CopyAndScaleTexture(GLuint texture,
                             const gfx::Size& src_size,
                             const gfx::Size& dst_size,
                             bool vertically_flip_texture,
                             ScalerQuality quality);

  // Returns the shader compiled from the source.
  GLuint CompileShaderFromSource(const GLchar* source, GLenum type);

  // Copies all pixels from |previous_texture| into |texture| that are
  // inside the region covered by |old_damage| but not part of |new_damage|.
  void CopySubBufferDamage(GLenum target,
                           GLuint texture,
                           GLuint previous_texture,
                           const SkRegion& new_damage,
                           const SkRegion& old_damage);

  // Simply creates a texture.
  GLuint CreateTexture();
  // Deletes a texture.
  void DeleteTexture(GLuint texture_id);

  // Inserts a fence sync, flushes, and generates a sync token.
  void GenerateSyncToken(gpu::SyncToken* sync_token);

  // Wait for the sync token before executing further GL commands.
  void WaitSyncToken(const gpu::SyncToken& sync_token);

  // Creates a mailbox holder that is attached to the given texture id, with a
  // sync point to wait on before using the mailbox. Returns a holder with an
  // empty mailbox on failure.
  // Note the texture is assumed to be GL_TEXTURE_2D.
  gpu::MailboxHolder ProduceMailboxHolderFromTexture(GLuint texture_id);

  // Creates a texture and consumes a mailbox into it. Returns 0 on failure.
  // Note the mailbox is assumed to be GL_TEXTURE_2D.
  GLuint ConsumeMailboxToTexture(const gpu::Mailbox& mailbox,
                                 const gpu::SyncToken& sync_token);

  // Resizes the texture's size to |size|.
  void ResizeTexture(GLuint texture, const gfx::Size& size);

  // Copies the framebuffer data given in |rect| to |texture|.
  void CopyTextureSubImage(GLuint texture, const gfx::Rect& rect);

  // Copies the all framebuffer data to |texture|. |size| specifies the
  // size of the framebuffer.
  void CopyTextureFullImage(GLuint texture, const gfx::Size& size);

  // Flushes GL commands.
  void Flush();

  // Force commands in the current command buffer to be executed before commands
  // in other command buffers from the same process (ie channel to the GPU
  // process).
  void InsertOrderingBarrier();

  // Caches all intermediate textures and programs needed to scale any subset of
  // a source texture at a fixed scaling ratio.
  class ScalerInterface {
   public:
    virtual ~ScalerInterface() {}

    // Scales a portion of |src_texture| and draws the result into
    // |dest_texture| at offset (0, 0).
    //
    // |src_texture_size| is the full, allocated size of the |src_texture|. This
    // is required for computing texture coordinate transforms (and only because
    // the OpenGL ES 2.0 API lacks the ability to query this info).
    //
    // |src_offset| is the offset in the source texture corresponding to point
    // (0,0) in the source/output coordinate spaces. This prevents the need for
    // extra texture copies just to re-position the source coordinate system.
    // TODO(crbug.com/775740): This must be set to whole-numbered values for
    // now, until the implementation is modified to handle fractional offsets.
    //
    // |output_rect| selects the region to draw (in the scaled, not the source,
    // coordinate space). This is used to save work in cases where only a
    // portion needs to be re-scaled. The implementation will back-compute,
    // internally, to determine the region of the |src_texture| to sample.
    //
    // WARNING: The output will always be placed at (0, 0) in the
    // |dest_texture|, and not at |output_rect.origin()|.
    //
    // Note that the src_texture will have the min/mag filter set to GL_LINEAR
    // and wrap_s/t set to CLAMP_TO_EDGE in this call.
    void Scale(GLuint src_texture,
               const gfx::Size& src_texture_size,
               const gfx::Vector2dF& src_offset,
               GLuint dest_texture,
               const gfx::Rect& output_rect) {
      ScaleToMultipleOutputs(src_texture, src_texture_size, src_offset,
                             dest_texture, 0, output_rect);
    }

    // Same as above, but for shaders that output to two textures at once.
    virtual void ScaleToMultipleOutputs(GLuint src_texture,
                                        const gfx::Size& src_texture_size,
                                        const gfx::Vector2dF& src_offset,
                                        GLuint dest_texture_0,
                                        GLuint dest_texture_1,
                                        const gfx::Rect& output_rect) = 0;

    // Given the |src_texture_size|, |src_offset| and |output_rect| arguments
    // that would be passed to Scale(), compute the region of pixels in the
    // source texture that would be sampled to produce a scaled result. The
    // result is stored in |sampling_rect|, along with the |offset| to the (0,0)
    // point relative to |sampling_rect|'s origin.
    //
    // This is used by clients that need to know the minimal portion of a source
    // buffer that must be copied without affecting Scale()'s results. This
    // method also accounts for vertical flipping.
    virtual void ComputeRegionOfInfluence(const gfx::Size& src_texture_size,
                                          const gfx::Vector2dF& src_offset,
                                          const gfx::Rect& output_rect,
                                          gfx::Rect* sampling_rect,
                                          gfx::Vector2dF* offset) const = 0;

    // Returns true if from:to represent the same scale ratio as that provided
    // by this scaler.
    virtual bool IsSameScaleRatio(const gfx::Vector2d& from,
                                  const gfx::Vector2d& to) const = 0;

    // Returns true if the scaler is assuming the source texture's content is
    // vertically flipped.
    virtual bool IsSamplingFlippedSource() const = 0;

    // Returns true if the scaler will vertically-flip the output. Note that if
    // both this method and IsSamplingFlippedSource() return true, then the
    // scaler output will be right-side up.
    virtual bool IsFlippingOutput() const = 0;

    // Returns the format to use when calling glReadPixels() to read-back the
    // output texture(s). This indicates whether the 0th and 2nd bytes in each
    // RGBA quad have been swapped. If no swapping has occurred, this will
    // return GL_RGBA. Otherwise, it will return GL_BGRA_EXT.
    virtual GLenum GetReadbackFormat() const = 0;

   protected:
    ScalerInterface() {}

   private:
    DISALLOW_COPY_AND_ASSIGN(ScalerInterface);
  };

  // Create a scaler that upscales or downscales at the given ratio
  // (scale_from:scale_to). Returns null on invalid arguments.
  //
  // If |flipped_source| is true, then the scaler will assume the content of the
  // source texture is vertically-flipped. This is required so that the scaler
  // can correctly compute the sampling region.
  //
  // If |flip_output| is true, then the scaler will vertically-flip its output
  // result. This is used when the output texture will be read-back into system
  // memory, so that the rows do not have to be copied in reverse.
  //
  // If |swizzle| is true, the 0th and 2nd elements in each RGBA quad will be
  // swapped. This is beneficial for optimizing read-back into system memory.
  //
  // WARNING: The returned scaler assumes both this GLHelper and its
  // GLES2Interface/ContextSupport will outlive it!
  std::unique_ptr<ScalerInterface> CreateScaler(ScalerQuality quality,
                                                const gfx::Vector2d& scale_from,
                                                const gfx::Vector2d& scale_to,
                                                bool flipped_source,
                                                bool flip_output,
                                                bool swizzle);

  // Create a pipeline that will (optionally) scale a source texture, and then
  // convert it to I420 (YUV) planar form, delivering results in three separate
  // output textures (one for each plane; see I420Converter::Convert()).
  //
  // Due to limitations in the OpenGL ES 2.0 API, the output textures will have
  // a format of GL_RGBA. However, each RGBA "pixel" in these textures actually
  // carries 4 consecutive pixels for the single-color-channel result plane.
  // Therefore, when using the OpenGL APIs to read-back the image into system
  // memory, note that a width 1/4 the actual |output_rect.width()| must be
  // used.
  //
  // |flipped_source|, |flip_output|, and |swizzle| have the same meaning as
  // that explained in the method comments for CreateScaler().
  //
  // If |use_mrt| is true, the pipeline will try to optimize the YUV conversion
  // using the multi-render-target extension, if the platform is capable.
  // |use_mrt| should only be set to false for testing.
  //
  // The benefit of using this pipeline is seen when these output textures are
  // read back from GPU to CPU memory: The I420 format reduces the amount of
  // data read back by a factor of ~2.6 (32bpp → 12bpp) which can greatly
  // improve performance, for things like video screen capture, on platforms
  // with slow GPU read-back performance.
  //
  // WARNING: The returned I420Converter instance assumes both this GLHelper and
  // its GLES2Interface/ContextSupport will outlive it!
  std::unique_ptr<I420Converter> CreateI420Converter(bool flipped_source,
                                                     bool flip_output,
                                                     bool swizzle,
                                                     bool use_mrt);

  // Create a readback pipeline that will (optionally) scale a source texture,
  // then convert it to YUV420 planar form, and finally read back that. This
  // reduces the amount of memory read from GPU to CPU memory by a factor of 2.6
  // (32bpp → 12bpp), which can be quite handy since readbacks have very limited
  // speed on some platforms.
  //
  // If |use_mrt| is true, the pipeline will try to optimize the YUV conversion
  // using the multi-render-target extension, if the platform is capable.
  // |use_mrt| should only be set to false for testing.
  //
  // WARNING: The returned ReadbackYUVInterface instance assumes both this
  // GLHelper and its GLES2Interface/ContextSupport will outlive it!
  //
  // TODO(crbug/754872): DEPRECATED. This will be removed soon, in favor of
  // CreateI420Converter().
  std::unique_ptr<ReadbackYUVInterface> CreateReadbackPipelineYUV(
      bool vertically_flip_texture,
      bool use_mrt);

  // Returns a ReadbackYUVInterface instance that is lazily created and owned by
  // this class. |use_mrt| is always true for these instances.
  ReadbackYUVInterface* GetReadbackPipelineYUV(bool vertically_flip_texture);

  // Returns the maximum number of draw buffers available,
  // 0 if GL_EXT_draw_buffers is not available.
  GLint MaxDrawBuffers();

  // Checks whether the readbback is supported for texture with the
  // matching config. This doesnt check for cross format readbacks.
  bool IsReadbackConfigSupported(SkColorType texture_format);

  // Returns a GLHelperReadbackSupport instance, for querying platform readback
  // capabilities and to determine the more-performant configurations.
  GLHelperReadbackSupport* GetReadbackSupport();

 protected:
  class CopyTextureToImpl;

  // Creates |copy_texture_to_impl_| if NULL.
  void InitCopyTextToImpl();
  // Creates |scaler_impl_| if NULL.
  void InitScalerImpl();
  // Creates |readback_support_| if NULL.
  void LazyInitReadbackSupportImpl();

  enum ReadbackSwizzle { kSwizzleNone = 0, kSwizzleBGRA };

  gpu::gles2::GLES2Interface* gl_;
  gpu::ContextSupport* context_support_;
  std::unique_ptr<CopyTextureToImpl> copy_texture_to_impl_;
  std::unique_ptr<GLHelperScaling> scaler_impl_;
  std::unique_ptr<GLHelperReadbackSupport> readback_support_;
  std::unique_ptr<ReadbackYUVInterface> shared_readback_yuv_flip_;
  std::unique_ptr<ReadbackYUVInterface> shared_readback_yuv_noflip_;

 private:
  DISALLOW_COPY_AND_ASSIGN(GLHelper);
};

// Splits an RGBA source texture's image into separate Y, U, and V planes. The U
// and V planes are half-width and half-height, according to the I420 standard.
class VIZ_COMMON_EXPORT I420Converter {
 public:
  I420Converter();
  virtual ~I420Converter();

  // Transforms a RGBA |src_texture| into three textures, each containing bytes
  // in I420 planar form. See the GLHelper::ScalerInterface::Scale() method
  // comments for the meaning/semantics of |src_texture_size|, |src_offset| and
  // |output_rect|. If |optional_scaler| is not null, it will first be used to
  // scale the source texture into an intermediate texture before generating the
  // Y+U+V planes.
  //
  // See notes for CreateI420Converter() regarding the semantics of the output
  // textures.
  virtual void Convert(GLuint src_texture,
                       const gfx::Size& src_texture_size,
                       const gfx::Vector2dF& src_offset,
                       GLHelper::ScalerInterface* optional_scaler,
                       const gfx::Rect& output_rect,
                       GLuint y_plane_texture,
                       GLuint u_plane_texture,
                       GLuint v_plane_texture) = 0;

  // Returns true if the converter is assuming the source texture's content is
  // vertically flipped.
  virtual bool IsSamplingFlippedSource() const = 0;

  // Returns true if the converter will vertically-flip the output.
  virtual bool IsFlippingOutput() const = 0;

  // Returns the format to use when calling glReadPixels() to read-back the
  // output textures. This indicates whether the 0th and 2nd bytes in each RGBA
  // quad have been swapped. If no swapping has occurred, this will return
  // GL_RGBA. Otherwise, it will return GL_BGRA_EXT.
  virtual GLenum GetReadbackFormat() const = 0;

  // Returns the texture size of the Y plane texture, based on the size of the
  // |output_rect| that was given to Convert(). This will have a width of
  // CEIL(output_rect_size.width() / 4), and the same height.
  static gfx::Size GetYPlaneTextureSize(const gfx::Size& output_rect_size);

  // Like GetYPlaneTextureSize(), except the returned size will have a width of
  // CEIL(output_rect_size.width() / 8), and a height of
  // CEIL(output_rect_size.height() / 2); because the chroma planes are half-
  // length in both dimensions in the I420 format.
  static gfx::Size GetChromaPlaneTextureSize(const gfx::Size& output_rect_size);

 private:
  DISALLOW_COPY_AND_ASSIGN(I420Converter);
};

// Similar to a ScalerInterface, a YUV readback pipeline will cache a scaler and
// all intermediate textures and frame buffers needed to scale, crop, letterbox
// and read back a texture from the GPU into CPU-accessible RAM. A single
// readback pipeline can handle multiple outstanding readbacks at the same time.
//
// TODO(crbug/754872): DEPRECATED. This will be removed soon, in favor of
// I420Converter and readback implementation in GLRendererCopier.
class VIZ_COMMON_EXPORT ReadbackYUVInterface {
 public:
  ReadbackYUVInterface() {}
  virtual ~ReadbackYUVInterface() {}

  // Optional behavior: This sets a scaler to use to scale the inputs before
  // planarizing. If null (or never called), then no scaling is performed.
  virtual void SetScaler(std::unique_ptr<GLHelper::ScalerInterface> scaler) = 0;

  // Returns the currently-set scaler, or null.
  virtual GLHelper::ScalerInterface* scaler() const = 0;

  // Returns true if the converter will vertically-flip the output.
  virtual bool IsFlippingOutput() const = 0;

  // Transforms a RGBA texture into I420 planar form, and then reads it back
  // from the GPU into system memory. See the GLHelper::ScalerInterface::Scale()
  // method comments for the meaning/semantics of |src_texture_size| and
  // |output_rect|. The process is:
  //
  //   1. Sync-wait and then consume and take ownership of the source texture
  //      provided by |mailbox|.
  //   2. Scale the source texture to an intermediate texture.
  //   3. Planarize, producing textures containing the Y, U, and V planes.
  //   4. Read-back the planar data, copying it into the given output
  //      destination. |paste_location| specifies the where to place the output
  //      pixels: Rect(paste_location.origin(), output_rect.size()).
  //   5. Run |callback| with true on success, false on failure (with no output
  //      modified).
  virtual void ReadbackYUV(const gpu::Mailbox& mailbox,
                           const gpu::SyncToken& sync_token,
                           const gfx::Size& src_texture_size,
                           const gfx::Rect& output_rect,
                           int y_plane_row_stride_bytes,
                           unsigned char* y_plane_data,
                           int u_plane_row_stride_bytes,
                           unsigned char* u_plane_data,
                           int v_plane_row_stride_bytes,
                           unsigned char* v_plane_data,
                           const gfx::Point& paste_location,
                           const base::Callback<void(bool)>& callback) = 0;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_GL_HELPER_H_
