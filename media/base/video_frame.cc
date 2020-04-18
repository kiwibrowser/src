// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/video_frame.h"

#include <algorithm>
#include <climits>

#include "base/atomic_sequence_num.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/aligned_memory.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "media/base/limits.h"
#include "media/base/timestamp_constants.h"
#include "media/base/video_util.h"
#include "ui/gfx/geometry/point.h"

namespace media {

namespace {

// Helper to privide gfx::Rect::Intersect() as an expression.
gfx::Rect Intersection(gfx::Rect a, const gfx::Rect& b) {
  a.Intersect(b);
  return a;
}

}  // namespace

// Static constexpr class for generating unique identifiers for each VideoFrame.
static base::AtomicSequenceNumber g_unique_id_generator;

static bool IsPowerOfTwo(size_t x) {
  return x != 0 && (x & (x - 1)) == 0;
}

static inline size_t RoundUp(size_t value, size_t alignment) {
  DCHECK(IsPowerOfTwo(alignment));
  return ((value + (alignment - 1)) & ~(alignment - 1));
}

static inline size_t RoundDown(size_t value, size_t alignment) {
  DCHECK(IsPowerOfTwo(alignment));
  return value & ~(alignment - 1);
}

static std::string StorageTypeToString(
    const VideoFrame::StorageType storage_type) {
  switch (storage_type) {
    case VideoFrame::STORAGE_UNKNOWN:
      return "UNKNOWN";
    case VideoFrame::STORAGE_OPAQUE:
      return "OPAQUE";
    case VideoFrame::STORAGE_UNOWNED_MEMORY:
      return "UNOWNED_MEMORY";
    case VideoFrame::STORAGE_OWNED_MEMORY:
      return "OWNED_MEMORY";
    case VideoFrame::STORAGE_SHMEM:
      return "SHMEM";
#if defined(OS_LINUX)
    case VideoFrame::STORAGE_DMABUFS:
      return "DMABUFS";
#endif
    case VideoFrame::STORAGE_MOJO_SHARED_BUFFER:
      return "MOJO_SHARED_BUFFER";
  }

  NOTREACHED() << "Invalid StorageType provided: " << storage_type;
  return "INVALID";
}

// Returns true if |frame| is accesible mapped in the VideoFrame memory space.
// static
static bool IsStorageTypeMappable(VideoFrame::StorageType storage_type) {
  return
#if defined(OS_LINUX)
      // This is not strictly needed but makes explicit that, at VideoFrame
      // level, DmaBufs are not mappable from userspace.
      storage_type != VideoFrame::STORAGE_DMABUFS &&
#endif
      (storage_type == VideoFrame::STORAGE_UNOWNED_MEMORY ||
       storage_type == VideoFrame::STORAGE_OWNED_MEMORY ||
       storage_type == VideoFrame::STORAGE_SHMEM ||
       storage_type == VideoFrame::STORAGE_MOJO_SHARED_BUFFER);
}

// Checks if |source_format| can be wrapped into a |target_format| frame.
static bool AreValidPixelFormatsForWrap(VideoPixelFormat source_format,
                                        VideoPixelFormat target_format) {
  if (source_format == target_format)
    return true;

  // It is possible to add other planar to planar format conversions here if the
  // use case is there.
  return source_format == PIXEL_FORMAT_I420A &&
         target_format == PIXEL_FORMAT_I420;
}

// If it is required to allocate aligned to multiple-of-two size overall for the
// frame of pixel |format|.
bool RequiresEvenSizeAllocation(VideoPixelFormat format) {
  switch (format) {
    case PIXEL_FORMAT_ARGB:
    case PIXEL_FORMAT_XRGB:
    case PIXEL_FORMAT_RGB24:
    case PIXEL_FORMAT_RGB32:
    case PIXEL_FORMAT_Y16:
      return false;
    case PIXEL_FORMAT_NV12:
    case PIXEL_FORMAT_NV21:
    case PIXEL_FORMAT_MT21:
    case PIXEL_FORMAT_I420:
    case PIXEL_FORMAT_MJPEG:
    case PIXEL_FORMAT_YUY2:
    case PIXEL_FORMAT_YV12:
    case PIXEL_FORMAT_I422:
    case PIXEL_FORMAT_I444:
    case PIXEL_FORMAT_YUV420P9:
    case PIXEL_FORMAT_YUV422P9:
    case PIXEL_FORMAT_YUV444P9:
    case PIXEL_FORMAT_YUV420P10:
    case PIXEL_FORMAT_YUV422P10:
    case PIXEL_FORMAT_YUV444P10:
    case PIXEL_FORMAT_YUV420P12:
    case PIXEL_FORMAT_YUV422P12:
    case PIXEL_FORMAT_YUV444P12:
    case PIXEL_FORMAT_I420A:
    case PIXEL_FORMAT_UYVY:
      return true;
    case PIXEL_FORMAT_UNKNOWN:
      break;
  }
  NOTREACHED() << "Unsupported video frame format: " << format;
  return false;
}

// static
bool VideoFrame::IsValidConfig(VideoPixelFormat format,
                               StorageType storage_type,
                               const gfx::Size& coded_size,
                               const gfx::Rect& visible_rect,
                               const gfx::Size& natural_size) {
  // Check maximum limits for all formats.
  int coded_size_area = coded_size.GetCheckedArea().ValueOrDefault(INT_MAX);
  int natural_size_area = natural_size.GetCheckedArea().ValueOrDefault(INT_MAX);
  static_assert(limits::kMaxCanvas < INT_MAX, "");
  if (coded_size_area > limits::kMaxCanvas ||
      coded_size.width() > limits::kMaxDimension ||
      coded_size.height() > limits::kMaxDimension || visible_rect.x() < 0 ||
      visible_rect.y() < 0 || visible_rect.right() > coded_size.width() ||
      visible_rect.bottom() > coded_size.height() ||
      natural_size_area > limits::kMaxCanvas ||
      natural_size.width() > limits::kMaxDimension ||
      natural_size.height() > limits::kMaxDimension)
    return false;

  // TODO(mcasas): Remove parameter |storage_type| when the opaque storage types
  // comply with the checks below. Right now we skip them.
  if (!IsStorageTypeMappable(storage_type))
    return true;

  // Make sure new formats are properly accounted for in the method.
  static_assert(PIXEL_FORMAT_MAX == 26,
                "Added pixel format, please review IsValidConfig()");

  if (format == PIXEL_FORMAT_UNKNOWN) {
    return coded_size.IsEmpty() && visible_rect.IsEmpty() &&
           natural_size.IsEmpty();
  }

  // Check that software-allocated buffer formats are not empty.
  return !coded_size.IsEmpty() && !visible_rect.IsEmpty() &&
         !natural_size.IsEmpty();
}

// static
scoped_refptr<VideoFrame> VideoFrame::CreateFrame(VideoPixelFormat format,
                                                  const gfx::Size& coded_size,
                                                  const gfx::Rect& visible_rect,
                                                  const gfx::Size& natural_size,
                                                  base::TimeDelta timestamp) {
  return CreateFrameInternal(format, coded_size, visible_rect, natural_size,
                             timestamp, false);
}

// static
scoped_refptr<VideoFrame> VideoFrame::CreateZeroInitializedFrame(
    VideoPixelFormat format,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    base::TimeDelta timestamp) {
  return CreateFrameInternal(format, coded_size, visible_rect, natural_size,
                             timestamp, true);
}

// static
scoped_refptr<VideoFrame> VideoFrame::WrapNativeTextures(
    VideoPixelFormat format,
    const gpu::MailboxHolder (&mailbox_holders)[kMaxPlanes],
    ReleaseMailboxCB mailbox_holder_release_cb,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    base::TimeDelta timestamp) {
  if (format != PIXEL_FORMAT_ARGB && format != PIXEL_FORMAT_XRGB &&
      format != PIXEL_FORMAT_RGB32 && format != PIXEL_FORMAT_UYVY &&
      format != PIXEL_FORMAT_NV12 && format != PIXEL_FORMAT_I420) {
    LOG(DFATAL) << "Unsupported pixel format: "
                << VideoPixelFormatToString(format);
    return nullptr;
  }
  const StorageType storage = STORAGE_OPAQUE;
  if (!IsValidConfig(format, storage, coded_size, visible_rect, natural_size)) {
    LOG(DFATAL) << __func__ << " Invalid config."
                << ConfigToString(format, storage, coded_size, visible_rect,
                                  natural_size);
    return nullptr;
  }

  return new VideoFrame(format, storage, coded_size, visible_rect, natural_size,
                        mailbox_holders, std::move(mailbox_holder_release_cb),
                        timestamp);
}

// static
scoped_refptr<VideoFrame> VideoFrame::WrapExternalData(
    VideoPixelFormat format,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    uint8_t* data,
    size_t data_size,
    base::TimeDelta timestamp) {
  return WrapExternalStorage(format, STORAGE_UNOWNED_MEMORY, coded_size,
                             visible_rect, natural_size, data, data_size,
                             timestamp, base::SharedMemoryHandle(), 0);
}

// static
scoped_refptr<VideoFrame> VideoFrame::WrapExternalSharedMemory(
    VideoPixelFormat format,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    uint8_t* data,
    size_t data_size,
    base::SharedMemoryHandle handle,
    size_t data_offset,
    base::TimeDelta timestamp) {
  return WrapExternalStorage(format, STORAGE_SHMEM, coded_size, visible_rect,
                             natural_size, data, data_size, timestamp, handle,
                             data_offset);
}

// static
scoped_refptr<VideoFrame> VideoFrame::WrapExternalYuvData(
    VideoPixelFormat format,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    int32_t y_stride,
    int32_t u_stride,
    int32_t v_stride,
    uint8_t* y_data,
    uint8_t* u_data,
    uint8_t* v_data,
    base::TimeDelta timestamp) {
  const StorageType storage = STORAGE_UNOWNED_MEMORY;
  if (!IsValidConfig(format, storage, coded_size, visible_rect, natural_size)) {
    LOG(DFATAL) << __func__ << " Invalid config."
                << ConfigToString(format, storage, coded_size, visible_rect,
                                  natural_size);
    return nullptr;
  }

  scoped_refptr<VideoFrame> frame(new VideoFrame(
      format, storage, coded_size, visible_rect, natural_size, timestamp));
  frame->strides_[kYPlane] = y_stride;
  frame->strides_[kUPlane] = u_stride;
  frame->strides_[kVPlane] = v_stride;
  frame->data_[kYPlane] = y_data;
  frame->data_[kUPlane] = u_data;
  frame->data_[kVPlane] = v_data;
  return frame;
}

// static
scoped_refptr<VideoFrame> VideoFrame::WrapExternalYuvaData(
    VideoPixelFormat format,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    int32_t y_stride,
    int32_t u_stride,
    int32_t v_stride,
    int32_t a_stride,
    uint8_t* y_data,
    uint8_t* u_data,
    uint8_t* v_data,
    uint8_t* a_data,
    base::TimeDelta timestamp) {
  const StorageType storage = STORAGE_UNOWNED_MEMORY;
  if (!IsValidConfig(format, storage, coded_size, visible_rect, natural_size)) {
    LOG(DFATAL) << __func__ << " Invalid config."
                << ConfigToString(format, storage, coded_size, visible_rect,
                                  natural_size);
    return nullptr;
  }

  if (NumPlanes(format) != 4) {
    LOG(DFATAL) << "Expecting Y, U, V and A planes to be present for the video"
                << " format.";
    return nullptr;
  }

  scoped_refptr<VideoFrame> frame(new VideoFrame(
      format, storage, coded_size, visible_rect, natural_size, timestamp));
  frame->strides_[kYPlane] = y_stride;
  frame->strides_[kUPlane] = u_stride;
  frame->strides_[kVPlane] = v_stride;
  frame->strides_[kAPlane] = a_stride;
  frame->data_[kYPlane] = y_data;
  frame->data_[kUPlane] = u_data;
  frame->data_[kVPlane] = v_data;
  frame->data_[kAPlane] = a_data;
  return frame;
}

#if defined(OS_LINUX)
// static
scoped_refptr<VideoFrame> VideoFrame::WrapExternalDmabufs(
    VideoPixelFormat format,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    const std::vector<int>& dmabuf_fds,
    base::TimeDelta timestamp) {
  const StorageType storage = STORAGE_DMABUFS;
  if (!IsValidConfig(format, storage, coded_size, visible_rect, natural_size)) {
    LOG(DFATAL) << __func__ << " Invalid config."
                << ConfigToString(format, storage, coded_size, visible_rect,
                                  natural_size);
    return nullptr;
  }

  gpu::MailboxHolder mailbox_holders[kMaxPlanes];
  scoped_refptr<VideoFrame> frame =
      new VideoFrame(format, storage, coded_size, visible_rect, natural_size,
                     mailbox_holders, ReleaseMailboxCB(), timestamp);
  if (!frame || !frame->DuplicateFileDescriptors(dmabuf_fds)) {
    LOG(DFATAL) << __func__ << " Couldn't duplicate fds.";
    return nullptr;
  }
  return frame;
}
#endif

#if defined(OS_MACOSX)
// static
scoped_refptr<VideoFrame> VideoFrame::WrapCVPixelBuffer(
    CVPixelBufferRef cv_pixel_buffer,
    base::TimeDelta timestamp) {
  DCHECK(cv_pixel_buffer);
  DCHECK(CFGetTypeID(cv_pixel_buffer) == CVPixelBufferGetTypeID());

  const OSType cv_format = CVPixelBufferGetPixelFormatType(cv_pixel_buffer);
  VideoPixelFormat format;
  // There are very few compatible CV pixel formats, so just check each.
  if (cv_format == kCVPixelFormatType_420YpCbCr8Planar) {
    format = PIXEL_FORMAT_I420;
  } else if (cv_format == kCVPixelFormatType_444YpCbCr8) {
    format = PIXEL_FORMAT_I444;
  } else if (cv_format == '420v') {
    // TODO(jfroy): Use kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange when the
    // minimum OS X and iOS SDKs permits it.
    format = PIXEL_FORMAT_NV12;
  } else {
    LOG(DFATAL) << "CVPixelBuffer format not supported: " << cv_format;
    return nullptr;
  }

  const gfx::Size coded_size(CVImageBufferGetEncodedSize(cv_pixel_buffer));
  const gfx::Rect visible_rect(CVImageBufferGetCleanRect(cv_pixel_buffer));
  const gfx::Size natural_size(CVImageBufferGetDisplaySize(cv_pixel_buffer));
  const StorageType storage = STORAGE_UNOWNED_MEMORY;

  if (!IsValidConfig(format, storage, coded_size, visible_rect, natural_size)) {
    LOG(DFATAL) << __func__ << " Invalid config."
                << ConfigToString(format, storage, coded_size, visible_rect,
                                  natural_size);
    return nullptr;
  }

  scoped_refptr<VideoFrame> frame(new VideoFrame(
      format, storage, coded_size, visible_rect, natural_size, timestamp));

  frame->cv_pixel_buffer_.reset(cv_pixel_buffer, base::scoped_policy::RETAIN);
  return frame;
}
#endif

// static
scoped_refptr<VideoFrame> VideoFrame::WrapVideoFrame(
    const scoped_refptr<VideoFrame>& frame,
    VideoPixelFormat format,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size) {
  // Frames with textures need mailbox info propagated, and there's no support
  // for that here yet, see http://crbug/362521.
  CHECK(!frame->HasTextures());
  DCHECK(frame->visible_rect().Contains(visible_rect));

  if (!AreValidPixelFormatsForWrap(frame->format(), format)) {
    LOG(DFATAL) << __func__ << " Invalid format conversion."
                << VideoPixelFormatToString(frame->format()) << " to "
                << VideoPixelFormatToString(format);
    return nullptr;
  }

  if (!IsValidConfig(format, frame->storage_type(), frame->coded_size(),
                     visible_rect, natural_size)) {
    LOG(DFATAL) << __func__ << " Invalid config."
                << ConfigToString(format, frame->storage_type(),
                                  frame->coded_size(), visible_rect,
                                  natural_size);
    return nullptr;
  }

  scoped_refptr<VideoFrame> wrapping_frame(
      new VideoFrame(format, frame->storage_type(), frame->coded_size(),
                     visible_rect, natural_size, frame->timestamp()));

  // Copy all metadata to the wrapped frame.
  wrapping_frame->metadata()->MergeMetadataFrom(frame->metadata());

  for (size_t i = 0; i < NumPlanes(format); ++i) {
    wrapping_frame->strides_[i] = frame->stride(i);
    wrapping_frame->data_[i] = frame->data(i);
  }

#if defined(OS_LINUX)
  // If there are any |dmabuf_fds_| plugged in, we should duplicate them.
  if (frame->storage_type() == STORAGE_DMABUFS) {
    std::vector<int> original_fds;
    for (size_t i = 0; i < kMaxPlanes; ++i)
      original_fds.push_back(frame->DmabufFd(i));
    if (!wrapping_frame->DuplicateFileDescriptors(original_fds)) {
      LOG(DFATAL) << __func__ << " Couldn't duplicate fds.";
      return nullptr;
    }
  }
#endif

  if (frame->storage_type() == STORAGE_SHMEM)
    wrapping_frame->AddSharedMemoryHandle(frame->shared_memory_handle_);

  return wrapping_frame;
}

// static
scoped_refptr<VideoFrame> VideoFrame::CreateEOSFrame() {
  scoped_refptr<VideoFrame> frame =
      new VideoFrame(PIXEL_FORMAT_UNKNOWN, STORAGE_UNKNOWN, gfx::Size(),
                     gfx::Rect(), gfx::Size(), kNoTimestamp);
  frame->metadata()->SetBoolean(VideoFrameMetadata::END_OF_STREAM, true);
  return frame;
}

// static
scoped_refptr<VideoFrame> VideoFrame::CreateColorFrame(
    const gfx::Size& size,
    uint8_t y,
    uint8_t u,
    uint8_t v,
    base::TimeDelta timestamp) {
  scoped_refptr<VideoFrame> frame =
      CreateFrame(PIXEL_FORMAT_I420, size, gfx::Rect(size), size, timestamp);
  FillYUV(frame.get(), y, u, v);
  return frame;
}

// static
scoped_refptr<VideoFrame> VideoFrame::CreateBlackFrame(const gfx::Size& size) {
  const uint8_t kBlackY = 0x00;
  const uint8_t kBlackUV = 0x80;
  const base::TimeDelta kZero;
  return CreateColorFrame(size, kBlackY, kBlackUV, kBlackUV, kZero);
}

// static
scoped_refptr<VideoFrame> VideoFrame::CreateTransparentFrame(
    const gfx::Size& size) {
  const uint8_t kBlackY = 0x00;
  const uint8_t kBlackUV = 0x00;
  const uint8_t kTransparentA = 0x00;
  const base::TimeDelta kZero;
  scoped_refptr<VideoFrame> frame =
      CreateFrame(PIXEL_FORMAT_I420A, size, gfx::Rect(size), size, kZero);
  FillYUVA(frame.get(), kBlackY, kBlackUV, kBlackUV, kTransparentA);
  return frame;
}

// static
size_t VideoFrame::NumPlanes(VideoPixelFormat format) {
  switch (format) {
    case PIXEL_FORMAT_UYVY:
    case PIXEL_FORMAT_YUY2:
    case PIXEL_FORMAT_ARGB:
    case PIXEL_FORMAT_XRGB:
    case PIXEL_FORMAT_RGB24:
    case PIXEL_FORMAT_RGB32:
    case PIXEL_FORMAT_MJPEG:
    case PIXEL_FORMAT_Y16:
      return 1;
    case PIXEL_FORMAT_NV12:
    case PIXEL_FORMAT_NV21:
    case PIXEL_FORMAT_MT21:
      return 2;
    case PIXEL_FORMAT_I420:
    case PIXEL_FORMAT_YV12:
    case PIXEL_FORMAT_I422:
    case PIXEL_FORMAT_I444:
    case PIXEL_FORMAT_YUV420P9:
    case PIXEL_FORMAT_YUV422P9:
    case PIXEL_FORMAT_YUV444P9:
    case PIXEL_FORMAT_YUV420P10:
    case PIXEL_FORMAT_YUV422P10:
    case PIXEL_FORMAT_YUV444P10:
    case PIXEL_FORMAT_YUV420P12:
    case PIXEL_FORMAT_YUV422P12:
    case PIXEL_FORMAT_YUV444P12:
      return 3;
    case PIXEL_FORMAT_I420A:
      return 4;
    case PIXEL_FORMAT_UNKNOWN:
      break;
  }
  NOTREACHED() << "Unsupported video frame format: " << format;
  return 0;
}

// static
size_t VideoFrame::AllocationSize(VideoPixelFormat format,
                                  const gfx::Size& coded_size) {
  size_t total = 0;
  for (size_t i = 0; i < NumPlanes(format); ++i)
    total += PlaneSize(format, i, coded_size).GetArea();
  return total;
}

// static
gfx::Size VideoFrame::PlaneSize(VideoPixelFormat format,
                                size_t plane,
                                const gfx::Size& coded_size) {
  DCHECK(IsValidPlane(plane, format));

  int width = coded_size.width();
  int height = coded_size.height();
  if (RequiresEvenSizeAllocation(format)) {
    // Align to multiple-of-two size overall. This ensures that non-subsampled
    // planes can be addressed by pixel with the same scaling as the subsampled
    // planes.
    width = RoundUp(width, 2);
    height = RoundUp(height, 2);
  }

  const gfx::Size subsample = SampleSize(format, plane);
  DCHECK(width % subsample.width() == 0);
  DCHECK(height % subsample.height() == 0);
  return gfx::Size(BytesPerElement(format, plane) * width / subsample.width(),
                   height / subsample.height());
}

// static
int VideoFrame::PlaneHorizontalBitsPerPixel(VideoPixelFormat format,
                                            size_t plane) {
  DCHECK(IsValidPlane(plane, format));
  const int bits_per_element = 8 * BytesPerElement(format, plane);
  const int horiz_pixels_per_element = SampleSize(format, plane).width();
  DCHECK_EQ(bits_per_element % horiz_pixels_per_element, 0);
  return bits_per_element / horiz_pixels_per_element;
}

// static
int VideoFrame::PlaneBitsPerPixel(VideoPixelFormat format, size_t plane) {
  DCHECK(IsValidPlane(plane, format));
  return PlaneHorizontalBitsPerPixel(format, plane) /
      SampleSize(format, plane).height();
}

// static
size_t VideoFrame::RowBytes(size_t plane, VideoPixelFormat format, int width) {
  DCHECK(IsValidPlane(plane, format));
  return BytesPerElement(format, plane) * Columns(plane, format, width);
}

// static
int VideoFrame::BytesPerElement(VideoPixelFormat format, size_t plane) {
  DCHECK(IsValidPlane(plane, format));
  switch (format) {
    case PIXEL_FORMAT_ARGB:
    case PIXEL_FORMAT_XRGB:
    case PIXEL_FORMAT_RGB32:
      return 4;
    case PIXEL_FORMAT_RGB24:
      return 3;
    case PIXEL_FORMAT_Y16:
    case PIXEL_FORMAT_UYVY:
    case PIXEL_FORMAT_YUY2:
    case PIXEL_FORMAT_YUV420P9:
    case PIXEL_FORMAT_YUV422P9:
    case PIXEL_FORMAT_YUV444P9:
    case PIXEL_FORMAT_YUV420P10:
    case PIXEL_FORMAT_YUV422P10:
    case PIXEL_FORMAT_YUV444P10:
    case PIXEL_FORMAT_YUV420P12:
    case PIXEL_FORMAT_YUV422P12:
    case PIXEL_FORMAT_YUV444P12:
      return 2;
    case PIXEL_FORMAT_NV12:
    case PIXEL_FORMAT_NV21:
    case PIXEL_FORMAT_MT21: {
      static const int bytes_per_element[] = {1, 2};
      DCHECK_LT(plane, arraysize(bytes_per_element));
      return bytes_per_element[plane];
    }
    case PIXEL_FORMAT_YV12:
    case PIXEL_FORMAT_I420:
    case PIXEL_FORMAT_I422:
    case PIXEL_FORMAT_I420A:
    case PIXEL_FORMAT_I444:
      return 1;
    case PIXEL_FORMAT_MJPEG:
      return 0;
    case PIXEL_FORMAT_UNKNOWN:
      break;
  }
  NOTREACHED();
  return 0;
}

// static
size_t VideoFrame::Rows(size_t plane, VideoPixelFormat format, int height) {
  DCHECK(IsValidPlane(plane, format));
  const int sample_height = SampleSize(format, plane).height();
  return RoundUp(height, sample_height) / sample_height;
}

// static
size_t VideoFrame::Columns(size_t plane, VideoPixelFormat format, int width) {
  DCHECK(IsValidPlane(plane, format));
  const int sample_width = SampleSize(format, plane).width();
  return RoundUp(width, sample_width) / sample_width;
}

// static
void VideoFrame::HashFrameForTesting(base::MD5Context* context,
                                     const scoped_refptr<VideoFrame>& frame) {
  DCHECK(context);
  for (size_t plane = 0; plane < NumPlanes(frame->format()); ++plane) {
    for (int row = 0; row < frame->rows(plane); ++row) {
      base::MD5Update(
          context,
          base::StringPiece(reinterpret_cast<char*>(frame->data(plane) +
                                                    frame->stride(plane) * row),
                            frame->row_bytes(plane)));
    }
  }
}

bool VideoFrame::IsMappable() const {
  return IsStorageTypeMappable(storage_type_);
}

bool VideoFrame::HasTextures() const {
  return !mailbox_holders_[0].mailbox.IsZero();
}

size_t VideoFrame::NumTextures() const {
  if (!HasTextures())
    return 0;

  size_t i = 0;
  for (; i < NumPlanes(format_); ++i) {
    if (mailbox_holders_[i].mailbox.IsZero()) {
      return i;
    }
  }
  return i;
}

gfx::ColorSpace VideoFrame::ColorSpace() const {
  if (color_space_ == gfx::ColorSpace()) {
    int videoframe_color_space;
    if (metadata()->GetInteger(media::VideoFrameMetadata::COLOR_SPACE,
                               &videoframe_color_space)) {
      switch (videoframe_color_space) {
        case media::COLOR_SPACE_JPEG:
          return gfx::ColorSpace::CreateJpeg();
        case media::COLOR_SPACE_HD_REC709:
          return gfx::ColorSpace::CreateREC709();
        case media::COLOR_SPACE_SD_REC601:
          return gfx::ColorSpace::CreateREC601();
        default:
          break;
      }
    }
  }
  return color_space_;
}

void VideoFrame::set_color_space(const gfx::ColorSpace& color_space) {
  color_space_ = color_space;
}

int VideoFrame::stride(size_t plane) const {
  DCHECK(IsValidPlane(plane, format_));
  return strides_[plane];
}

int VideoFrame::row_bytes(size_t plane) const {
  return RowBytes(plane, format_, coded_size_.width());
}

int VideoFrame::rows(size_t plane) const {
  return Rows(plane, format_, coded_size_.height());
}

const uint8_t* VideoFrame::data(size_t plane) const {
  DCHECK(IsValidPlane(plane, format_));
  DCHECK(IsMappable());
  return data_[plane];
}

uint8_t* VideoFrame::data(size_t plane) {
  DCHECK(IsValidPlane(plane, format_));
  DCHECK(IsMappable());
  return data_[plane];
}

const uint8_t* VideoFrame::visible_data(size_t plane) const {
  DCHECK(IsValidPlane(plane, format_));
  DCHECK(IsMappable());

  // Calculate an offset that is properly aligned for all planes.
  const gfx::Size alignment = CommonAlignment(format_);
  const gfx::Point offset(RoundDown(visible_rect_.x(), alignment.width()),
                          RoundDown(visible_rect_.y(), alignment.height()));

  const gfx::Size subsample = SampleSize(format_, plane);
  DCHECK(offset.x() % subsample.width() == 0);
  DCHECK(offset.y() % subsample.height() == 0);
  return data(plane) +
         stride(plane) * (offset.y() / subsample.height()) +  // Row offset.
         BytesPerElement(format_, plane) *                    // Column offset.
             (offset.x() / subsample.width());
}

uint8_t* VideoFrame::visible_data(size_t plane) {
  return const_cast<uint8_t*>(
      static_cast<const VideoFrame*>(this)->visible_data(plane));
}

const gpu::MailboxHolder&
VideoFrame::mailbox_holder(size_t texture_index) const {
  DCHECK(HasTextures());
  DCHECK(IsValidPlane(texture_index, format_));
  return mailbox_holders_[texture_index];
}

base::SharedMemoryHandle VideoFrame::shared_memory_handle() const {
  DCHECK_EQ(storage_type_, STORAGE_SHMEM);
  DCHECK(shared_memory_handle_.IsValid());
  return shared_memory_handle_;
}

size_t VideoFrame::shared_memory_offset() const {
  DCHECK_EQ(storage_type_, STORAGE_SHMEM);
  DCHECK(shared_memory_handle_.IsValid());
  return shared_memory_offset_;
}

#if defined(OS_LINUX)
int VideoFrame::DmabufFd(size_t plane) const {
  DCHECK_EQ(storage_type_, STORAGE_DMABUFS);
  DCHECK(IsValidPlane(plane, format_));
  return dmabuf_fds_[plane].get();
}

bool VideoFrame::DuplicateFileDescriptors(const std::vector<int>& in_fds) {
  // TODO(mcasas): Support offsets for e.g. multiplanar inside a single |in_fd|.

  storage_type_ = STORAGE_DMABUFS;
  // TODO(posciak): This is not exactly correct, it's possible for one
  // buffer to contain more than one plane.
  if (in_fds.size() != NumPlanes(format_)) {
    LOG(FATAL) << "Not enough dmabuf fds provided, got: " <<  in_fds.size()
               << ", expected: " << NumPlanes(format_);
    return false;
  }

  // Make sure that all fds are closed if any dup() fails,
  base::ScopedFD temp_dmabuf_fds[kMaxPlanes];
  for (size_t i = 0; i < in_fds.size(); ++i) {
    temp_dmabuf_fds[i] = base::ScopedFD(HANDLE_EINTR(dup(in_fds[i])));
    if (!temp_dmabuf_fds[i].is_valid()) {
      DPLOG(ERROR) << "Failed duplicating a dmabuf fd";
      return false;
    }
  }
  for (size_t i = 0; i < kMaxPlanes; ++i)
    dmabuf_fds_[i] = std::move(temp_dmabuf_fds[i]);

  return true;
}
#endif

void VideoFrame::AddSharedMemoryHandle(base::SharedMemoryHandle handle) {
  storage_type_ = STORAGE_SHMEM;
  shared_memory_handle_ = handle;
}

#if defined(OS_MACOSX)
CVPixelBufferRef VideoFrame::CvPixelBuffer() const {
  return cv_pixel_buffer_.get();
}
#endif

void VideoFrame::SetReleaseMailboxCB(ReleaseMailboxCB release_mailbox_cb) {
  DCHECK(!release_mailbox_cb.is_null());
  DCHECK(mailbox_holders_release_cb_.is_null());
  mailbox_holders_release_cb_ = std::move(release_mailbox_cb);
}

bool VideoFrame::HasReleaseMailboxCB() const {
  return !mailbox_holders_release_cb_.is_null();
}

void VideoFrame::AddDestructionObserver(base::OnceClosure callback) {
  DCHECK(!callback.is_null());
  done_callbacks_.push_back(std::move(callback));
}

gpu::SyncToken VideoFrame::UpdateReleaseSyncToken(SyncTokenClient* client) {
  DCHECK(HasTextures());
  base::AutoLock locker(release_sync_token_lock_);
  // Must wait on the previous sync point before inserting a new sync point so
  // that |mailbox_holders_release_cb_| guarantees the previous sync point
  // occurred when it waits on |release_sync_token_|.
  if (release_sync_token_.HasData())
    client->WaitSyncToken(release_sync_token_);
  client->GenerateSyncToken(&release_sync_token_);
  return release_sync_token_;
}

std::string VideoFrame::AsHumanReadableString() {
  if (metadata()->IsTrue(media::VideoFrameMetadata::END_OF_STREAM))
    return "end of stream";

  std::ostringstream s;
  s << ConfigToString(format_, storage_type_, coded_size_, visible_rect_,
                      natural_size_)
    << " timestamp:" << timestamp_.InMicroseconds();
  return s.str();
}

size_t VideoFrame::BitDepth() const {
  switch (format_) {
    case media::PIXEL_FORMAT_UNKNOWN:
      NOTREACHED();
      FALLTHROUGH;
    case media::PIXEL_FORMAT_I420:
    case media::PIXEL_FORMAT_YV12:
    case media::PIXEL_FORMAT_I422:
    case media::PIXEL_FORMAT_I420A:
    case media::PIXEL_FORMAT_I444:
    case media::PIXEL_FORMAT_NV12:
    case media::PIXEL_FORMAT_NV21:
    case media::PIXEL_FORMAT_UYVY:
    case media::PIXEL_FORMAT_YUY2:
    case media::PIXEL_FORMAT_ARGB:
    case media::PIXEL_FORMAT_XRGB:
    case media::PIXEL_FORMAT_RGB24:
    case media::PIXEL_FORMAT_RGB32:
    case media::PIXEL_FORMAT_MJPEG:
    case media::PIXEL_FORMAT_MT21:
      return 8;
    case media::PIXEL_FORMAT_YUV420P9:
    case media::PIXEL_FORMAT_YUV422P9:
    case media::PIXEL_FORMAT_YUV444P9:
      return 9;
    case media::PIXEL_FORMAT_YUV420P10:
    case media::PIXEL_FORMAT_YUV422P10:
    case media::PIXEL_FORMAT_YUV444P10:
      return 10;
    case media::PIXEL_FORMAT_YUV420P12:
    case media::PIXEL_FORMAT_YUV422P12:
    case media::PIXEL_FORMAT_YUV444P12:
      return 12;
    case media::PIXEL_FORMAT_Y16:
      return 16;
  }
  NOTREACHED();
  return 0;
}

// static
scoped_refptr<VideoFrame> VideoFrame::WrapExternalStorage(
    VideoPixelFormat format,
    StorageType storage_type,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    uint8_t* data,
    size_t data_size,
    base::TimeDelta timestamp,
    base::SharedMemoryHandle handle,
    size_t data_offset) {
  DCHECK(IsStorageTypeMappable(storage_type));

  // TODO(miu): This function should support any pixel format.
  // http://crbug.com/555909
  if (format != PIXEL_FORMAT_I420 && format != PIXEL_FORMAT_Y16 &&
      format != PIXEL_FORMAT_ARGB) {
    LOG(DFATAL) << "Only PIXEL_FORMAT_I420, PIXEL_FORMAT_Y16, and "
                   "PIXEL_FORMAT_ARGB formats are supported: "
                << VideoPixelFormatToString(format);
    return nullptr;
  }

  if (!IsValidConfig(format, storage_type, coded_size, visible_rect,
                     natural_size)) {
    LOG(DFATAL) << __func__ << " Invalid config."
                << ConfigToString(format, storage_type, coded_size,
                                  visible_rect, natural_size);
    return nullptr;
  }

  scoped_refptr<VideoFrame> frame;
  if (storage_type == STORAGE_SHMEM) {
    frame = new VideoFrame(format, storage_type, coded_size, visible_rect,
                           natural_size, timestamp, handle, data_offset);
  } else {
    frame = new VideoFrame(format, storage_type, coded_size, visible_rect,
                           natural_size, timestamp);
  }
  switch (NumPlanes(format)) {
    case 1:
      frame->strides_[kYPlane] = RowBytes(kYPlane, format, coded_size.width());
      frame->data_[kYPlane] = data;
      return frame;
    case 3:
      DCHECK_EQ(format, PIXEL_FORMAT_I420);
      // TODO(miu): This always rounds widths down, whereas
      // VideoFrame::RowBytes() always rounds up.  This inconsistency must be
      // resolved.  Perhaps a CommonAlignment() check should be made in
      // IsValidConfig()?
      // http://crbug.com/555909
      frame->strides_[kYPlane] = RowBytes(kYPlane, format, coded_size.width());
      frame->data_[kYPlane] = data;
      frame->strides_[kVPlane] = coded_size.width() / 2;
      frame->data_[kVPlane] = data + (coded_size.GetArea() * 5 / 4);
      frame->strides_[kUPlane] = coded_size.width() / 2;
      frame->data_[kUPlane] = data + coded_size.GetArea();
      return frame;
    default:
      LOG(DFATAL) << "Invalid number of planes: " << NumPlanes(format)
                  << " in format: " << VideoPixelFormatToString(format);
      return nullptr;
  }
}

VideoFrame::VideoFrame(VideoPixelFormat format,
                       StorageType storage_type,
                       const gfx::Size& coded_size,
                       const gfx::Rect& visible_rect,
                       const gfx::Size& natural_size,
                       base::TimeDelta timestamp)
    : format_(format),
      storage_type_(storage_type),
      coded_size_(coded_size),
      visible_rect_(Intersection(visible_rect, gfx::Rect(coded_size))),
      natural_size_(natural_size),
      shared_memory_offset_(0),
      timestamp_(timestamp),
      unique_id_(g_unique_id_generator.GetNext()) {
  DCHECK(IsValidConfig(format_, storage_type, coded_size_, visible_rect_,
                       natural_size_));
  DCHECK(visible_rect_ == visible_rect)
      << "visible_rect " << visible_rect.ToString() << " exceeds coded_size "
      << coded_size.ToString();
  memset(&mailbox_holders_, 0, sizeof(mailbox_holders_));
  memset(&strides_, 0, sizeof(strides_));
  memset(&data_, 0, sizeof(data_));
}

VideoFrame::~VideoFrame() {
  if (!mailbox_holders_release_cb_.is_null()) {
    gpu::SyncToken release_sync_token;
    {
      // To ensure that changes to |release_sync_token_| are visible on this
      // thread (imply a memory barrier).
      base::AutoLock locker(release_sync_token_lock_);
      release_sync_token = release_sync_token_;
    }
    base::ResetAndReturn(&mailbox_holders_release_cb_).Run(release_sync_token);
  }

  for (auto& callback : done_callbacks_)
    base::ResetAndReturn(&callback).Run();
}

// static
std::string VideoFrame::ConfigToString(const VideoPixelFormat format,
                                       const StorageType storage_type,
                                       const gfx::Size& coded_size,
                                       const gfx::Rect& visible_rect,
                                       const gfx::Size& natural_size) {
  return base::StringPrintf(
      "format:%s storage_type:%s coded_size:%s visible_rect:%s natural_size:%s",
      VideoPixelFormatToString(format).c_str(),
      StorageTypeToString(storage_type).c_str(), coded_size.ToString().c_str(),
      visible_rect.ToString().c_str(), natural_size.ToString().c_str());
}

// static
bool VideoFrame::IsValidPlane(size_t plane, VideoPixelFormat format) {
  DCHECK_LE(NumPlanes(format), static_cast<size_t>(kMaxPlanes));
  return (plane < NumPlanes(format));
}

// static
gfx::Size VideoFrame::DetermineAlignedSize(VideoPixelFormat format,
                                           const gfx::Size& dimensions) {
  const gfx::Size alignment = CommonAlignment(format);
  const gfx::Size adjusted =
      gfx::Size(RoundUp(dimensions.width(), alignment.width()),
                RoundUp(dimensions.height(), alignment.height()));
  DCHECK((adjusted.width() % alignment.width() == 0) &&
         (adjusted.height() % alignment.height() == 0));
  return adjusted;
}

void VideoFrame::set_data(size_t plane, uint8_t* ptr) {
  DCHECK(IsValidPlane(plane, format_));
  DCHECK(ptr);
  data_[plane] = ptr;
}

void VideoFrame::set_stride(size_t plane, int stride) {
  DCHECK(IsValidPlane(plane, format_));
  DCHECK_GT(stride, 0);
  strides_[plane] = stride;
}

VideoFrame::VideoFrame(VideoPixelFormat format,
                       StorageType storage_type,
                       const gfx::Size& coded_size,
                       const gfx::Rect& visible_rect,
                       const gfx::Size& natural_size,
                       base::TimeDelta timestamp,
                       base::SharedMemoryHandle handle,
                       size_t shared_memory_offset)
    : VideoFrame(format,
                 storage_type,
                 coded_size,
                 visible_rect,
                 natural_size,
                 timestamp) {
  DCHECK_EQ(storage_type, STORAGE_SHMEM);
  AddSharedMemoryHandle(handle);
  shared_memory_offset_ = shared_memory_offset;
}

VideoFrame::VideoFrame(VideoPixelFormat format,
                       StorageType storage_type,
                       const gfx::Size& coded_size,
                       const gfx::Rect& visible_rect,
                       const gfx::Size& natural_size,
                       const gpu::MailboxHolder (&mailbox_holders)[kMaxPlanes],
                       ReleaseMailboxCB mailbox_holder_release_cb,
                       base::TimeDelta timestamp)
    : VideoFrame(format,
                 storage_type,
                 coded_size,
                 visible_rect,
                 natural_size,
                 timestamp) {
  memcpy(&mailbox_holders_, mailbox_holders, sizeof(mailbox_holders_));
  mailbox_holders_release_cb_ = std::move(mailbox_holder_release_cb);
}

// static
scoped_refptr<VideoFrame> VideoFrame::CreateFrameInternal(
    VideoPixelFormat format,
    const gfx::Size& coded_size,
    const gfx::Rect& visible_rect,
    const gfx::Size& natural_size,
    base::TimeDelta timestamp,
    bool zero_initialize_memory) {
  // Since we're creating a new frame (and allocating memory for it ourselves),
  // we can pad the requested |coded_size| if necessary if the request does not
  // line up on sample boundaries. See discussion at http://crrev.com/1240833003
  const gfx::Size new_coded_size = DetermineAlignedSize(format, coded_size);
  const StorageType storage = STORAGE_OWNED_MEMORY;
  if (!IsValidConfig(format, storage, new_coded_size, visible_rect,
                     natural_size)) {
    LOG(DFATAL) << __func__ << " Invalid config."
                << ConfigToString(format, storage, coded_size, visible_rect,
                                  natural_size);
    return nullptr;
  }

  scoped_refptr<VideoFrame> frame(new VideoFrame(
      format, storage, new_coded_size, visible_rect, natural_size, timestamp));
  frame->AllocateMemory(zero_initialize_memory);
  return frame;
}

// static
gfx::Size VideoFrame::SampleSize(VideoPixelFormat format, size_t plane) {
  DCHECK(IsValidPlane(plane, format));

  switch (plane) {
    case kYPlane:  // and kARGBPlane:
    case kAPlane:
      return gfx::Size(1, 1);

    case kUPlane:  // and kUVPlane:
    case kVPlane:
      switch (format) {
        case PIXEL_FORMAT_I444:
        case PIXEL_FORMAT_YUV444P9:
        case PIXEL_FORMAT_YUV444P10:
        case PIXEL_FORMAT_YUV444P12:
        case PIXEL_FORMAT_Y16:
          return gfx::Size(1, 1);

        case PIXEL_FORMAT_I422:
        case PIXEL_FORMAT_YUV422P9:
        case PIXEL_FORMAT_YUV422P10:
        case PIXEL_FORMAT_YUV422P12:
          return gfx::Size(2, 1);

        case PIXEL_FORMAT_YV12:
        case PIXEL_FORMAT_I420:
        case PIXEL_FORMAT_I420A:
        case PIXEL_FORMAT_NV12:
        case PIXEL_FORMAT_NV21:
        case PIXEL_FORMAT_MT21:
        case PIXEL_FORMAT_YUV420P9:
        case PIXEL_FORMAT_YUV420P10:
        case PIXEL_FORMAT_YUV420P12:
          return gfx::Size(2, 2);

        case PIXEL_FORMAT_UNKNOWN:
        case PIXEL_FORMAT_UYVY:
        case PIXEL_FORMAT_YUY2:
        case PIXEL_FORMAT_ARGB:
        case PIXEL_FORMAT_XRGB:
        case PIXEL_FORMAT_RGB24:
        case PIXEL_FORMAT_RGB32:
        case PIXEL_FORMAT_MJPEG:
          break;
      }
  }
  NOTREACHED();
  return gfx::Size();
}

// static
gfx::Size VideoFrame::CommonAlignment(VideoPixelFormat format) {
  int max_sample_width = 0;
  int max_sample_height = 0;
  for (size_t plane = 0; plane < NumPlanes(format); ++plane) {
    const gfx::Size sample_size = SampleSize(format, plane);
    max_sample_width = std::max(max_sample_width, sample_size.width());
    max_sample_height = std::max(max_sample_height, sample_size.height());
  }
  return gfx::Size(max_sample_width, max_sample_height);
}

void VideoFrame::AllocateMemory(bool zero_initialize_memory) {
  DCHECK_EQ(storage_type_, STORAGE_OWNED_MEMORY);
  static_assert(0 == kYPlane, "y plane data must be index 0");

  size_t data_size = 0;
  size_t offset[kMaxPlanes];

  // Tightly pack if it is single planar.
  if (NumPlanes(format_) == 1) {
    data_size = AllocationSize(format_, coded_size_);
    offset[0] = 0;
    strides_[0] = row_bytes(0);
  } else {
    for (size_t plane = 0; plane < NumPlanes(format_); ++plane) {
      // These values were chosen to mirror ffmpeg's get_video_buffer().
      // TODO(dalecurtis): This should be configurable; eventually ffmpeg wants
      // us to use av_cpu_max_align(), but... for now, they just hard-code 32.
      const size_t height = RoundUp(rows(plane), kFrameAddressAlignment);
      strides_[plane] = RoundUp(row_bytes(plane), kFrameAddressAlignment);
      offset[plane] = data_size;
      data_size += height * strides_[plane];
    }

    // The extra line of UV being allocated is because h264 chroma MC
    // overreads by one line in some cases, see libavcodec/utils.c:
    // avcodec_align_dimensions2() and libavcodec/x86/h264_chromamc.asm:
    // put_h264_chroma_mc4_ssse3().
    DCHECK(IsValidPlane(kUPlane, format_));
    data_size += strides_[kUPlane] + kFrameSizePadding;
  }

  uint8_t* data = reinterpret_cast<uint8_t*>(
      base::AlignedAlloc(data_size, kFrameAddressAlignment));
  if (zero_initialize_memory)
    memset(data, 0, data_size);

  for (size_t plane = 0; plane < NumPlanes(format_); ++plane)
    data_[plane] = data + offset[plane];

  AddDestructionObserver(base::Bind(&base::AlignedFree, data));
}

}  // namespace media
