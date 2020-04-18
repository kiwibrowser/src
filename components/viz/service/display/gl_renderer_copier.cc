// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/gl_renderer_copier.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/stl_util.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "components/viz/common/frame_sinks/copy_output_util.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/service/display/texture_deleter.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

// Syntactic sugar to DCHECK that two sizes are equal.
#define DCHECK_SIZE_EQ(a, b)                                \
  DCHECK((a) == (b)) << #a " != " #b ": " << (a).ToString() \
                     << " != " << (b).ToString()

namespace viz {

using ResultFormat = CopyOutputRequest::ResultFormat;

namespace {

constexpr int kRGBABytesPerPixel = 4;

// Returns the source property of the |request|, if it is set. Otherwise,
// returns an empty token. This is needed because CopyOutputRequest will crash
// if source() is called when !has_source().
base::UnguessableToken SourceOf(const CopyOutputRequest& request) {
  return request.has_source() ? request.source() : base::UnguessableToken();
}

}  // namespace

GLRendererCopier::GLRendererCopier(
    scoped_refptr<ContextProvider> context_provider,
    TextureDeleter* texture_deleter,
    ComputeWindowRectCallback window_rect_callback)
    : context_provider_(std::move(context_provider)),
      texture_deleter_(texture_deleter),
      window_rect_callback_(std::move(window_rect_callback)),
      helper_(context_provider_->ContextGL(),
              context_provider_->ContextSupport()) {}

GLRendererCopier::~GLRendererCopier() {
  for (auto& entry : cache_)
    FreeCachedResources(&entry.second);
}

void GLRendererCopier::CopyFromTextureOrFramebuffer(
    std::unique_ptr<CopyOutputRequest> request,
    const gfx::Rect& output_rect,
    GLenum internal_format,
    GLuint framebuffer_texture,
    const gfx::Size& framebuffer_texture_size,
    bool flipped_source,
    const gfx::ColorSpace& color_space) {
  // Finalize the source subrect, as the entirety of the RenderPass's output
  // optionally clamped to the requested copy area. Then, compute the result
  // rect, which is the selection clamped to the maximum possible result bounds.
  // If there will be zero pixels of output or the scaling ratio was not
  // reasonable, do not proceed.
  gfx::Rect copy_rect = output_rect;
  if (request->has_area())
    copy_rect.Intersect(request->area());
  gfx::Rect result_rect = request->is_scaled()
                              ? copy_output::ComputeResultRect(
                                    gfx::Rect(copy_rect.size()),
                                    request->scale_from(), request->scale_to())
                              : gfx::Rect(copy_rect.size());
  if (request->has_result_selection())
    result_rect.Intersect(request->result_selection());
  if (result_rect.IsEmpty())
    return;

  // Execute the cheapest workflow that satisfies the copy request.
  switch (request->result_format()) {
    case ResultFormat::RGBA_BITMAP: {
      // Scale and/or flip the source framebuffer content, but only if
      // necessary, before starting readback.
      if (request->is_scaled() || !flipped_source) {
        const GLuint result_texture = RenderResultTexture(
            *request, copy_rect, internal_format, framebuffer_texture,
            framebuffer_texture_size, flipped_source, result_rect);
        const base::UnguessableToken& request_source = SourceOf(*request);
        StartReadbackFromTexture(std::move(request), result_texture,
                                 gfx::Rect(result_rect.size()), result_rect,
                                 color_space);
        CacheObjectsOrDelete(request_source, CacheEntry::kResultTexture, 1,
                             &result_texture);
      } else {
        StartReadbackFromFramebuffer(
            std::move(request),
            window_rect_callback_.Run(result_rect +
                                      copy_rect.OffsetFromOrigin()),
            result_rect, color_space);
      }
      break;
    }

    case ResultFormat::RGBA_TEXTURE: {
      const GLuint result_texture = RenderResultTexture(
          *request, copy_rect, internal_format, framebuffer_texture,
          framebuffer_texture_size, flipped_source, result_rect);
      SendTextureResult(std::move(request), result_texture, result_rect,
                        color_space);
      break;
    }

    case ResultFormat::I420_PLANES: {
      // The optimized single-copy path, provided by GLBufferI420Result,
      // requires that the result be accessed via a task in the same task runner
      // sequence as the GLRendererCopier. Since I420_PLANES requests are meant
      // to be VIZ-internal, this is an acceptable limitation to enforce.
      DCHECK(request->SendsResultsInCurrentSequence());

      // I420 readback always requires a source texture whose content is
      // Y-flipped. If a |framebuffer_texture| was not provided, or its content
      // is not flipped, or scaling was requested; an intermediate texture must
      // first be rendered from the currently-bound framebuffer.
      if (request->is_scaled() || !flipped_source || framebuffer_texture == 0) {
        const GLuint result_texture = RenderResultTexture(
            *request, copy_rect, internal_format, framebuffer_texture,
            framebuffer_texture_size, flipped_source, result_rect);
        const base::UnguessableToken& request_source = SourceOf(*request);
        StartI420ReadbackFromTexture(
            std::move(request), result_texture, result_rect.size(),
            gfx::Rect(result_rect.size()), result_rect, color_space);
        CacheObjectsOrDelete(request_source, CacheEntry::kResultTexture, 1,
                             &result_texture);
      } else {
        StartI420ReadbackFromTexture(
            std::move(request), framebuffer_texture, framebuffer_texture_size,
            window_rect_callback_.Run(result_rect +
                                      copy_rect.OffsetFromOrigin()),
            result_rect, color_space);
      }
      break;
    }
  }
}

void GLRendererCopier::FreeUnusedCachedResources() {
  ++purge_counter_;

  // Purge all cache entries that should no longer be kept alive, freeing any
  // resources they held.
  const auto IsTooOld = [this](const decltype(cache_)::value_type& entry) {
    return static_cast<int32_t>(purge_counter_ -
                                entry.second.purge_count_at_last_use) >=
           kKeepalivePeriod;
  };
  for (auto& entry : cache_) {
    if (IsTooOld(entry))
      FreeCachedResources(&entry.second);
  }
  base::EraseIf(cache_, IsTooOld);
}

GLuint GLRendererCopier::RenderResultTexture(
    const CopyOutputRequest& request,
    const gfx::Rect& framebuffer_copy_rect,
    GLenum internal_format,
    GLuint framebuffer_texture,
    const gfx::Size& framebuffer_texture_size,
    bool flipped_source,
    const gfx::Rect& result_rect) {
  // Compute the sampling rect. This is the region of the framebuffer, in window
  // coordinates, which contains the pixels that can affect the result.
  //
  // TODO(crbug.com/775740): When executing for scaling copy requests, use the
  // scaler's ComputeRegionOfInfluence() utility to compute a smaller sampling
  // rect so that a smaller source copy can be made below. But first, the scaler
  // implementation needs to be fixed to account for fractional source offsets.
  gfx::Rect sampling_rect = window_rect_callback_.Run(
      request.is_scaled()
          ? framebuffer_copy_rect
          : (result_rect + framebuffer_copy_rect.OffsetFromOrigin()));

  auto* const gl = context_provider_->ContextGL();

  // Determine the source texture: This is either the one attached to the
  // framebuffer, or a copy made from the framebuffer. Its format will be the
  // same as |internal_format|.
  //
  // TODO(crbug/767221): All of this (including some texture copies) wouldn't be
  // necessary if we could query whether the currently-bound framebuffer has a
  // texture attached to it, and just source from that texture directly (i.e.,
  // using glGetFramebufferAttachmentParameteriv() and
  // glGetTexLevelParameteriv(GL_TEXTURE_WIDTH/HEIGHT)).
  GLuint source_texture;
  gfx::Size source_texture_size;
  if (framebuffer_texture != 0) {
    source_texture = framebuffer_texture;
    source_texture_size = framebuffer_texture_size;
  } else {
    // Optimization: If the texture copy completely satsifies the request, just
    // return it as the result texture. The request must not include scaling nor
    // a texture mailbox to use for delivering results. The texture format must
    // also be GL_RGBA, as described by CopyOutputResult::Format::RGBA_TEXTURE.
    const int purpose =
        (!request.is_scaled() && flipped_source && internal_format == GL_RGBA)
            ? CacheEntry::kResultTexture
            : CacheEntry::kFramebufferCopyTexture;
    TakeCachedObjectsOrCreate(SourceOf(request), purpose, 1, &source_texture);
    gl->BindTexture(GL_TEXTURE_2D, source_texture);
    gl->CopyTexImage2D(GL_TEXTURE_2D, 0, internal_format, sampling_rect.x(),
                       sampling_rect.y(), sampling_rect.width(),
                       sampling_rect.height(), 0);
    if (purpose == CacheEntry::kResultTexture)
      return source_texture;
    source_texture_size = sampling_rect.size();
    sampling_rect.set_origin(gfx::Point());
  }

  GLuint result_texture = 0;
  TakeCachedObjectsOrCreate(SourceOf(request), CacheEntry::kResultTexture, 1,
                            &result_texture);
  gl->BindTexture(GL_TEXTURE_2D, result_texture);
  gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, result_rect.width(),
                 result_rect.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  // Populate the result texture with a scaled/exact copy.
  if (request.is_scaled()) {
    std::unique_ptr<GLHelper::ScalerInterface> scaler =
        TakeCachedScalerOrCreate(request, flipped_source);
    // The scaler will assume the Y offset does not account for a flipped source
    // texture. However, |sampling_rect| does account for that. Thus, translate
    // back for the call to Scale() below.
    const gfx::Vector2d source_offset =
        flipped_source
            ? gfx::Vector2d(sampling_rect.x(), source_texture_size.height() -
                                                   sampling_rect.bottom())
            : sampling_rect.OffsetFromOrigin();
    scaler->Scale(source_texture, source_texture_size, source_offset,
                  result_texture, result_rect);
    CacheScalerOrDelete(SourceOf(request), std::move(scaler));
  } else {
    DCHECK_SIZE_EQ(sampling_rect.size(), result_rect.size());
    const bool flip_output = !flipped_source;
    gl->CopySubTextureCHROMIUM(
        source_texture, 0 /* source_level */, GL_TEXTURE_2D, result_texture,
        0 /* dest_level */, 0 /* xoffset */, 0 /* yoffset */, sampling_rect.x(),
        sampling_rect.y(), sampling_rect.width(), sampling_rect.height(),
        flip_output, false, false);
  }

  // If |source_texture| was a copy, maybe cache it for future requests.
  if (framebuffer_texture == 0) {
    CacheObjectsOrDelete(SourceOf(request), CacheEntry::kFramebufferCopyTexture,
                         1, &source_texture);
  }

  return result_texture;
}

void GLRendererCopier::StartReadbackFromTexture(
    std::unique_ptr<CopyOutputRequest> request,
    GLuint source_texture,
    const gfx::Rect& copy_rect,
    const gfx::Rect& result_rect,
    const gfx::ColorSpace& color_space) {
  // Bind |source_texture| to a framebuffer, and then start readback from that.
  GLuint framebuffer = 0;
  const base::UnguessableToken& request_source = SourceOf(*request);
  TakeCachedObjectsOrCreate(request_source, CacheEntry::kReadbackFramebuffer, 1,
                            &framebuffer);
  auto* const gl = context_provider_->ContextGL();
  gl->BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           source_texture, 0);
  StartReadbackFromFramebuffer(std::move(request), copy_rect, result_rect,
                               color_space);
  CacheObjectsOrDelete(request_source, CacheEntry::kReadbackFramebuffer, 1,
                       &framebuffer);
}

namespace {

class GLPixelBufferRGBAResult : public CopyOutputResult {
 public:
  GLPixelBufferRGBAResult(const gfx::Rect& result_rect,
                          scoped_refptr<ContextProvider> context_provider,
                          GLuint transfer_buffer,
                          GLenum readback_format)
      : CopyOutputResult(CopyOutputResult::Format::RGBA_BITMAP, result_rect),
        context_provider_(std::move(context_provider)),
        transfer_buffer_(transfer_buffer),
        readback_format_(readback_format) {}

  ~GLPixelBufferRGBAResult() final {
    if (transfer_buffer_)
      context_provider_->ContextGL()->DeleteBuffers(1, &transfer_buffer_);
  }

  bool ReadRGBAPlane(uint8_t* dest, int stride) const final {
    // No need to read from GPU memory if a cached bitmap already exists.
    if (rect().IsEmpty() || cached_bitmap()->readyToDraw())
      return CopyOutputResult::ReadRGBAPlane(dest, stride);
    auto* const gl = context_provider_->ContextGL();
    gl->BindBuffer(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, transfer_buffer_);
    const uint8_t* pixels = static_cast<uint8_t*>(gl->MapBufferCHROMIUM(
        GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, GL_READ_ONLY));
    if (pixels) {
      const SkColorType src_format = (readback_format_ == GL_BGRA_EXT)
                                         ? kBGRA_8888_SkColorType
                                         : kRGBA_8888_SkColorType;
      const int src_bytes_per_row = size().width() * kRGBABytesPerPixel;
      const SkImageInfo src_row_image_info =
          SkImageInfo::Make(size().width(), 1, src_format, kPremul_SkAlphaType);
      const SkImageInfo dest_row_image_info =
          SkImageInfo::MakeN32Premul(size().width(), 1);

      for (int y = 0; y < size().height(); ++y) {
        const int flipped_y = (size().height() - y - 1);
        const uint8_t* const src_row = pixels + flipped_y * src_bytes_per_row;
        void* const dest_row = dest + y * stride;
        SkPixmap src_pixmap(src_row_image_info, src_row, src_bytes_per_row);
        SkPixmap dest_pixmap(dest_row_image_info, dest_row, stride);
        src_pixmap.readPixels(dest_pixmap);
      }
      gl->UnmapBufferCHROMIUM(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM);
    }
    gl->BindBuffer(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, 0);
    return !!pixels;
  }

  const SkBitmap& AsSkBitmap() const final {
    if (rect().IsEmpty())
      return *cached_bitmap();  // Return "null" bitmap for empty result.

    if (cached_bitmap()->readyToDraw())
      return *cached_bitmap();

    SkBitmap result_bitmap;
    result_bitmap.allocPixels(
        SkImageInfo::MakeN32Premul(size().width(), size().height()));
    ReadRGBAPlane(static_cast<uint8_t*>(result_bitmap.getPixels()),
                  result_bitmap.rowBytes());
    *cached_bitmap() = result_bitmap;
    // Now that we have a cached bitmap, no need to read from GPU memory
    // anymore.
    context_provider_->ContextGL()->DeleteBuffers(1, &transfer_buffer_);
    transfer_buffer_ = 0;
    return *cached_bitmap();
  }

 private:
  const scoped_refptr<ContextProvider> context_provider_;
  mutable GLuint transfer_buffer_;
  GLenum readback_format_;
};

// Manages the execution of one asynchronous framebuffer readback and contains
// all the relevant state needed to complete a copy request. The constructor
// initiates the operation, and then at some later point either: 1) the Finish()
// method is invoked; or 2) the instance will be destroyed (cancelled) because
// the GL context is going away. Either way, the GL objects created for this
// workflow are properly cleaned-up.
//
// Motivation: In case #2, it's possible GLRendererCopier will have been
// destroyed before Finish(). However, since there are no dependencies on
// GLRendererCopier to finish the copy request, there's no reason to mess around
// with a complex WeakPtr-to-GLRendererCopier scheme.
class ReadPixelsWorkflow {
 public:
  // Saves all revelant state and initiates the GL asynchronous read-pixels
  // workflow.
  ReadPixelsWorkflow(std::unique_ptr<CopyOutputRequest> copy_request,
                     const gfx::Rect& copy_rect,
                     const gfx::Rect& result_rect,
                     scoped_refptr<ContextProvider> context_provider,
                     GLenum readback_format)
      : copy_request_(std::move(copy_request)),
        result_rect_(result_rect),
        context_provider_(std::move(context_provider)),
        readback_format_(readback_format) {
    DCHECK(readback_format_ == GL_RGBA || readback_format_ == GL_BGRA_EXT);

    auto* const gl = context_provider_->ContextGL();

    // Create a buffer for the pixel transfer.
    gl->GenBuffers(1, &transfer_buffer_);
    gl->BindBuffer(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, transfer_buffer_);
    gl->BufferData(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM,
                   kRGBABytesPerPixel * result_rect.size().GetArea(), nullptr,
                   GL_STREAM_READ);

    // Execute an asynchronous read-pixels operation, with a query that triggers
    // when Finish() should be run.
    gl->GenQueriesEXT(1, &query_);
    gl->BeginQueryEXT(GL_ASYNC_PIXEL_PACK_COMPLETED_CHROMIUM, query_);
    gl->ReadPixels(copy_rect.x(), copy_rect.y(), copy_rect.width(),
                   copy_rect.height(), readback_format_, GL_UNSIGNED_BYTE,
                   nullptr);
    gl->EndQueryEXT(GL_ASYNC_PIXEL_PACK_COMPLETED_CHROMIUM);
    gl->BindBuffer(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, 0);
  }

  // The destructor is called when the callback that owns this instance is
  // destroyed. That will happen either with or without a call to Finish(),
  // but either way everything will be clean-up appropriately.
  ~ReadPixelsWorkflow() {
    auto* const gl = context_provider_->ContextGL();
    gl->DeleteQueriesEXT(1, &query_);
    if (transfer_buffer_)
      gl->DeleteBuffers(1, &transfer_buffer_);
  }

  GLuint query() const { return query_; }

  // Callback for the asynchronous glReadPixels(). The pixels are read from the
  // transfer buffer, and a CopyOutputResult is sent to the requestor.
  void Finish() {
    auto result = std::make_unique<GLPixelBufferRGBAResult>(
        result_rect_, context_provider_, transfer_buffer_, readback_format_);
    transfer_buffer_ = 0;  // Ownerhip was transferred to the result.
    if (!copy_request_->SendsResultsInCurrentSequence()) {
      // Force readback into a SkBitmap now, because after PostTask we don't
      // have access to |context_provider_|.
      result->AsSkBitmap();
    }
    copy_request_->SendResult(std::move(result));
  }

 private:
  const std::unique_ptr<CopyOutputRequest> copy_request_;
  const gfx::Rect result_rect_;
  const scoped_refptr<ContextProvider> context_provider_;
  const GLenum readback_format_;
  GLuint transfer_buffer_ = 0;
  GLuint query_ = 0;
};

}  // namespace

void GLRendererCopier::StartReadbackFromFramebuffer(
    std::unique_ptr<CopyOutputRequest> request,
    const gfx::Rect& copy_rect,
    const gfx::Rect& result_rect,
    const gfx::ColorSpace& color_space) {
  DCHECK_NE(request->result_format(), ResultFormat::RGBA_TEXTURE);
  DCHECK_SIZE_EQ(copy_rect.size(), result_rect.size());

  auto workflow = std::make_unique<ReadPixelsWorkflow>(
      std::move(request), copy_rect, result_rect, context_provider_,
      GetOptimalReadbackFormat());
  const GLuint query = workflow->query();
  context_provider_->ContextSupport()->SignalQuery(
      query, base::BindOnce(&ReadPixelsWorkflow::Finish, std::move(workflow)));
}

void GLRendererCopier::SendTextureResult(
    std::unique_ptr<CopyOutputRequest> request,
    GLuint result_texture,
    const gfx::Rect& result_rect,
    const gfx::ColorSpace& color_space) {
  DCHECK_EQ(request->result_format(), ResultFormat::RGBA_TEXTURE);

  auto* const gl = context_provider_->ContextGL();

  // Package the |result_texture| into a mailbox with the required
  // synchronization mechanisms. This lets the requestor ensure operations
  // within its own GL context will be using the texture at a point in time
  // after the texture has been rendered (via GLRendererCopier's GL context).
  gpu::Mailbox mailbox;
  gl->GenMailboxCHROMIUM(mailbox.name);
  gl->ProduceTextureDirectCHROMIUM(result_texture, mailbox.name);
  gpu::SyncToken sync_token;
  gl->GenSyncTokenCHROMIUM(sync_token.GetData());

  // Create a callback that deletes what was created in this GL context.
  // Note: There's no need to try to pool/re-use the result texture from here,
  // since only clients that are trying to re-invent video capture would see any
  // significant performance benefit. Instead, such clients should use the video
  // capture services provided by VIZ.
  auto release_callback =
      texture_deleter_->GetReleaseCallback(context_provider_, result_texture);

  request->SendResult(std::make_unique<CopyOutputTextureResult>(
      result_rect, mailbox, sync_token, color_space,
      std::move(release_callback)));
}

namespace {

// Specialization of CopyOutputResult which reads I420 plane data from a GL
// pixel buffer object, and automatically deletes the pixel buffer object at
// destruction time. This provides an optimal one-copy data flow, from the pixel
// buffer into client-provided memory.
class GLPixelBufferI420Result : public CopyOutputResult {
 public:
  GLPixelBufferI420Result(const gfx::Rect& result_rect,
                          scoped_refptr<ContextProvider> context_provider,
                          GLuint transfer_buffer,
                          int y_stride,
                          int chroma_stride)
      : CopyOutputResult(CopyOutputResult::Format::I420_PLANES, result_rect),
        context_provider_(std::move(context_provider)),
        transfer_buffer_(transfer_buffer),
        y_stride_(y_stride),
        chroma_stride_(chroma_stride) {}

  ~GLPixelBufferI420Result() final {
    context_provider_->ContextGL()->DeleteBuffers(1, &transfer_buffer_);
  }

  bool ReadI420Planes(uint8_t* y_out,
                      int y_out_stride,
                      uint8_t* u_out,
                      int u_out_stride,
                      uint8_t* v_out,
                      int v_out_stride) const final {
    DCHECK_GE(y_out_stride, size().width());
    const int chroma_row_bytes = (size().width() + 1) / 2;
    DCHECK_GE(u_out_stride, chroma_row_bytes);
    DCHECK_GE(v_out_stride, chroma_row_bytes);

    auto* const gl = context_provider_->ContextGL();
    gl->BindBuffer(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, transfer_buffer_);
    const uint8_t* pixels = static_cast<uint8_t*>(gl->MapBufferCHROMIUM(
        GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, GL_READ_ONLY));
    if (pixels) {
      const auto CopyPlane = [](const uint8_t* src, int src_stride,
                                int row_bytes, int num_rows, uint8_t* out,
                                int out_stride) {
        for (int i = 0; i < num_rows;
             ++i, src += src_stride, out += out_stride) {
          memcpy(out, src, row_bytes);
        }
      };
      CopyPlane(pixels, y_stride_, size().width(), size().height(), y_out,
                y_out_stride);
      pixels += y_stride_ * size().height();
      const int chroma_height = (size().height() + 1) / 2;
      CopyPlane(pixels, chroma_stride_, chroma_row_bytes, chroma_height, u_out,
                u_out_stride);
      pixels += chroma_stride_ * chroma_height;
      CopyPlane(pixels, chroma_stride_, chroma_row_bytes, chroma_height, v_out,
                v_out_stride);
      gl->UnmapBufferCHROMIUM(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM);
    }
    gl->BindBuffer(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, 0);

    return !!pixels;
  }

 private:
  const scoped_refptr<ContextProvider> context_provider_;
  const GLuint transfer_buffer_;
  const int y_stride_;
  const int chroma_stride_;
  const gfx::Vector2d i420_offset_;
};

// Like the ReadPixelsWorkflow, except for I420 planes readback. Because there
// are three separate glReadPixels operations that may complete in any order, a
// ReadI420PlanesWorkflow will receive notifications from three separate "GL
// query" callbacks. It is only after all three operations have completed that a
// fully-assembled CopyOutputResult can be sent.
//
// Please see class comments for ReadPixelsWorkflow for discussion about how GL
// context loss is handled during the workflow.
//
// Also, see gl_helper.h for an explanation of how planar data is packed into
// RGBA textures, and how Y/Chroma plane rounding work.
class ReadI420PlanesWorkflow
    : public base::RefCountedThreadSafe<ReadI420PlanesWorkflow> {
 public:
  ReadI420PlanesWorkflow(std::unique_ptr<CopyOutputRequest> copy_request,
                         const gfx::Rect& result_rect,
                         scoped_refptr<ContextProvider> context_provider)
      : copy_request_(std::move(copy_request)),
        result_rect_(result_rect),
        context_provider_(std::move(context_provider)),
        y_texture_size_(
            I420Converter::GetYPlaneTextureSize(result_rect.size())),
        chroma_texture_size_(
            I420Converter::GetChromaPlaneTextureSize(result_rect.size())) {
    // Create a buffer for the pixel transfer: A single buffer is used and will
    // contain the Y plane, then the U plane, then the V plane.
    auto* const gl = context_provider_->ContextGL();
    gl->GenBuffers(1, &transfer_buffer_);
    gl->BindBuffer(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, transfer_buffer_);
    const int y_plane_bytes = kRGBABytesPerPixel * y_texture_size_.GetArea();
    const int chroma_plane_bytes =
        kRGBABytesPerPixel * chroma_texture_size_.GetArea();
    gl->BufferData(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM,
                   y_plane_bytes + 2 * chroma_plane_bytes, nullptr,
                   GL_STREAM_READ);
    data_offsets_ = {0, y_plane_bytes, y_plane_bytes + chroma_plane_bytes};
    gl->BindBuffer(GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, 0);

    // Generate the three queries used for determining when each of the plane
    // readbacks has completed.
    gl->GenQueriesEXT(3, queries_.data());
  }

  void BindTransferBuffer() {
    DCHECK_NE(transfer_buffer_, 0u);
    context_provider_->ContextGL()->BindBuffer(
        GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, transfer_buffer_);
  }

  void StartPlaneReadback(int plane, GLenum readback_format) {
    DCHECK_NE(queries_[plane], 0u);
    auto* const gl = context_provider_->ContextGL();
    gl->BeginQueryEXT(GL_ASYNC_PIXEL_PACK_COMPLETED_CHROMIUM, queries_[plane]);
    const gfx::Size& size = plane == 0 ? y_texture_size_ : chroma_texture_size_;
    // Note: While a PIXEL_PACK_BUFFER is bound, OpenGL interprets the last
    // argument to ReadPixels() as a byte offset within the buffer instead of
    // an actual pointer in system memory.
    uint8_t* offset_in_buffer = 0;
    offset_in_buffer += data_offsets_[plane];
    gl->ReadPixels(0, 0, size.width(), size.height(), readback_format,
                   GL_UNSIGNED_BYTE, offset_in_buffer);
    gl->EndQueryEXT(GL_ASYNC_PIXEL_PACK_COMPLETED_CHROMIUM);
    context_provider_->ContextSupport()->SignalQuery(
        queries_[plane],
        base::Bind(&ReadI420PlanesWorkflow::OnFinishedPlane, this, plane));
  }

  void UnbindTransferBuffer() {
    context_provider_->ContextGL()->BindBuffer(
        GL_PIXEL_PACK_TRANSFER_BUFFER_CHROMIUM, 0);
  }

 private:
  friend class base::RefCountedThreadSafe<ReadI420PlanesWorkflow>;

  ~ReadI420PlanesWorkflow() {
    auto* const gl = context_provider_->ContextGL();
    if (transfer_buffer_ != 0)
      gl->DeleteBuffers(1, &transfer_buffer_);
    for (GLuint& query : queries_) {
      if (query != 0)
        gl->DeleteQueriesEXT(1, &query);
    }
  }

  void OnFinishedPlane(int plane) {
    context_provider_->ContextGL()->DeleteQueriesEXT(1, &queries_[plane]);
    queries_[plane] = 0;

    // If all three readbacks have completed, send the result.
    if (queries_ == std::array<GLuint, 3>{{0, 0, 0}}) {
      copy_request_->SendResult(std::make_unique<GLPixelBufferI420Result>(
          result_rect_, context_provider_, transfer_buffer_,
          kRGBABytesPerPixel * y_texture_size_.width(),
          kRGBABytesPerPixel * chroma_texture_size_.width()));
      transfer_buffer_ = 0;  // Ownership was transferred to the result.
    }
  }

  const std::unique_ptr<CopyOutputRequest> copy_request_;
  const gfx::Rect result_rect_;
  const scoped_refptr<ContextProvider> context_provider_;
  const gfx::Size y_texture_size_;
  const gfx::Size chroma_texture_size_;
  GLuint transfer_buffer_;
  std::array<int, 3> data_offsets_;
  std::array<GLuint, 3> queries_;
};

}  // namespace

void GLRendererCopier::StartI420ReadbackFromTexture(
    std::unique_ptr<CopyOutputRequest> request,
    GLuint source_texture,
    const gfx::Size& source_texture_size,
    const gfx::Rect& copy_rect,
    const gfx::Rect& result_rect,
    const gfx::ColorSpace& color_space) {
  DCHECK_EQ(request->result_format(), ResultFormat::I420_PLANES);
  DCHECK_SIZE_EQ(copy_rect.size(), result_rect.size());

  // Get the GL objects needed for I420 readback.
  const base::UnguessableToken& source = SourceOf(*request);
  std::array<GLuint, 3> plane_textures;
  TakeCachedObjectsOrCreate(source, CacheEntry::kYPlaneTexture, 3,
                            plane_textures.data());
  std::array<GLuint, 3> plane_framebuffers;
  TakeCachedObjectsOrCreate(source, CacheEntry::kReadbackFramebuffer, 3,
                            plane_framebuffers.data());

  // Run-once, if needed: If the optimal readback format has not yet been
  // determined, call GetOptimalReadbackFormat() now. It will query the GL
  // implementation, which requires a bound framebuffer, ready for readback.
  // Therefore, just set-up the Y plane's texture+framebuffer and go for it.
  // This must be done now so that the I420Converter can be configured according
  // to the readback format.
  auto* const gl = context_provider_->ContextGL();
  if (optimal_readback_format_ == static_cast<GLenum>(GL_NONE)) {
    gl->BindTexture(GL_TEXTURE_2D, plane_textures[0]);
    const gfx::Size& y_texture_size =
        I420Converter::GetYPlaneTextureSize(result_rect.size());
    gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, y_texture_size.width(),
                   y_texture_size.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                   nullptr);
    gl->BindFramebuffer(GL_FRAMEBUFFER, plane_framebuffers[0]);
    gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, plane_textures[0], 0);
    GetOptimalReadbackFormat();
  }

  // Convert the |source_texture| into separate Y+U+V planes.
  std::unique_ptr<I420Converter> converter =
      TakeCachedI420ConverterOrCreate(source);
  // The converter will assume the Y offset does not account for a flipped
  // source texture. However, |copy_rect| does account for that. Thus, translate
  // back for the call to Convert() below.
  const gfx::Vector2d source_offset(
      copy_rect.x(), source_texture_size.height() - copy_rect.bottom());
  // TODO(crbug/758057): Plumb-in proper color space conversion into
  // I420Converter. If the request did not specify one, use Rec. 709.
  converter->Convert(source_texture, source_texture_size, source_offset,
                     nullptr, result_rect, plane_textures[0], plane_textures[1],
                     plane_textures[2]);

  // Execute three asynchronous read-pixels operations, one for each plane. The
  // CopyOutputRequest is passed to the ReadI420PlanesWorkflow, which will send
  // the CopyOutputResult once all readback operations are complete.
  const auto workflow = base::MakeRefCounted<ReadI420PlanesWorkflow>(
      std::move(request), result_rect, context_provider_);
  workflow->BindTransferBuffer();
  for (int plane = 0; plane < 3; ++plane) {
    gl->BindFramebuffer(GL_FRAMEBUFFER, plane_framebuffers[plane]);
    gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, plane_textures[plane], 0);
    workflow->StartPlaneReadback(plane, converter->GetReadbackFormat());
  }
  workflow->UnbindTransferBuffer();

  // Clean up.
  CacheI420ConverterOrDelete(source, std::move(converter));
  CacheObjectsOrDelete(source, CacheEntry::kReadbackFramebuffer, 3,
                       plane_framebuffers.data());
  CacheObjectsOrDelete(source, CacheEntry::kYPlaneTexture, 3,
                       plane_textures.data());
}

void GLRendererCopier::TakeCachedObjectsOrCreate(
    const base::UnguessableToken& for_source,
    int first,
    int count,
    GLuint* names) {
  for (int i = 0; i < count; ++i)
    names[i] = 0;

  // If the objects can be found in the cache, take them and return them.
  if (!for_source.is_empty()) {
    auto& cached_object_names = cache_[for_source].object_names;
    if (cached_object_names[first] != 0) {
      for (int i = 0; i < count; ++i) {
        names[i] = cached_object_names[first + i];
        // Assumption: The caller is always using the same |first| and |count|
        // args, and so every GLuint copied to |names| should be non-zero.
        DCHECK_NE(names[i], 0u);
        cached_object_names[first + i] = 0;
      }
      return;
    }
  }

  // Generate new ones.
  auto* const gl = context_provider_->ContextGL();
  if (first >= CacheEntry::kReadbackFramebuffer) {
    gl->GenFramebuffers(count, names);
  } else {
    gl->GenTextures(count, names);
    for (int i = 0; i < count; ++i) {
      gl->BindTexture(GL_TEXTURE_2D, names[i]);
      gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
  }
}

void GLRendererCopier::CacheObjectsOrDelete(
    const base::UnguessableToken& for_source,
    int first,
    int count,
    const GLuint* names) {
  // Do not cache objects for copy requests without a common source.
  if (for_source.is_empty()) {
    auto* const gl = context_provider_->ContextGL();
    if (first >= CacheEntry::kReadbackFramebuffer)
      gl->DeleteFramebuffers(count, names);
    else
      gl->DeleteTextures(count, names);
    return;
  }

  CacheEntry& entry = cache_[for_source];
  for (int i = 0; i < count; ++i) {
    DCHECK_EQ(entry.object_names[first + i], 0u);
    entry.object_names[first + i] = names[i];
  }
  entry.purge_count_at_last_use = purge_counter_;
}

std::unique_ptr<GLHelper::ScalerInterface>
GLRendererCopier::TakeCachedScalerOrCreate(const CopyOutputRequest& for_request,
                                           bool flipped_source) {
  // If an identically-configured scaler can be found in the cache, take it and
  // return it. If a differently-configured scaler was found, delete it.
  if (for_request.has_source()) {
    std::unique_ptr<GLHelper::ScalerInterface>& cached_scaler =
        cache_[for_request.source()].scaler;
    if (cached_scaler) {
      if (cached_scaler->IsSameScaleRatio(for_request.scale_from(),
                                          for_request.scale_to()) &&
          cached_scaler->IsSamplingFlippedSource() == flipped_source) {
        return std::move(cached_scaler);
      } else {
        cached_scaler.reset();
      }
    }
  }

  // At this point, a new instance must be created. For downscaling, use the
  // GOOD quality setting (appropriate for thumbnailing); and, for upscaling,
  // use the BEST quality.
  const bool is_downscale_in_both_dimensions =
      for_request.scale_to().x() < for_request.scale_from().x() &&
      for_request.scale_to().y() < for_request.scale_from().y();
  const GLHelper::ScalerQuality quality = is_downscale_in_both_dimensions
                                              ? GLHelper::SCALER_QUALITY_GOOD
                                              : GLHelper::SCALER_QUALITY_BEST;
  const bool flip_output = !flipped_source;
  return helper_.CreateScaler(quality, for_request.scale_from(),
                              for_request.scale_to(), flipped_source,
                              flip_output, false);
}

void GLRendererCopier::CacheScalerOrDelete(
    const base::UnguessableToken& for_source,
    std::unique_ptr<GLHelper::ScalerInterface> scaler) {
  // If the request has a source, cache |scaler| for the next copy request.
  // Otherwise, |scaler| will be auto-deleted on out-of-scope.
  if (!for_source.is_empty()) {
    CacheEntry& entry = cache_[for_source];
    entry.scaler = std::move(scaler);
    entry.purge_count_at_last_use = purge_counter_;
  }
}

std::unique_ptr<I420Converter>
GLRendererCopier::TakeCachedI420ConverterOrCreate(
    const base::UnguessableToken& for_source) {
  // If there is an I420 converter in the cache for the request's source, take
  // it and return it.
  if (!for_source.is_empty()) {
    std::unique_ptr<I420Converter>& cached_converter =
        cache_[for_source].i420_converter;
    if (cached_converter)
      return std::move(cached_converter);
  }

  // A new one must be created: The converter will also do the Y-flip and byte
  // swizzling in GL so that the data copied from the mapped pixel buffer is in
  // the exact row and byte ordering needed for GLPixelBufferI420Result.
  return helper_.CreateI420Converter(
      true, true, GetOptimalReadbackFormat() == GL_BGRA_EXT, true);
}

void GLRendererCopier::CacheI420ConverterOrDelete(
    const base::UnguessableToken& for_source,
    std::unique_ptr<I420Converter> i420_converter) {
  // If the request has a source, cache |i420_converter| for the next copy
  // request. Otherwise, it will be auto-deleted on out-of-scope.
  if (!for_source.is_empty()) {
    CacheEntry& entry = cache_[for_source];
    entry.i420_converter = std::move(i420_converter);
    entry.purge_count_at_last_use = purge_counter_;
  }
}

void GLRendererCopier::FreeCachedResources(CacheEntry* entry) {
  auto* const gl = context_provider_->ContextGL();
  size_t i = 0;
  for (; i < static_cast<size_t>(CacheEntry::kReadbackFramebuffer); ++i) {
    if (entry->object_names[i] != 0)
      gl->DeleteTextures(1, &(entry->object_names[i]));
  }
  for (; i < entry->object_names.size(); ++i) {
    if (entry->object_names[i] != 0)
      gl->DeleteFramebuffers(1, &(entry->object_names[i]));
  }
  entry->object_names.fill(0);
  entry->scaler.reset();
  entry->i420_converter.reset();
}

GLenum GLRendererCopier::GetOptimalReadbackFormat() {
  if (optimal_readback_format_ != GL_NONE)
    return optimal_readback_format_;

  // If the GL implementation internally uses the GL_BGRA_EXT+GL_UNSIGNED_BYTE
  // format+type combination, then consider that the optimal readback
  // format+type. Otherwise, use GL_RGBA+GL_UNSIGNED_BYTE, which all platforms
  // must support, per the GLES 2.0 spec.
  auto* const gl = context_provider_->ContextGL();
  GLint type = 0;
  GLint readback_format = 0;
  gl->GetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &type);
  if (type == GL_UNSIGNED_BYTE)
    gl->GetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &readback_format);
  if (readback_format != GL_BGRA_EXT)
    readback_format = GL_RGBA;

  optimal_readback_format_ = static_cast<GLenum>(readback_format);
  return optimal_readback_format_;
}

GLRendererCopier::CacheEntry::CacheEntry() {
  object_names.fill(0);
}

GLRendererCopier::CacheEntry::CacheEntry(CacheEntry&& other)
    : purge_count_at_last_use(other.purge_count_at_last_use),
      object_names(other.object_names),
      scaler(std::move(other.scaler)),
      i420_converter(std::move(other.i420_converter)) {
  other.object_names.fill(0);
}

GLRendererCopier::CacheEntry& GLRendererCopier::CacheEntry::operator=(
    CacheEntry&& other) {
  purge_count_at_last_use = other.purge_count_at_last_use;
  object_names = other.object_names;
  other.object_names.fill(0);
  scaler = std::move(other.scaler);
  i420_converter = std::move(other.i420_converter);
  return *this;
}

GLRendererCopier::CacheEntry::~CacheEntry() {
  // Ensure all resources were freed by this point. Resources aren't explicity
  // freed here, in the destructor, because some require access to the GL
  // context. See FreeCachedResources().
  DCHECK(std::find_if(object_names.begin(), object_names.end(),
                      [](GLuint x) { return x != 0; }) == object_names.end());
  DCHECK(!scaler);
  DCHECK(!i420_converter);
}

}  // namespace viz
