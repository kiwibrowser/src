// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/gl_helper.h"

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "base/bind.h"
#include "base/containers/queue.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "components/viz/common/gl_helper_readback_support.h"
#include "components/viz/common/gl_helper_scaling.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "third_party/skia/include/gpu/GrTypes.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/vector2d.h"

using gpu::gles2::GLES2Interface;

namespace viz {

namespace {

class ScopedFlush {
 public:
  explicit ScopedFlush(gpu::gles2::GLES2Interface* gl) : gl_(gl) {}

  ~ScopedFlush() { gl_->Flush(); }

 private:
  gpu::gles2::GLES2Interface* gl_;

  DISALLOW_COPY_AND_ASSIGN(ScopedFlush);
};

// Helper class for allocating and holding an RGBA texture of a given
// size.
class TextureHolder {
 public:
  TextureHolder(GLES2Interface* gl, gfx::Size size)
      : texture_(gl), size_(size) {
    ScopedTextureBinder<GL_TEXTURE_2D> texture_binder(gl, texture_);
    gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width(), size.height(), 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  }

  GLuint texture() const { return texture_.id(); }
  gfx::Size size() const { return size_; }

 private:
  ScopedTexture texture_;
  gfx::Size size_;

  DISALLOW_COPY_AND_ASSIGN(TextureHolder);
};

class I420ConverterImpl : public I420Converter {
 public:
  I420ConverterImpl(GLES2Interface* gl,
                    GLHelperScaling* scaler_impl,
                    bool flipped_source,
                    bool flip_output,
                    bool swizzle,
                    bool use_mrt);

  ~I420ConverterImpl() override;

  void Convert(GLuint src_texture,
               const gfx::Size& src_texture_size,
               const gfx::Vector2dF& src_offset,
               GLHelper::ScalerInterface* optional_scaler,
               const gfx::Rect& output_rect,
               GLuint y_plane_texture,
               GLuint u_plane_texture,
               GLuint v_plane_texture) override;

  bool IsSamplingFlippedSource() const override;
  bool IsFlippingOutput() const override;
  GLenum GetReadbackFormat() const override;

 protected:
  // Returns true if the planerizer should use the faster, two-pass shaders to
  // generate the YUV planar outputs. If false, the source will be scanned three
  // times, once for each Y/U/V plane.
  bool use_mrt() const { return !v_planerizer_; }

  // Reallocates the intermediate and plane textures, if needed.
  void EnsureTexturesSizedFor(const gfx::Size& scaler_output_size,
                              const gfx::Size& y_texture_size,
                              const gfx::Size& chroma_texture_size,
                              GLuint y_plane_texture,
                              GLuint u_plane_texture,
                              GLuint v_plane_texture);

  GLES2Interface* const gl_;

 private:
  // These generate the Y/U/V planes. If MRT is being used, |y_planerizer_|
  // generates the Y and interim UV plane, |u_planerizer_| generates the final U
  // and V planes, and |v_planerizer_| is unused. If MRT is not being used, each
  // of these generates only one of the Y/U/V planes.
  const std::unique_ptr<GLHelper::ScalerInterface> y_planerizer_;
  const std::unique_ptr<GLHelper::ScalerInterface> u_planerizer_;
  const std::unique_ptr<GLHelper::ScalerInterface> v_planerizer_;

  // Intermediate texture, holding the scaler's output.
  base::Optional<TextureHolder> intermediate_;

  // Intermediate texture, holding the UV interim output (if the MRT shader is
  // being used).
  base::Optional<ScopedTexture> uv_;

  DISALLOW_COPY_AND_ASSIGN(I420ConverterImpl);
};

}  // namespace

typedef GLHelperReadbackSupport::FormatSupport FormatSupport;

// Implements GLHelper::CropScaleReadbackAndCleanTexture and encapsulates
// the data needed for it.
class GLHelper::CopyTextureToImpl
    : public base::SupportsWeakPtr<GLHelper::CopyTextureToImpl> {
 public:
  CopyTextureToImpl(GLES2Interface* gl,
                    gpu::ContextSupport* context_support,
                    GLHelper* helper)
      : gl_(gl),
        context_support_(context_support),
        helper_(helper),
        flush_(gl),
        max_draw_buffers_(0) {
    const GLubyte* extensions = gl_->GetString(GL_EXTENSIONS);
    if (!extensions)
      return;
    std::string extensions_string =
        " " + std::string(reinterpret_cast<const char*>(extensions)) + " ";
    if (extensions_string.find(" GL_EXT_draw_buffers ") != std::string::npos) {
      gl_->GetIntegerv(GL_MAX_DRAW_BUFFERS_EXT, &max_draw_buffers_);
    }
  }
  ~CopyTextureToImpl() { CancelRequests(); }

  GLuint ConsumeMailboxToTexture(const gpu::Mailbox& mailbox,
                                 const gpu::SyncToken& sync_token) {
    return helper_->ConsumeMailboxToTexture(mailbox, sync_token);
  }

  void CropScaleReadbackAndCleanTexture(
      GLuint src_texture,
      const gfx::Size& src_size,
      const gfx::Size& dst_size,
      unsigned char* out,
      const SkColorType out_color_type,
      const base::Callback<void(bool)>& callback,
      GLHelper::ScalerQuality quality);

  void ReadbackTextureSync(GLuint texture,
                           const gfx::Rect& src_rect,
                           unsigned char* out,
                           SkColorType format);

  void ReadbackTextureAsync(GLuint texture,
                            const gfx::Size& dst_size,
                            unsigned char* out,
                            SkColorType color_type,
                            const base::Callback<void(bool)>& callback);

  // Reads back bytes from the currently bound frame buffer.
  // Note that dst_size is specified in bytes, not pixels.
  void ReadbackAsync(const gfx::Size& dst_size,
                     size_t bytes_per_row,     // generally dst_size.width() * 4
                     size_t row_stride_bytes,  // generally dst_size.width() * 4
                     unsigned char* out,
                     GLenum format,
                     GLenum type,
                     size_t bytes_per_pixel,
                     const base::Callback<void(bool)>& callback);

  void ReadbackPlane(const gfx::Size& texture_size,
                     int row_stride_bytes,
                     unsigned char* data,
                     int size_shift,
                     const gfx::Rect& paste_rect,
                     ReadbackSwizzle swizzle,
                     const base::Callback<void(bool)>& callback);

  GLuint CopyAndScaleTexture(GLuint texture,
                             const gfx::Size& src_size,
                             const gfx::Size& dst_size,
                             bool vertically_flip_texture,
                             GLHelper::ScalerQuality quality);

  std::unique_ptr<ReadbackYUVInterface> CreateReadbackPipelineYUV(
      bool flip_vertically,
      bool use_mrt);

  // Returns the maximum number of draw buffers available,
  // 0 if GL_EXT_draw_buffers is not available.
  GLint MaxDrawBuffers() const { return max_draw_buffers_; }

  FormatSupport GetReadbackConfig(SkColorType color_type,
                                  bool can_swizzle,
                                  GLenum* format,
                                  GLenum* type,
                                  size_t* bytes_per_pixel);

 private:
  // A single request to CropScaleReadbackAndCleanTexture.
  // The main thread can cancel the request, before it's handled by the helper
  // thread, by resetting the texture and pixels fields. Alternatively, the
  // thread marks that it handles the request by resetting the pixels field
  // (meaning it guarantees that the callback with be called).
  // In either case, the callback must be called exactly once, and the texture
  // must be deleted by the main thread gl.
  struct Request {
    Request(const gfx::Size& size_,
            size_t bytes_per_row_,
            size_t row_stride_bytes_,
            unsigned char* pixels_,
            const base::Callback<void(bool)>& callback_)
        : done(false),
          size(size_),
          bytes_per_row(bytes_per_row_),
          row_stride_bytes(row_stride_bytes_),
          pixels(pixels_),
          callback(callback_),
          buffer(0),
          query(0) {}

    bool done;
    bool result;
    gfx::Size size;
    size_t bytes_per_row;
    size_t row_stride_bytes;
    unsigned char* pixels;
    base::Callback<void(bool)> callback;
    GLuint buffer;
    GLuint query;
  };

  // We must take care to call the callbacks last, as they may
  // end up destroying the gl_helper and make *this invalid.
  // We stick the finished requests in a stack object that calls
  // the callbacks when it goes out of scope.
  class FinishRequestHelper {
   public:
    FinishRequestHelper() {}
    ~FinishRequestHelper() {
      while (!requests_.empty()) {
        Request* request = requests_.front();
        requests_.pop();
        request->callback.Run(request->result);
        delete request;
      }
    }
    void Add(Request* r) { requests_.push(r); }

   private:
    base::queue<Request*> requests_;
    DISALLOW_COPY_AND_ASSIGN(FinishRequestHelper);
  };

  // A readback pipeline that also converts the data to YUV before
  // reading it back.
  class ReadbackYUVImpl : public I420ConverterImpl,
                          public ReadbackYUVInterface {
   public:
    ReadbackYUVImpl(GLES2Interface* gl,
                    CopyTextureToImpl* copy_impl,
                    GLHelperScaling* scaler_impl,
                    bool flip_vertically,
                    ReadbackSwizzle swizzle,
                    bool use_mrt);

    ~ReadbackYUVImpl() override;

    void SetScaler(std::unique_ptr<GLHelper::ScalerInterface> scaler) override;

    GLHelper::ScalerInterface* scaler() const override;

    bool IsFlippingOutput() const override;

    void ReadbackYUV(const gpu::Mailbox& mailbox,
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
                     const base::Callback<void(bool)>& callback) override;

   private:
    GLES2Interface* gl_;
    CopyTextureToImpl* copy_impl_;
    ReadbackSwizzle swizzle_;

    // May be null if no scaling is required. This can be changed between calls
    // to ReadbackYUV().
    std::unique_ptr<GLHelper::ScalerInterface> scaler_;

    // These are the output textures for each Y/U/V plane.
    ScopedTexture y_;
    ScopedTexture u_;
    ScopedTexture v_;

    // Framebuffers used by ReadbackPlane(). They are cached here so as to not
    // be re-allocated for every frame of video.
    ScopedFramebuffer y_readback_framebuffer_;
    ScopedFramebuffer u_readback_framebuffer_;
    ScopedFramebuffer v_readback_framebuffer_;

    DISALLOW_COPY_AND_ASSIGN(ReadbackYUVImpl);
  };

  // Copies everything from |src_texture| (of size |src_size|),
  // scales it to |dst_size|, writes it into a texture, and returns its ID.
  // |src_size| is the size of |src_texture|.
  GLuint ScaleTexture(GLuint src_texture,
                      const gfx::Size& src_size,
                      const gfx::Size& dst_size,
                      bool vertically_flip_texture,
                      bool swizzle,
                      SkColorType color_type,
                      GLHelper::ScalerQuality quality);

  // Converts each four consecutive pixels of the source texture into one pixel
  // in the result texture with each pixel channel representing the grayscale
  // color of one of the four original pixels:
  // R1G1B1A1 R2G2B2A2 R3G3B3A3 R4G4B4A4 -> X1X2X3X4
  // The resulting texture is still an RGBA texture (which is ~4 times narrower
  // than the original). If rendered directly, it wouldn't show anything useful,
  // but the data in it can be used to construct a grayscale image.
  // |encoded_texture_size| is the exact size of the resulting RGBA texture. It
  // is equal to src_size.width()/4 rounded upwards. Some channels in the last
  // pixel ((-src_size.width()) % 4) to be exact) are padding and don't contain
  // useful data.
  // If swizzle is set to true, the transformed pixels are reordered:
  // R1G1B1A1 R2G2B2A2 R3G3B3A3 R4G4B4A4 -> X3X2X1X4.
  GLuint EncodeTextureAsGrayscale(GLuint src_texture,
                                  const gfx::Size& src_size,
                                  gfx::Size* const encoded_texture_size,
                                  bool vertically_flip_texture,
                                  bool swizzle);

  static void nullcallback(bool success) {}
  void ReadbackDone(Request* request, size_t bytes_per_pixel);
  void FinishRequest(Request* request,
                     bool result,
                     FinishRequestHelper* helper);
  void CancelRequests();

  GLES2Interface* gl_;
  gpu::ContextSupport* context_support_;
  GLHelper* helper_;

  // A scoped flush that will ensure all resource deletions are flushed when
  // this object is destroyed. Must be declared before other Scoped* fields.
  ScopedFlush flush_;

  base::queue<Request*> request_queue_;
  GLint max_draw_buffers_;
};

std::unique_ptr<GLHelper::ScalerInterface> GLHelper::CreateScaler(
    ScalerQuality quality,
    const gfx::Vector2d& scale_from,
    const gfx::Vector2d& scale_to,
    bool flipped_source,
    bool flip_output,
    bool swizzle) {
  InitScalerImpl();
  return scaler_impl_->CreateScaler(quality, scale_from, scale_to,
                                    flipped_source, flip_output, swizzle);
}

GLuint GLHelper::CopyTextureToImpl::ScaleTexture(
    GLuint src_texture,
    const gfx::Size& src_size,
    const gfx::Size& dst_size,
    bool vertically_flip_texture,
    bool swizzle,
    SkColorType color_type,
    GLHelper::ScalerQuality quality) {
  if (src_size.IsEmpty() || dst_size.IsEmpty())
    return 0;

  GLuint dst_texture = 0u;
  gl_->GenTextures(1, &dst_texture);
  {
    GLenum format = GL_RGBA, type = GL_UNSIGNED_BYTE;
    ScopedTextureBinder<GL_TEXTURE_2D> texture_binder(gl_, dst_texture);

    // Use GL_RGBA for destination/temporary texture unless we're working with
    // 16-bit data
    if (color_type == kRGB_565_SkColorType) {
      format = GL_RGB;
      type = GL_UNSIGNED_SHORT_5_6_5;
    }
    gl_->TexImage2D(GL_TEXTURE_2D, 0, format, dst_size.width(),
                    dst_size.height(), 0, format, type, nullptr);
  }

  const std::unique_ptr<ScalerInterface> scaler = helper_->CreateScaler(
      quality, gfx::Vector2d(src_size.width(), src_size.height()),
      gfx::Vector2d(dst_size.width(), dst_size.height()), false,
      vertically_flip_texture, swizzle);
  scaler->Scale(src_texture, src_size, gfx::Vector2dF(), dst_texture,
                gfx::Rect(dst_size));
  return dst_texture;
}

GLuint GLHelper::CopyTextureToImpl::EncodeTextureAsGrayscale(
    GLuint src_texture,
    const gfx::Size& src_size,
    gfx::Size* const encoded_texture_size,
    bool vertically_flip_texture,
    bool swizzle) {
  GLuint dst_texture = 0u;
  gl_->GenTextures(1, &dst_texture);
  // The size of the encoded texture.
  *encoded_texture_size =
      gfx::Size((src_size.width() + 3) / 4, src_size.height());
  {
    ScopedTextureBinder<GL_TEXTURE_2D> texture_binder(gl_, dst_texture);
    gl_->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, encoded_texture_size->width(),
                    encoded_texture_size->height(), 0, GL_RGBA,
                    GL_UNSIGNED_BYTE, nullptr);
  }

  helper_->InitScalerImpl();
  const std::unique_ptr<ScalerInterface> planerizer =
      helper_->scaler_impl_.get()->CreateGrayscalePlanerizer(
          false, vertically_flip_texture, swizzle);
  planerizer->Scale(src_texture, src_size, gfx::Vector2dF(), dst_texture,
                    gfx::Rect(*encoded_texture_size));
  return dst_texture;
}

void GLHelper::CopyTextureToImpl::ReadbackAsync(
    const gfx::Size& dst_size,
    size_t bytes_per_row,
    size_t row_stride_bytes,
    unsigned char* out,
    GLenum format,
    GLenum type,
    size_t bytes_per_pixel,
    const base::Callback<void(bool)>& callback) {
  TRACE_EVENT0("gpu.capture", "GLHelper::CopyTextureToImpl::ReadbackAsync");
  Request* request =
      new Request(dst_size, bytes_per_row, row_stride_bytes, out, callback);
  request_queue_.push(request);
  request->buffer = 0u;

  gl_->GenBuffers(1, &request->buffer);
  gl_->BindBuffer(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, request->buffer);
  gl_->BufferData(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM,
                  bytes_per_pixel * dst_size.GetArea(), nullptr,
                  GL_STREAM_READ);

  request->query = 0u;
  gl_->GenQueriesEXT(1, &request->query);
  gl_->BeginQueryEXT(GL_ASYNC_PIXEL_PACK_COMPLETED_CHROMIUM, request->query);
  gl_->ReadPixels(0, 0, dst_size.width(), dst_size.height(), format, type,
                  nullptr);
  gl_->EndQueryEXT(GL_ASYNC_PIXEL_PACK_COMPLETED_CHROMIUM);
  gl_->BindBuffer(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, 0);
  context_support_->SignalQuery(
      request->query, base::Bind(&CopyTextureToImpl::ReadbackDone, AsWeakPtr(),
                                 request, bytes_per_pixel));
}

void GLHelper::CopyTextureToImpl::CropScaleReadbackAndCleanTexture(
    GLuint src_texture,
    const gfx::Size& src_size,
    const gfx::Size& dst_size,
    unsigned char* out,
    const SkColorType out_color_type,
    const base::Callback<void(bool)>& callback,
    GLHelper::ScalerQuality quality) {
  GLenum format, type;
  size_t bytes_per_pixel;
  SkColorType readback_color_type = out_color_type;
  // Single-component textures are not supported by all GPUs, so  we implement
  // kAlpha_8_SkColorType support here via a special encoding (see below) using
  // a 32-bit texture to represent an 8-bit image.
  // Thus we use generic 32-bit readback in this case.
  if (out_color_type == kAlpha_8_SkColorType) {
    readback_color_type = kRGBA_8888_SkColorType;
  }

  FormatSupport supported = GetReadbackConfig(readback_color_type, true,
                                              &format, &type, &bytes_per_pixel);

  if (supported == GLHelperReadbackSupport::NOT_SUPPORTED) {
    callback.Run(false);
    return;
  }

  GLuint texture = src_texture;

  // Scale texture if needed
  // Optimization: SCALER_QUALITY_FAST is just a single bilinear pass, which we
  // can do just as well in EncodeTextureAsGrayscale, which we will do if
  // out_color_type is kAlpha_8_SkColorType, so let's skip the scaling step
  // in that case.
  bool scale_texture = out_color_type != kAlpha_8_SkColorType ||
                       quality != GLHelper::SCALER_QUALITY_FAST;
  if (scale_texture) {
    // Don't swizzle during the scale step for kAlpha_8_SkColorType.
    // We will swizzle in the encode step below if needed.
    bool scale_swizzle = out_color_type == kAlpha_8_SkColorType
                             ? false
                             : supported == GLHelperReadbackSupport::SWIZZLE;
    texture =
        ScaleTexture(src_texture, src_size, dst_size, true, scale_swizzle,
                     out_color_type == kAlpha_8_SkColorType ? kN32_SkColorType
                                                            : out_color_type,
                     quality);
    if (!texture) {
      callback.Run(false);
      return;
    }
  }

  gfx::Size readback_texture_size = dst_size;
  // Encode texture to grayscale if needed.
  if (out_color_type == kAlpha_8_SkColorType) {
    // Do the vertical flip here if we haven't already done it when we scaled
    // the texture.
    bool encode_as_grayscale_vertical_flip = !scale_texture;
    // EncodeTextureAsGrayscale by default creates a texture which should be
    // read back as RGBA, so need to swizzle if the readback format is BGRA.
    bool encode_as_grayscale_swizzle = format == GL_BGRA_EXT;
    GLuint tmp_texture = EncodeTextureAsGrayscale(
        texture, dst_size, &readback_texture_size,
        encode_as_grayscale_vertical_flip, encode_as_grayscale_swizzle);
    // If the scaled texture was created - delete it
    if (scale_texture)
      gl_->DeleteTextures(1, &texture);
    texture = tmp_texture;
    DCHECK(texture);
  }

  // Readback the pixels of the resulting texture
  ScopedFramebuffer dst_framebuffer(gl_);
  ScopedFramebufferBinder<GL_FRAMEBUFFER> framebuffer_binder(gl_,
                                                             dst_framebuffer);
  ScopedTextureBinder<GL_TEXTURE_2D> texture_binder(gl_, texture);
  gl_->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            texture, 0);

  size_t bytes_per_row = out_color_type == kAlpha_8_SkColorType
                             ? dst_size.width()
                             : dst_size.width() * bytes_per_pixel;

  ReadbackAsync(readback_texture_size, bytes_per_row, bytes_per_row, out,
                format, type, bytes_per_pixel, callback);
  gl_->DeleteTextures(1, &texture);
}

void GLHelper::CopyTextureToImpl::ReadbackTextureSync(GLuint texture,
                                                      const gfx::Rect& src_rect,
                                                      unsigned char* out,
                                                      SkColorType color_type) {
  GLenum format, type;
  size_t bytes_per_pixel;
  FormatSupport supported =
      GetReadbackConfig(color_type, false, &format, &type, &bytes_per_pixel);
  if (supported == GLHelperReadbackSupport::NOT_SUPPORTED) {
    return;
  }

  ScopedFramebuffer dst_framebuffer(gl_);
  ScopedFramebufferBinder<GL_FRAMEBUFFER> framebuffer_binder(gl_,
                                                             dst_framebuffer);
  ScopedTextureBinder<GL_TEXTURE_2D> texture_binder(gl_, texture);
  gl_->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            texture, 0);
  gl_->ReadPixels(src_rect.x(), src_rect.y(), src_rect.width(),
                  src_rect.height(), format, type, out);
}

void GLHelper::CopyTextureToImpl::ReadbackTextureAsync(
    GLuint texture,
    const gfx::Size& dst_size,
    unsigned char* out,
    SkColorType color_type,
    const base::Callback<void(bool)>& callback) {
  GLenum format, type;
  size_t bytes_per_pixel;
  FormatSupport supported =
      GetReadbackConfig(color_type, false, &format, &type, &bytes_per_pixel);
  if (supported == GLHelperReadbackSupport::NOT_SUPPORTED) {
    callback.Run(false);
    return;
  }

  ScopedFramebuffer dst_framebuffer(gl_);
  ScopedFramebufferBinder<GL_FRAMEBUFFER> framebuffer_binder(gl_,
                                                             dst_framebuffer);
  ScopedTextureBinder<GL_TEXTURE_2D> texture_binder(gl_, texture);
  gl_->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            texture, 0);
  ReadbackAsync(dst_size, dst_size.width() * bytes_per_pixel,
                dst_size.width() * bytes_per_pixel, out, format, type,
                bytes_per_pixel, callback);
}

GLuint GLHelper::CopyTextureToImpl::CopyAndScaleTexture(
    GLuint src_texture,
    const gfx::Size& src_size,
    const gfx::Size& dst_size,
    bool vertically_flip_texture,
    GLHelper::ScalerQuality quality) {
  return ScaleTexture(src_texture, src_size, dst_size, vertically_flip_texture,
                      false, kRGBA_8888_SkColorType, quality);
}

void GLHelper::CopyTextureToImpl::ReadbackDone(Request* finished_request,
                                               size_t bytes_per_pixel) {
  TRACE_EVENT0("gpu.capture",
               "GLHelper::CopyTextureToImpl::CheckReadbackFramebufferComplete");
  finished_request->done = true;

  FinishRequestHelper finish_request_helper;

  // We process transfer requests in the order they were received, regardless
  // of the order we get the callbacks in.
  while (!request_queue_.empty()) {
    Request* request = request_queue_.front();
    if (!request->done) {
      break;
    }

    bool result = false;
    if (request->buffer != 0) {
      gl_->BindBuffer(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, request->buffer);
      unsigned char* data = static_cast<unsigned char*>(gl_->MapBufferCHROMIUM(
          GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, GL_READ_ONLY));
      if (data) {
        result = true;
        if (request->bytes_per_row == request->size.width() * bytes_per_pixel &&
            request->bytes_per_row == request->row_stride_bytes) {
          memcpy(request->pixels, data,
                 request->size.GetArea() * bytes_per_pixel);
        } else {
          unsigned char* out = request->pixels;
          for (int y = 0; y < request->size.height(); y++) {
            memcpy(out, data, request->bytes_per_row);
            out += request->row_stride_bytes;
            data += request->size.width() * bytes_per_pixel;
          }
        }
        gl_->UnmapBufferCHROMIUM(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM);
      }
      gl_->BindBuffer(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, 0);
    }
    FinishRequest(request, result, &finish_request_helper);
  }
}

void GLHelper::CopyTextureToImpl::FinishRequest(
    Request* request,
    bool result,
    FinishRequestHelper* finish_request_helper) {
  TRACE_EVENT0("gpu.capture", "GLHelper::CopyTextureToImpl::FinishRequest");
  DCHECK(request_queue_.front() == request);
  request_queue_.pop();
  request->result = result;
  ScopedFlush flush(gl_);
  if (request->query != 0) {
    gl_->DeleteQueriesEXT(1, &request->query);
    request->query = 0;
  }
  if (request->buffer != 0) {
    gl_->DeleteBuffers(1, &request->buffer);
    request->buffer = 0;
  }
  finish_request_helper->Add(request);
}

void GLHelper::CopyTextureToImpl::CancelRequests() {
  FinishRequestHelper finish_request_helper;
  while (!request_queue_.empty()) {
    Request* request = request_queue_.front();
    FinishRequest(request, false, &finish_request_helper);
  }
}

FormatSupport GLHelper::CopyTextureToImpl::GetReadbackConfig(
    SkColorType color_type,
    bool can_swizzle,
    GLenum* format,
    GLenum* type,
    size_t* bytes_per_pixel) {
  helper_->LazyInitReadbackSupportImpl();
  return helper_->readback_support_->GetReadbackConfig(
      color_type, can_swizzle, format, type, bytes_per_pixel);
}

GLHelper::GLHelper(GLES2Interface* gl, gpu::ContextSupport* context_support)
    : gl_(gl), context_support_(context_support) {}

GLHelper::~GLHelper() {}

void GLHelper::CropScaleReadbackAndCleanTexture(
    GLuint src_texture,
    const gfx::Size& src_size,
    const gfx::Size& dst_size,
    unsigned char* out,
    const SkColorType out_color_type,
    const base::Callback<void(bool)>& callback,
    GLHelper::ScalerQuality quality) {
  InitCopyTextToImpl();
  copy_texture_to_impl_->CropScaleReadbackAndCleanTexture(
      src_texture, src_size, dst_size, out, out_color_type, callback, quality);
}

void GLHelper::CropScaleReadbackAndCleanMailbox(
    const gpu::Mailbox& src_mailbox,
    const gpu::SyncToken& sync_token,
    const gfx::Size& src_size,
    const gfx::Size& dst_size,
    unsigned char* out,
    const SkColorType out_color_type,
    const base::Callback<void(bool)>& callback,
    GLHelper::ScalerQuality quality) {
  GLuint mailbox_texture = ConsumeMailboxToTexture(src_mailbox, sync_token);
  CropScaleReadbackAndCleanTexture(mailbox_texture, src_size, dst_size, out,
                                   out_color_type, callback, quality);
  gl_->DeleteTextures(1, &mailbox_texture);
}

void GLHelper::ReadbackTextureSync(GLuint texture,
                                   const gfx::Rect& src_rect,
                                   unsigned char* out,
                                   SkColorType format) {
  InitCopyTextToImpl();
  copy_texture_to_impl_->ReadbackTextureSync(texture, src_rect, out, format);
}

void GLHelper::ReadbackTextureAsync(
    GLuint texture,
    const gfx::Size& dst_size,
    unsigned char* out,
    SkColorType color_type,
    const base::Callback<void(bool)>& callback) {
  InitCopyTextToImpl();
  copy_texture_to_impl_->ReadbackTextureAsync(texture, dst_size, out,
                                              color_type, callback);
}

GLuint GLHelper::CopyAndScaleTexture(GLuint texture,
                                     const gfx::Size& src_size,
                                     const gfx::Size& dst_size,
                                     bool vertically_flip_texture,
                                     ScalerQuality quality) {
  InitCopyTextToImpl();
  return copy_texture_to_impl_->CopyAndScaleTexture(
      texture, src_size, dst_size, vertically_flip_texture, quality);
}

GLuint GLHelper::CompileShaderFromSource(const GLchar* source, GLenum type) {
  GLuint shader = gl_->CreateShader(type);
  GLint length = base::checked_cast<GLint>(strlen(source));
  gl_->ShaderSource(shader, 1, &source, &length);
  gl_->CompileShader(shader);
  GLint compile_status = 0;
  gl_->GetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
  if (!compile_status) {
    GLint log_length = 0;
    gl_->GetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length) {
      std::unique_ptr<GLchar[]> log(new GLchar[log_length]);
      GLsizei returned_log_length = 0;
      gl_->GetShaderInfoLog(shader, log_length, &returned_log_length,
                            log.get());
      LOG(ERROR) << std::string(log.get(), returned_log_length);
    }
    gl_->DeleteShader(shader);
    return 0;
  }
  return shader;
}

void GLHelper::InitCopyTextToImpl() {
  // Lazily initialize |copy_texture_to_impl_|
  if (!copy_texture_to_impl_)
    copy_texture_to_impl_.reset(
        new CopyTextureToImpl(gl_, context_support_, this));
}

void GLHelper::InitScalerImpl() {
  // Lazily initialize |scaler_impl_|
  if (!scaler_impl_)
    scaler_impl_.reset(new GLHelperScaling(gl_, this));
}

void GLHelper::LazyInitReadbackSupportImpl() {
  // Lazily initialize |readback_support_|.
  if (!readback_support_)
    readback_support_.reset(new GLHelperReadbackSupport(gl_));
}

GLint GLHelper::MaxDrawBuffers() {
  InitCopyTextToImpl();
  return copy_texture_to_impl_->MaxDrawBuffers();
}

void GLHelper::CopySubBufferDamage(GLenum target,
                                   GLuint texture,
                                   GLuint previous_texture,
                                   const SkRegion& new_damage,
                                   const SkRegion& old_damage) {
  SkRegion region(old_damage);
  if (region.op(new_damage, SkRegion::kDifference_Op)) {
    ScopedFramebuffer dst_framebuffer(gl_);
    ScopedFramebufferBinder<GL_FRAMEBUFFER> framebuffer_binder(gl_,
                                                               dst_framebuffer);
    gl_->BindTexture(target, texture);
    gl_->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target,
                              previous_texture, 0);
    for (SkRegion::Iterator it(region); !it.done(); it.next()) {
      const SkIRect& rect = it.rect();
      gl_->CopyTexSubImage2D(target, 0, rect.x(), rect.y(), rect.x(), rect.y(),
                             rect.width(), rect.height());
    }
    gl_->BindTexture(target, 0);
    gl_->Flush();
  }
}

GLuint GLHelper::CreateTexture() {
  GLuint texture = 0u;
  gl_->GenTextures(1, &texture);
  ScopedTextureBinder<GL_TEXTURE_2D> texture_binder(gl_, texture);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  return texture;
}

void GLHelper::DeleteTexture(GLuint texture_id) {
  gl_->DeleteTextures(1, &texture_id);
}

void GLHelper::GenerateSyncToken(gpu::SyncToken* sync_token) {
  gl_->GenSyncTokenCHROMIUM(sync_token->GetData());
}

void GLHelper::WaitSyncToken(const gpu::SyncToken& sync_token) {
  gl_->WaitSyncTokenCHROMIUM(sync_token.GetConstData());
}

gpu::MailboxHolder GLHelper::ProduceMailboxHolderFromTexture(
    GLuint texture_id) {
  gpu::Mailbox mailbox;
  gl_->GenMailboxCHROMIUM(mailbox.name);
  gl_->ProduceTextureDirectCHROMIUM(texture_id, mailbox.name);

  gpu::SyncToken sync_token;
  GenerateSyncToken(&sync_token);

  return gpu::MailboxHolder(mailbox, sync_token, GL_TEXTURE_2D);
}

GLuint GLHelper::ConsumeMailboxToTexture(const gpu::Mailbox& mailbox,
                                         const gpu::SyncToken& sync_token) {
  if (mailbox.IsZero())
    return 0;
  if (sync_token.HasData())
    WaitSyncToken(sync_token);
  GLuint texture = gl_->CreateAndConsumeTextureCHROMIUM(mailbox.name);
  return texture;
}

void GLHelper::ResizeTexture(GLuint texture, const gfx::Size& size) {
  ScopedTextureBinder<GL_TEXTURE_2D> texture_binder(gl_, texture);
  gl_->TexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.width(), size.height(), 0,
                  GL_RGB, GL_UNSIGNED_BYTE, nullptr);
}

void GLHelper::CopyTextureSubImage(GLuint texture, const gfx::Rect& rect) {
  ScopedTextureBinder<GL_TEXTURE_2D> texture_binder(gl_, texture);
  gl_->CopyTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.x(),
                         rect.y(), rect.width(), rect.height());
}

void GLHelper::CopyTextureFullImage(GLuint texture, const gfx::Size& size) {
  ScopedTextureBinder<GL_TEXTURE_2D> texture_binder(gl_, texture);
  gl_->CopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, size.width(),
                      size.height(), 0);
}

void GLHelper::Flush() {
  gl_->Flush();
}

void GLHelper::InsertOrderingBarrier() {
  gl_->OrderingBarrierCHROMIUM();
}

void GLHelper::CopyTextureToImpl::ReadbackPlane(
    const gfx::Size& texture_size,
    int row_stride_bytes,
    unsigned char* data,
    int size_shift,
    const gfx::Rect& paste_rect,
    ReadbackSwizzle swizzle,
    const base::Callback<void(bool)>& callback) {
  const size_t offset = row_stride_bytes * (paste_rect.y() >> size_shift) +
                        (paste_rect.x() >> size_shift);
  ReadbackAsync(texture_size, paste_rect.width() >> size_shift,
                row_stride_bytes, data + offset,
                (swizzle == kSwizzleBGRA) ? GL_BGRA_EXT : GL_RGBA,
                GL_UNSIGNED_BYTE, 4, callback);
}

I420Converter::I420Converter() = default;
I420Converter::~I420Converter() = default;

// static
gfx::Size I420Converter::GetYPlaneTextureSize(const gfx::Size& output_size) {
  return gfx::Size((output_size.width() + 3) / 4, output_size.height());
}

// static
gfx::Size I420Converter::GetChromaPlaneTextureSize(
    const gfx::Size& output_size) {
  return gfx::Size((output_size.width() + 7) / 8,
                   (output_size.height() + 1) / 2);
}

namespace {

I420ConverterImpl::I420ConverterImpl(GLES2Interface* gl,
                                     GLHelperScaling* scaler_impl,
                                     bool flipped_source,
                                     bool flip_output,
                                     bool swizzle,
                                     bool use_mrt)
    : gl_(gl),
      y_planerizer_(
          use_mrt ? scaler_impl->CreateI420MrtPass1Planerizer(flipped_source,
                                                              flip_output,
                                                              swizzle)
                  : scaler_impl->CreateI420Planerizer(0,
                                                      flipped_source,
                                                      flip_output,
                                                      swizzle)),
      u_planerizer_(use_mrt ? scaler_impl->CreateI420MrtPass2Planerizer(swizzle)
                            : scaler_impl->CreateI420Planerizer(1,
                                                                flipped_source,
                                                                flip_output,
                                                                swizzle)),
      v_planerizer_(use_mrt ? nullptr
                            : scaler_impl->CreateI420Planerizer(2,
                                                                flipped_source,
                                                                flip_output,
                                                                swizzle)) {}

I420ConverterImpl::~I420ConverterImpl() = default;

void I420ConverterImpl::Convert(GLuint src_texture,
                                const gfx::Size& src_texture_size,
                                const gfx::Vector2dF& src_offset,
                                GLHelper::ScalerInterface* optional_scaler,
                                const gfx::Rect& output_rect,
                                GLuint y_plane_texture,
                                GLuint u_plane_texture,
                                GLuint v_plane_texture) {
  const gfx::Size scaler_output_size =
      optional_scaler ? output_rect.size() : gfx::Size();
  const gfx::Size y_texture_size = GetYPlaneTextureSize(output_rect.size());
  const gfx::Size chroma_texture_size =
      GetChromaPlaneTextureSize(output_rect.size());
  EnsureTexturesSizedFor(scaler_output_size, y_texture_size,
                         chroma_texture_size, y_plane_texture, u_plane_texture,
                         v_plane_texture);

  // Scale first, if needed.
  if (optional_scaler) {
    // The scaler should not be configured to do any swizzling.
    DCHECK_EQ(optional_scaler->GetReadbackFormat(),
              static_cast<GLenum>(GL_RGBA));
    optional_scaler->Scale(src_texture, src_texture_size, src_offset,
                           intermediate_->texture(), output_rect);
  }

  // Convert the intermediate (or source) texture into Y, U and V planes.
  const GLuint texture =
      optional_scaler ? intermediate_->texture() : src_texture;
  const gfx::Size texture_size =
      optional_scaler ? intermediate_->size() : src_texture_size;
  const gfx::Vector2dF offset = optional_scaler ? gfx::Vector2dF() : src_offset;
  if (use_mrt()) {
    y_planerizer_->ScaleToMultipleOutputs(texture, texture_size, offset,
                                          y_plane_texture, uv_->id(),
                                          gfx::Rect(y_texture_size));
    u_planerizer_->ScaleToMultipleOutputs(
        uv_->id(), y_texture_size, gfx::Vector2dF(), u_plane_texture,
        v_plane_texture, gfx::Rect(chroma_texture_size));
  } else {
    y_planerizer_->Scale(texture, texture_size, offset, y_plane_texture,
                         gfx::Rect(y_texture_size));
    u_planerizer_->Scale(texture, texture_size, offset, u_plane_texture,
                         gfx::Rect(chroma_texture_size));
    v_planerizer_->Scale(texture, texture_size, offset, v_plane_texture,
                         gfx::Rect(chroma_texture_size));
  }
}

bool I420ConverterImpl::IsSamplingFlippedSource() const {
  return y_planerizer_->IsSamplingFlippedSource();
}

bool I420ConverterImpl::IsFlippingOutput() const {
  return y_planerizer_->IsFlippingOutput();
}

GLenum I420ConverterImpl::GetReadbackFormat() const {
  return y_planerizer_->GetReadbackFormat();
}

void I420ConverterImpl::EnsureTexturesSizedFor(
    const gfx::Size& scaler_output_size,
    const gfx::Size& y_texture_size,
    const gfx::Size& chroma_texture_size,
    GLuint y_plane_texture,
    GLuint u_plane_texture,
    GLuint v_plane_texture) {
  // Reallocate the intermediate texture, if needed.
  if (!scaler_output_size.IsEmpty()) {
    if (!intermediate_ || intermediate_->size() != scaler_output_size)
      intermediate_.emplace(gl_, scaler_output_size);
  } else {
    intermediate_ = base::nullopt;
  }

  // Size the interim UV plane and the three output planes.
  const auto SetRGBATextureSize = [this](const gfx::Size& size) {
    gl_->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width(), size.height(), 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  };
  if (use_mrt()) {
    uv_.emplace(gl_);
    gl_->BindTexture(GL_TEXTURE_2D, uv_->id());
    SetRGBATextureSize(y_texture_size);
  }
  gl_->BindTexture(GL_TEXTURE_2D, y_plane_texture);
  SetRGBATextureSize(y_texture_size);
  gl_->BindTexture(GL_TEXTURE_2D, u_plane_texture);
  SetRGBATextureSize(chroma_texture_size);
  gl_->BindTexture(GL_TEXTURE_2D, v_plane_texture);
  SetRGBATextureSize(chroma_texture_size);
}

}  // namespace

GLHelper::CopyTextureToImpl::ReadbackYUVImpl::ReadbackYUVImpl(
    GLES2Interface* gl,
    CopyTextureToImpl* copy_impl,
    GLHelperScaling* scaler_impl,
    bool flip_vertically,
    ReadbackSwizzle swizzle,
    bool use_mrt)
    : I420ConverterImpl(gl,
                        scaler_impl,
                        false,
                        flip_vertically,
                        swizzle == kSwizzleBGRA,
                        use_mrt),
      gl_(gl),
      copy_impl_(copy_impl),
      swizzle_(swizzle),
      y_(gl_),
      u_(gl_),
      v_(gl_),
      y_readback_framebuffer_(gl_),
      u_readback_framebuffer_(gl_),
      v_readback_framebuffer_(gl_) {}

GLHelper::CopyTextureToImpl::ReadbackYUVImpl::~ReadbackYUVImpl() = default;

void GLHelper::CopyTextureToImpl::ReadbackYUVImpl::SetScaler(
    std::unique_ptr<GLHelper::ScalerInterface> scaler) {
  scaler_ = std::move(scaler);
}

GLHelper::ScalerInterface*
GLHelper::CopyTextureToImpl::ReadbackYUVImpl::scaler() const {
  return scaler_.get();
}

bool GLHelper::CopyTextureToImpl::ReadbackYUVImpl::IsFlippingOutput() const {
  return I420ConverterImpl::IsFlippingOutput();
}

void GLHelper::CopyTextureToImpl::ReadbackYUVImpl::ReadbackYUV(
    const gpu::Mailbox& mailbox,
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
    const base::Callback<void(bool)>& callback) {
  DCHECK(!(paste_location.x() & 1));
  DCHECK(!(paste_location.y() & 1));

  GLuint mailbox_texture =
      copy_impl_->ConsumeMailboxToTexture(mailbox, sync_token);
  I420ConverterImpl::Convert(mailbox_texture, src_texture_size,
                             gfx::Vector2dF(), scaler_.get(), output_rect, y_,
                             u_, v_);
  gl_->DeleteTextures(1, &mailbox_texture);

  // Read back planes, one at a time. Keep the video frame alive while doing the
  // readback.
  const gfx::Rect paste_rect(paste_location, output_rect.size());
  const auto SetUpAndBindFramebuffer = [this](GLuint framebuffer,
                                              GLuint texture) {
    gl_->BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    gl_->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, texture, 0);
  };
  SetUpAndBindFramebuffer(y_readback_framebuffer_, y_);
  copy_impl_->ReadbackPlane(GetYPlaneTextureSize(output_rect.size()),
                            y_plane_row_stride_bytes, y_plane_data, 0,
                            paste_rect, swizzle_, base::Bind(&nullcallback));
  SetUpAndBindFramebuffer(u_readback_framebuffer_, u_);
  const gfx::Size chroma_texture_size =
      GetChromaPlaneTextureSize(output_rect.size());
  copy_impl_->ReadbackPlane(chroma_texture_size, u_plane_row_stride_bytes,
                            u_plane_data, 1, paste_rect, swizzle_,
                            base::Bind(&nullcallback));
  SetUpAndBindFramebuffer(v_readback_framebuffer_, v_);
  copy_impl_->ReadbackPlane(chroma_texture_size, v_plane_row_stride_bytes,
                            v_plane_data, 1, paste_rect, swizzle_, callback);
  gl_->BindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool GLHelper::IsReadbackConfigSupported(SkColorType color_type) {
  LazyInitReadbackSupportImpl();
  GLenum format, type;
  size_t bytes_per_pixel;
  FormatSupport support = readback_support_->GetReadbackConfig(
      color_type, false, &format, &type, &bytes_per_pixel);

  return (support == GLHelperReadbackSupport::SUPPORTED);
}

std::unique_ptr<I420Converter> GLHelper::CreateI420Converter(
    bool flipped_source,
    bool flip_output,
    bool swizzle,
    bool use_mrt) {
  InitCopyTextToImpl();
  InitScalerImpl();
  return std::make_unique<I420ConverterImpl>(
      gl_, scaler_impl_.get(), flipped_source, flip_output, swizzle,
      use_mrt && (copy_texture_to_impl_->MaxDrawBuffers() >= 2));
}

std::unique_ptr<ReadbackYUVInterface>
GLHelper::CopyTextureToImpl::CreateReadbackPipelineYUV(bool flip_vertically,
                                                       bool use_mrt) {
  helper_->InitScalerImpl();
  // Just query if the best readback configuration needs a swizzle In
  // ReadbackPlane() we will choose GL_RGBA/GL_BGRA_EXT based on swizzle
  GLenum format, type;
  size_t bytes_per_pixel;
  FormatSupport supported = GetReadbackConfig(kRGBA_8888_SkColorType, true,
                                              &format, &type, &bytes_per_pixel);
  DCHECK((format == GL_RGBA || format == GL_BGRA_EXT) &&
         type == GL_UNSIGNED_BYTE);

  ReadbackSwizzle swizzle = kSwizzleNone;
  if (supported == GLHelperReadbackSupport::SWIZZLE)
    swizzle = kSwizzleBGRA;

  return std::make_unique<ReadbackYUVImpl>(
      gl_, this, helper_->scaler_impl_.get(), flip_vertically, swizzle,
      use_mrt && (max_draw_buffers_ >= 2));
}

std::unique_ptr<ReadbackYUVInterface> GLHelper::CreateReadbackPipelineYUV(
    bool flip_vertically,
    bool use_mrt) {
  InitCopyTextToImpl();
  return copy_texture_to_impl_->CreateReadbackPipelineYUV(flip_vertically,
                                                          use_mrt);
}

ReadbackYUVInterface* GLHelper::GetReadbackPipelineYUV(
    bool vertically_flip_texture) {
  ReadbackYUVInterface* yuv_reader = nullptr;
  if (vertically_flip_texture) {
    if (!shared_readback_yuv_flip_) {
      shared_readback_yuv_flip_ = CreateReadbackPipelineYUV(
          vertically_flip_texture, true /* use_mrt */);
    }
    yuv_reader = shared_readback_yuv_flip_.get();
  } else {
    if (!shared_readback_yuv_noflip_) {
      shared_readback_yuv_noflip_ = CreateReadbackPipelineYUV(
          vertically_flip_texture, true /* use_mrt */);
    }
    yuv_reader = shared_readback_yuv_noflip_.get();
  }
  DCHECK(!yuv_reader->scaler());
  return yuv_reader;
}

GLHelperReadbackSupport* GLHelper::GetReadbackSupport() {
  LazyInitReadbackSupportImpl();
  return readback_support_.get();
}

}  // namespace viz
