// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/common/mojo_shared_buffer_video_frame.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/numerics/safe_math.h"

namespace media {

// static
scoped_refptr<MojoSharedBufferVideoFrame>
MojoSharedBufferVideoFrame::CreateDefaultI420(const gfx::Size& dimensions,
                                              base::TimeDelta timestamp) {
  const VideoPixelFormat format = PIXEL_FORMAT_I420;
  const gfx::Rect visible_rect(dimensions);

  // Since we're allocating memory for the new frame, pad the requested
  // size if necessary so that the requested size does line up on sample
  // boundaries. See related discussion in VideoFrame::CreateFrameInternal().
  const gfx::Size coded_size = DetermineAlignedSize(format, dimensions);
  if (!IsValidConfig(format, STORAGE_MOJO_SHARED_BUFFER, coded_size,
                     visible_rect, dimensions)) {
    LOG(DFATAL) << __func__ << " Invalid config. "
                << ConfigToString(format, STORAGE_MOJO_SHARED_BUFFER,
                                  dimensions, visible_rect, dimensions);
    return nullptr;
  }

  // Allocate a shared memory buffer big enough to hold the desired frame.
  const size_t allocation_size = VideoFrame::AllocationSize(format, coded_size);
  mojo::ScopedSharedBufferHandle handle =
      mojo::SharedBufferHandle::Create(allocation_size);
  if (!handle.is_valid()) {
    DLOG(ERROR) << __func__ << " Unable to allocate memory.";
    return nullptr;
  }

  // Create and initialize the frame. As this is I420 format, the U and V
  // planes have samples for each 2x2 block. The memory is laid out as follows:
  //  - Yplane, full size (each element represents a 1x1 block)
  //  - Uplane, quarter size (each element represents a 2x2 block)
  //  - Vplane, quarter size (each element represents a 2x2 block)
  DCHECK((coded_size.width() % 2 == 0) && (coded_size.height() % 2 == 0));
  return Create(format, coded_size, visible_rect, dimensions, std::move(handle),
                allocation_size, 0 /* y_offset */, coded_size.GetArea(),
                coded_size.GetArea() * 5 / 4, coded_size.width(),
                coded_size.width() / 2, coded_size.width() / 2, timestamp);
}

// static
scoped_refptr<MojoSharedBufferVideoFrame> MojoSharedBufferVideoFrame::Create(
    VideoPixelFormat format,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    mojo::ScopedSharedBufferHandle handle,
    size_t data_size,
    size_t y_offset,
    size_t u_offset,
    size_t v_offset,
    int32_t y_stride,
    int32_t u_stride,
    int32_t v_stride,
    base::TimeDelta timestamp) {
  if (!IsValidConfig(format, STORAGE_MOJO_SHARED_BUFFER, coded_size,
                     visible_rect, natural_size)) {
    LOG(DFATAL) << __func__ << " Invalid config. "
                << ConfigToString(format, STORAGE_MOJO_SHARED_BUFFER,
                                  coded_size, visible_rect, natural_size);
    return nullptr;
  }

  // Validate that the offsets and strides fit in the buffer.
  //
  // We can rely on coded_size.GetArea() being relatively small (compared to the
  // range of an int) due to the IsValidConfig() check above.
  //
  // TODO(sandersd): Allow non-sequential formats.
  if (NumPlanes(format) != 3) {
    DLOG(ERROR) << __func__ << " " << VideoPixelFormatToString(format)
                << " is not supported; only YUV formats are allowed";
    return nullptr;
  }

  if (y_stride < 0 || u_stride < 0 || v_stride < 0) {
    DLOG(ERROR) << __func__ << " Invalid stride";
    return nullptr;
  }

  // Safe given sizeof(size_t) >= sizeof(int32_t).
  size_t y_stride_size_t = y_stride;
  size_t u_stride_size_t = u_stride;
  size_t v_stride_size_t = v_stride;
  if (y_stride_size_t < RowBytes(kYPlane, format, coded_size.width()) ||
      u_stride_size_t < RowBytes(kUPlane, format, coded_size.width()) ||
      v_stride_size_t < RowBytes(kVPlane, format, coded_size.width())) {
    DLOG(ERROR) << __func__ << " Invalid stride";
    return nullptr;
  }

  base::CheckedNumeric<size_t> y_rows =
      Rows(kYPlane, format, coded_size.height());
  base::CheckedNumeric<size_t> u_rows =
      Rows(kUPlane, format, coded_size.height());
  base::CheckedNumeric<size_t> v_rows =
      Rows(kVPlane, format, coded_size.height());

  base::CheckedNumeric<size_t> y_bound = y_rows * y_stride + y_offset;
  base::CheckedNumeric<size_t> u_bound = u_rows * u_stride + u_offset;
  base::CheckedNumeric<size_t> v_bound = v_rows * v_stride + v_offset;

  if (!y_bound.IsValid() || !u_bound.IsValid() || !v_bound.IsValid() ||
      y_bound.ValueOrDie() > data_size || u_bound.ValueOrDie() > data_size ||
      v_bound.ValueOrDie() > data_size) {
    DLOG(ERROR) << __func__ << " Invalid offset";
    return nullptr;
  }

  // Now allocate the frame and initialize it.
  scoped_refptr<MojoSharedBufferVideoFrame> frame(
      new MojoSharedBufferVideoFrame(format, coded_size, visible_rect,
                                     natural_size, std::move(handle), data_size,
                                     timestamp));
  if (!frame->Init(y_stride, u_stride, v_stride, y_offset, u_offset,
                   v_offset)) {
    DLOG(ERROR) << __func__ << " MojoSharedBufferVideoFrame::Init failed.";
    return nullptr;
  }

  return frame;
}

MojoSharedBufferVideoFrame::MojoSharedBufferVideoFrame(
    VideoPixelFormat format,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    mojo::ScopedSharedBufferHandle handle,
    size_t mapped_size,
    base::TimeDelta timestamp)
    : VideoFrame(format,
                 STORAGE_MOJO_SHARED_BUFFER,
                 coded_size,
                 visible_rect,
                 natural_size,
                 timestamp),
      shared_buffer_handle_(std::move(handle)),
      shared_buffer_size_(mapped_size) {
  DCHECK(shared_buffer_handle_.is_valid());
}

bool MojoSharedBufferVideoFrame::Init(int32_t y_stride,
                                      int32_t u_stride,
                                      int32_t v_stride,
                                      size_t y_offset,
                                      size_t u_offset,
                                      size_t v_offset) {
  DCHECK(!shared_buffer_mapping_);
  shared_buffer_mapping_ = shared_buffer_handle_->Map(shared_buffer_size_);
  if (!shared_buffer_mapping_)
    return false;

  set_stride(kYPlane, y_stride);
  set_stride(kUPlane, u_stride);
  set_stride(kVPlane, v_stride);
  offsets_[kYPlane] = y_offset;
  offsets_[kUPlane] = u_offset;
  offsets_[kVPlane] = v_offset;
  set_data(kYPlane, shared_buffer_data() + y_offset);
  set_data(kUPlane, shared_buffer_data() + u_offset);
  set_data(kVPlane, shared_buffer_data() + v_offset);
  return true;
}

MojoSharedBufferVideoFrame::~MojoSharedBufferVideoFrame() {
  // Call |mojo_shared_buffer_done_cb_| to take ownership of
  // |shared_buffer_handle_|.
  if (!mojo_shared_buffer_done_cb_.is_null())
    mojo_shared_buffer_done_cb_.Run(std::move(shared_buffer_handle_),
                                    shared_buffer_size_);
}

size_t MojoSharedBufferVideoFrame::PlaneOffset(size_t plane) const {
  DCHECK(IsValidPlane(plane, format()));
  return offsets_[plane];
}

void MojoSharedBufferVideoFrame::SetMojoSharedBufferDoneCB(
    const MojoSharedBufferDoneCB& mojo_shared_buffer_done_cb) {
  mojo_shared_buffer_done_cb_ = mojo_shared_buffer_done_cb;
}

const mojo::SharedBufferHandle& MojoSharedBufferVideoFrame::Handle() const {
  return shared_buffer_handle_.get();
}

size_t MojoSharedBufferVideoFrame::MappedSize() const {
  return shared_buffer_size_;
}

}  // namespace media
