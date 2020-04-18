// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/v4l2/v4l2_video_decode_accelerator.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/numerics/safe_conversions.h"
#include "base/posix/eintr_wrapper.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "media/base/media_switches.h"
#include "media/gpu/shared_memory_region.h"
#include "media/video/h264_parser.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/scoped_binders.h"

#define DVLOGF(level) DVLOG(level) << __func__ << "(): "
#define VLOGF(level) VLOG(level) << __func__ << "(): "
#define VPLOGF(level) VPLOG(level) << __func__ << "(): "

#define NOTIFY_ERROR(x)                      \
  do {                                       \
    VLOGF(1) << "Setting error state:" << x; \
    SetErrorState(x);                        \
  } while (0)

#define IOCTL_OR_ERROR_RETURN_VALUE(type, arg, value, type_str) \
  do {                                                          \
    if (device_->Ioctl(type, arg) != 0) {                       \
      VPLOGF(1) << "ioctl() failed: " << type_str;              \
      NOTIFY_ERROR(PLATFORM_FAILURE);                           \
      return value;                                             \
    }                                                           \
  } while (0)

#define IOCTL_OR_ERROR_RETURN(type, arg) \
  IOCTL_OR_ERROR_RETURN_VALUE(type, arg, ((void)0), #type)

#define IOCTL_OR_ERROR_RETURN_FALSE(type, arg) \
  IOCTL_OR_ERROR_RETURN_VALUE(type, arg, false, #type)

#define IOCTL_OR_LOG_ERROR(type, arg)           \
  do {                                          \
    if (device_->Ioctl(type, arg) != 0)         \
      VPLOGF(1) << "ioctl() failed: " << #type; \
  } while (0)

namespace media {

// static
const uint32_t V4L2VideoDecodeAccelerator::supported_input_fourccs_[] = {
    V4L2_PIX_FMT_H264, V4L2_PIX_FMT_VP8, V4L2_PIX_FMT_VP9,
};

struct V4L2VideoDecodeAccelerator::BitstreamBufferRef {
  BitstreamBufferRef(
      base::WeakPtr<Client>& client,
      scoped_refptr<base::SingleThreadTaskRunner>& client_task_runner,
      std::unique_ptr<SharedMemoryRegion> shm,
      int32_t input_id);
  ~BitstreamBufferRef();
  const base::WeakPtr<Client> client;
  const scoped_refptr<base::SingleThreadTaskRunner> client_task_runner;
  const std::unique_ptr<SharedMemoryRegion> shm;
  size_t bytes_used;
  const int32_t input_id;
};

struct V4L2VideoDecodeAccelerator::EGLSyncKHRRef {
  EGLSyncKHRRef(EGLDisplay egl_display, EGLSyncKHR egl_sync);
  ~EGLSyncKHRRef();
  EGLDisplay const egl_display;
  EGLSyncKHR egl_sync;
};

V4L2VideoDecodeAccelerator::BitstreamBufferRef::BitstreamBufferRef(
    base::WeakPtr<Client>& client,
    scoped_refptr<base::SingleThreadTaskRunner>& client_task_runner,
    std::unique_ptr<SharedMemoryRegion> shm,
    int32_t input_id)
    : client(client),
      client_task_runner(client_task_runner),
      shm(std::move(shm)),
      bytes_used(0),
      input_id(input_id) {}

V4L2VideoDecodeAccelerator::BitstreamBufferRef::~BitstreamBufferRef() {
  if (input_id >= 0) {
    client_task_runner->PostTask(
        FROM_HERE,
        base::Bind(&Client::NotifyEndOfBitstreamBuffer, client, input_id));
  }
}

V4L2VideoDecodeAccelerator::EGLSyncKHRRef::EGLSyncKHRRef(EGLDisplay egl_display,
                                                         EGLSyncKHR egl_sync)
    : egl_display(egl_display), egl_sync(egl_sync) {}

V4L2VideoDecodeAccelerator::EGLSyncKHRRef::~EGLSyncKHRRef() {
  // We don't check for eglDestroySyncKHR failures, because if we get here
  // with a valid sync object, something went wrong and we are getting
  // destroyed anyway.
  if (egl_sync != EGL_NO_SYNC_KHR)
    eglDestroySyncKHR(egl_display, egl_sync);
}

V4L2VideoDecodeAccelerator::InputRecord::InputRecord()
    : at_device(false), address(NULL), length(0), bytes_used(0), input_id(-1) {}

V4L2VideoDecodeAccelerator::InputRecord::~InputRecord() {}

V4L2VideoDecodeAccelerator::OutputRecord::OutputRecord()
    : state(kFree),
      egl_image(EGL_NO_IMAGE_KHR),
      egl_sync(EGL_NO_SYNC_KHR),
      picture_id(-1),
      texture_id(0),
      cleared(false) {}

V4L2VideoDecodeAccelerator::OutputRecord::OutputRecord(OutputRecord&&) =
    default;

V4L2VideoDecodeAccelerator::OutputRecord::~OutputRecord() {}

V4L2VideoDecodeAccelerator::PictureRecord::PictureRecord(bool cleared,
                                                         const Picture& picture)
    : cleared(cleared), picture(picture) {}

V4L2VideoDecodeAccelerator::PictureRecord::~PictureRecord() {}

V4L2VideoDecodeAccelerator::V4L2VideoDecodeAccelerator(
    EGLDisplay egl_display,
    const GetGLContextCallback& get_gl_context_cb,
    const MakeGLContextCurrentCallback& make_context_current_cb,
    const scoped_refptr<V4L2Device>& device)
    : child_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      decoder_thread_("V4L2DecoderThread"),
      decoder_state_(kUninitialized),
      output_mode_(Config::OutputMode::ALLOCATE),
      device_(device),
      decoder_delay_bitstream_buffer_id_(-1),
      decoder_current_input_buffer_(-1),
      decoder_decode_buffer_tasks_scheduled_(0),
      decoder_frames_at_client_(0),
      decoder_flushing_(false),
      decoder_cmd_supported_(false),
      flush_awaiting_last_output_buffer_(false),
      reset_pending_(false),
      decoder_partial_frame_pending_(false),
      input_streamon_(false),
      input_buffer_queued_count_(0),
      output_streamon_(false),
      output_buffer_queued_count_(0),
      output_dpb_size_(0),
      output_planes_count_(0),
      picture_clearing_count_(0),
      device_poll_thread_("V4L2DevicePollThread"),
      egl_display_(egl_display),
      get_gl_context_cb_(get_gl_context_cb),
      make_context_current_cb_(make_context_current_cb),
      video_profile_(VIDEO_CODEC_PROFILE_UNKNOWN),
      input_format_fourcc_(0),
      output_format_fourcc_(0),
      egl_image_format_fourcc_(0),
      egl_image_planes_count_(0),
      weak_this_factory_(this) {
  weak_this_ = weak_this_factory_.GetWeakPtr();
}

V4L2VideoDecodeAccelerator::~V4L2VideoDecodeAccelerator() {
  DCHECK(!decoder_thread_.IsRunning());
  DCHECK(!device_poll_thread_.IsRunning());
  DVLOGF(2);

  // These maps have members that should be manually destroyed, e.g. file
  // descriptors, mmap() segments, etc.
  DCHECK(input_buffer_map_.empty());
  DCHECK(output_buffer_map_.empty());
}

bool V4L2VideoDecodeAccelerator::Initialize(const Config& config,
                                            Client* client) {
  VLOGF(2) << "profile: " << config.profile
           << ", output_mode=" << static_cast<int>(config.output_mode);
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(decoder_state_, kUninitialized);

  if (config.is_encrypted()) {
    NOTREACHED() << "Encrypted streams are not supported for this VDA";
    return false;
  }

  if (config.output_mode != Config::OutputMode::ALLOCATE &&
      config.output_mode != Config::OutputMode::IMPORT) {
    NOTREACHED() << "Only ALLOCATE and IMPORT OutputModes are supported";
    return false;
  }

  client_ptr_factory_.reset(new base::WeakPtrFactory<Client>(client));
  client_ = client_ptr_factory_->GetWeakPtr();
  // If we haven't been set up to decode on separate thread via
  // TryToSetupDecodeOnSeparateThread(), use the main thread/client for
  // decode tasks.
  if (!decode_task_runner_) {
    decode_task_runner_ = child_task_runner_;
    DCHECK(!decode_client_);
    decode_client_ = client_;
  }

  video_profile_ = config.profile;

  // We need the context to be initialized to query extensions.
  if (!make_context_current_cb_.is_null()) {
    if (egl_display_ == EGL_NO_DISPLAY) {
      VLOGF(1) << "could not get EGLDisplay";
      return false;
    }

    if (!make_context_current_cb_.Run()) {
      VLOGF(1) << "could not make context current";
      return false;
    }

// TODO(posciak): https://crbug.com/450898.
#if defined(ARCH_CPU_ARMEL)
    if (!gl::g_driver_egl.ext.b_EGL_KHR_fence_sync) {
      VLOGF(1) << "context does not have EGL_KHR_fence_sync";
      return false;
    }
#endif
  } else {
    DVLOGF(2) << "No GL callbacks provided, initializing without GL support";
  }

  input_format_fourcc_ =
      V4L2Device::VideoCodecProfileToV4L2PixFmt(video_profile_, false);

  if (!device_->Open(V4L2Device::Type::kDecoder, input_format_fourcc_)) {
    VLOGF(1) << "Failed to open device for profile: " << config.profile
             << " fourcc: " << std::hex << "0x" << input_format_fourcc_;
    return false;
  }

  // Capabilities check.
  struct v4l2_capability caps;
  const __u32 kCapsRequired = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
  IOCTL_OR_ERROR_RETURN_FALSE(VIDIOC_QUERYCAP, &caps);
  if ((caps.capabilities & kCapsRequired) != kCapsRequired) {
    VLOGF(1) << "ioctl() failed: VIDIOC_QUERYCAP"
             << ", caps check failed: 0x" << std::hex << caps.capabilities;
    return false;
  }

  output_mode_ = config.output_mode;
  if (!SetupFormats())
    return false;

  if (video_profile_ >= H264PROFILE_MIN && video_profile_ <= H264PROFILE_MAX) {
    decoder_h264_parser_.reset(new H264Parser());
  }

  if (!decoder_thread_.Start()) {
    VLOGF(1) << "decoder thread failed to start";
    return false;
  }

  decoder_state_ = kInitialized;

  // InitializeTask will NOTIFY_ERROR on failure.
  decoder_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::InitializeTask,
                            base::Unretained(this)));

  return true;
}

void V4L2VideoDecodeAccelerator::InitializeTask() {
  VLOGF(2);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_EQ(decoder_state_, kInitialized);

  // Subscribe to the resolution change event.
  struct v4l2_event_subscription sub;
  memset(&sub, 0, sizeof(sub));
  sub.type = V4L2_EVENT_SOURCE_CHANGE;
  IOCTL_OR_ERROR_RETURN(VIDIOC_SUBSCRIBE_EVENT, &sub);

  if (!CreateInputBuffers()) {
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return;
  }

  decoder_cmd_supported_ = IsDecoderCmdSupported();

  if (!StartDevicePoll())
    return;
}

void V4L2VideoDecodeAccelerator::Decode(
    const BitstreamBuffer& bitstream_buffer) {
  DVLOGF(4) << "input_id=" << bitstream_buffer.id()
            << ", size=" << bitstream_buffer.size();
  DCHECK(decode_task_runner_->BelongsToCurrentThread());

  if (bitstream_buffer.id() < 0) {
    VLOGF(1) << "Invalid bitstream_buffer, id: " << bitstream_buffer.id();
    if (base::SharedMemory::IsHandleValid(bitstream_buffer.handle()))
      base::SharedMemory::CloseHandle(bitstream_buffer.handle());
    NOTIFY_ERROR(INVALID_ARGUMENT);
    return;
  }

  // DecodeTask() will take care of running a DecodeBufferTask().
  decoder_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::DecodeTask,
                            base::Unretained(this), bitstream_buffer));
}

void V4L2VideoDecodeAccelerator::AssignPictureBuffers(
    const std::vector<PictureBuffer>& buffers) {
  VLOGF(2) << "buffer_count=" << buffers.size();
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  decoder_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&V4L2VideoDecodeAccelerator::AssignPictureBuffersTask,
                 base::Unretained(this), buffers));
}

void V4L2VideoDecodeAccelerator::AssignPictureBuffersTask(
    const std::vector<PictureBuffer>& buffers) {
  VLOGF(2);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_EQ(decoder_state_, kAwaitingPictureBuffers);

  uint32_t req_buffer_count = output_dpb_size_ + kDpbOutputBufferExtraCount;
  if (image_processor_device_)
    req_buffer_count += kDpbOutputBufferExtraCountForImageProcessor;

  if (buffers.size() < req_buffer_count) {
    VLOGF(1) << "Failed to provide requested picture buffers. (Got "
             << buffers.size() << ", requested " << req_buffer_count << ")";
    NOTIFY_ERROR(INVALID_ARGUMENT);
    return;
  }

  // Allocate the output buffers.
  struct v4l2_requestbuffers reqbufs;
  memset(&reqbufs, 0, sizeof(reqbufs));
  reqbufs.count = buffers.size();
  reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  if (!image_processor_device_ && output_mode_ == Config::OutputMode::IMPORT) {
    reqbufs.memory = V4L2_MEMORY_DMABUF;
  } else {
    reqbufs.memory = V4L2_MEMORY_MMAP;
  }
  IOCTL_OR_ERROR_RETURN(VIDIOC_REQBUFS, &reqbufs);

  if (reqbufs.count != buffers.size()) {
    VLOGF(1) << "Could not allocate enough output buffers";
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return;
  }

  DCHECK(free_output_buffers_.empty());
  DCHECK(output_buffer_map_.empty());
  output_buffer_map_.resize(buffers.size());
  if (image_processor_device_ && output_mode_ == Config::OutputMode::ALLOCATE) {
    if (!CreateImageProcessor())
      return;
  }

  for (size_t i = 0; i < output_buffer_map_.size(); ++i) {
    DCHECK(buffers[i].size() == egl_image_size_);

    OutputRecord& output_record = output_buffer_map_[i];
    DCHECK_EQ(output_record.state, kFree);
    DCHECK_EQ(output_record.egl_image, EGL_NO_IMAGE_KHR);
    DCHECK_EQ(output_record.egl_sync, EGL_NO_SYNC_KHR);
    DCHECK_EQ(output_record.picture_id, -1);
    DCHECK_EQ(output_record.cleared, false);
    DCHECK(output_record.processor_input_fds.empty());

    output_record.picture_id = buffers[i].id();
    output_record.texture_id = buffers[i].service_texture_ids().empty()
                                   ? 0
                                   : buffers[i].service_texture_ids()[0];

    // This will remain kAtClient until ImportBufferForPicture is called, either
    // by the client, or by ourselves, if we are allocating.
    output_record.state = kAtClient;

    if (image_processor_device_) {
      std::vector<base::ScopedFD> dmabuf_fds = device_->GetDmabufsForV4L2Buffer(
          i, output_planes_count_, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
      if (dmabuf_fds.empty()) {
        VLOGF(1) << "Failed to get DMABUFs of decoder.";
        NOTIFY_ERROR(PLATFORM_FAILURE);
        return;
      }
      output_record.processor_input_fds = std::move(dmabuf_fds);
    }

    if (output_mode_ == Config::OutputMode::ALLOCATE) {
      std::vector<base::ScopedFD> dmabuf_fds;
      dmabuf_fds = egl_image_device_->GetDmabufsForV4L2Buffer(
          i, egl_image_planes_count_, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
      if (dmabuf_fds.empty()) {
        VLOGF(1) << "Failed to get DMABUFs for EGLImage.";
        NOTIFY_ERROR(PLATFORM_FAILURE);
        return;
      }
      int plane_horiz_bits_per_pixel = VideoFrame::PlaneHorizontalBitsPerPixel(
          V4L2Device::V4L2PixFmtToVideoPixelFormat(egl_image_format_fourcc_),
          0);
      ImportBufferForPictureTask(
          output_record.picture_id, std::move(dmabuf_fds),
          egl_image_size_.width() * plane_horiz_bits_per_pixel / 8);
    }  // else we'll get triggered via ImportBufferForPicture() from client.

    DVLOGF(3) << "buffer[" << i << "]: picture_id=" << output_record.picture_id;
  }

  if (output_mode_ == Config::OutputMode::ALLOCATE) {
    ScheduleDecodeBufferTaskIfNeeded();
  }
}

void V4L2VideoDecodeAccelerator::CreateEGLImageFor(
    size_t buffer_index,
    int32_t picture_buffer_id,
    std::vector<base::ScopedFD> dmabuf_fds,
    GLuint texture_id,
    const gfx::Size& size,
    uint32_t fourcc) {
  DVLOGF(3) << "index=" << buffer_index;
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  DCHECK_NE(texture_id, 0u);

  if (get_gl_context_cb_.is_null() || make_context_current_cb_.is_null()) {
    VLOGF(1) << "GL callbacks required for binding to EGLImages";
    NOTIFY_ERROR(INVALID_ARGUMENT);
    return;
  }

  gl::GLContext* gl_context = get_gl_context_cb_.Run();
  if (!gl_context || !make_context_current_cb_.Run()) {
    VLOGF(1) << "No GL context";
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return;
  }

  gl::ScopedTextureBinder bind_restore(GL_TEXTURE_EXTERNAL_OES, 0);

  EGLImageKHR egl_image = egl_image_device_->CreateEGLImage(
      egl_display_, gl_context->GetHandle(), texture_id, size, buffer_index,
      fourcc, dmabuf_fds);
  if (egl_image == EGL_NO_IMAGE_KHR) {
    VLOGF(1) << "could not create EGLImageKHR,"
             << " index=" << buffer_index << " texture_id=" << texture_id;
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return;
  }

  decoder_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&V4L2VideoDecodeAccelerator::AssignEGLImage,
                 base::Unretained(this), buffer_index, picture_buffer_id,
                 egl_image, base::Passed(&dmabuf_fds)));
}

void V4L2VideoDecodeAccelerator::AssignEGLImage(
    size_t buffer_index,
    int32_t picture_buffer_id,
    EGLImageKHR egl_image,
    std::vector<base::ScopedFD> dmabuf_fds) {
  DVLOGF(3) << "index=" << buffer_index << ", picture_id=" << picture_buffer_id;
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());

  // It's possible that while waiting for the EGLImages to be allocated and
  // assigned, we have already decoded more of the stream and saw another
  // resolution change. This is a normal situation, in such a case either there
  // is no output record with this index awaiting an EGLImage to be assigned to
  // it, or the record is already updated to use a newer PictureBuffer and is
  // awaiting an EGLImage associated with a different picture_buffer_id. If so,
  // just discard this image, we will get the one we are waiting for later.
  if (buffer_index >= output_buffer_map_.size() ||
      output_buffer_map_[buffer_index].picture_id != picture_buffer_id) {
    DVLOGF(4) << "Picture set already changed, dropping EGLImage";
    child_task_runner_->PostTask(
        FROM_HERE, base::Bind(base::IgnoreResult(&V4L2Device::DestroyEGLImage),
                              device_, egl_display_, egl_image));
    return;
  }

  OutputRecord& output_record = output_buffer_map_[buffer_index];
  DCHECK_EQ(output_record.egl_image, EGL_NO_IMAGE_KHR);
  DCHECK_EQ(output_record.egl_sync, EGL_NO_SYNC_KHR);
  DCHECK_EQ(output_record.state, kFree);
  DCHECK_EQ(std::count(free_output_buffers_.begin(), free_output_buffers_.end(),
                       buffer_index),
            0);
  output_record.egl_image = egl_image;
  if (output_mode_ == Config::OutputMode::IMPORT) {
    DCHECK(output_record.output_fds.empty());
    output_record.output_fds.swap(dmabuf_fds);
  }
  free_output_buffers_.push_back(buffer_index);
  if (decoder_state_ != kChangingResolution) {
    Enqueue();
    ScheduleDecodeBufferTaskIfNeeded();
  }
}

void V4L2VideoDecodeAccelerator::ImportBufferForPicture(
    int32_t picture_buffer_id,
    VideoPixelFormat pixel_format,
    const gfx::GpuMemoryBufferHandle& gpu_memory_buffer_handle) {
  DVLOGF(3) << "picture_buffer_id=" << picture_buffer_id;
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  if (output_mode_ != Config::OutputMode::IMPORT) {
    VLOGF(1) << "Cannot import in non-import mode";
    NOTIFY_ERROR(INVALID_ARGUMENT);
    return;
  }

  // |output_format_fourcc_| is the output format of the decoder. It is not
  // the final output format from the image processor (if exists).
  // Use |egl_image_format_fourcc_|, it will be the final output format.
  if (pixel_format !=
      V4L2Device::V4L2PixFmtToVideoPixelFormat(egl_image_format_fourcc_)) {
    VLOGF(1) << "Unsupported import format: " << pixel_format;
    NOTIFY_ERROR(INVALID_ARGUMENT);
    return;
  }

  std::vector<base::ScopedFD> dmabuf_fds;
  int32_t stride = 0;
#if defined(USE_OZONE)
  for (const auto& fd : gpu_memory_buffer_handle.native_pixmap_handle.fds) {
    DCHECK_NE(fd.fd, -1);
    dmabuf_fds.push_back(base::ScopedFD(fd.fd));
  }
  stride = gpu_memory_buffer_handle.native_pixmap_handle.planes[0].stride;
  for (const auto& plane :
       gpu_memory_buffer_handle.native_pixmap_handle.planes) {
    DVLOGF(3) << ": offset=" << plane.offset << ", stride=" << plane.stride;
  }
#endif

  decoder_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&V4L2VideoDecodeAccelerator::ImportBufferForPictureTask,
                 base::Unretained(this), picture_buffer_id,
                 base::Passed(&dmabuf_fds), stride));
}

void V4L2VideoDecodeAccelerator::ImportBufferForPictureTask(
    int32_t picture_buffer_id,
    std::vector<base::ScopedFD> dmabuf_fds,
    int32_t stride) {
  DVLOGF(3) << "picture_buffer_id=" << picture_buffer_id
            << ", dmabuf_fds.size()=" << dmabuf_fds.size()
            << ", stride=" << stride;
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());

  const auto iter =
      std::find_if(output_buffer_map_.begin(), output_buffer_map_.end(),
                   [picture_buffer_id](const OutputRecord& output_record) {
                     return output_record.picture_id == picture_buffer_id;
                   });
  if (iter == output_buffer_map_.end()) {
    // It's possible that we've already posted a DismissPictureBuffer for this
    // picture, but it has not yet executed when this ImportBufferForPicture was
    // posted to us by the client. In that case just ignore this (we've already
    // dismissed it and accounted for that).
    DVLOGF(3) << "got picture id=" << picture_buffer_id
              << " not in use (anymore?).";
    return;
  }

  if (iter->state != kAtClient) {
    VLOGF(1) << "Cannot import buffer not owned by client";
    NOTIFY_ERROR(INVALID_ARGUMENT);
    return;
  }

  int plane_horiz_bits_per_pixel = VideoFrame::PlaneHorizontalBitsPerPixel(
      V4L2Device::V4L2PixFmtToVideoPixelFormat(egl_image_format_fourcc_), 0);
  if (plane_horiz_bits_per_pixel == 0 ||
      (stride * 8) % plane_horiz_bits_per_pixel != 0) {
    VLOGF(1) << "Invalid format " << egl_image_format_fourcc_ << " or stride "
             << stride;
    NOTIFY_ERROR(INVALID_ARGUMENT);
    return;
  }
  if (reset_pending_) {
    FinishReset();
  }

  int adjusted_coded_width = stride * 8 / plane_horiz_bits_per_pixel;
  if (image_processor_device_ && !image_processor_) {
    // This is the first buffer import. Create the image processor and change
    // the decoder state. The client may adjust the coded width. We don't have
    // the final coded size in AssignPictureBuffers yet. Use the adjusted coded
    // width to create the image processor.
    VLOGF(2) << "Original egl_image_size=" << egl_image_size_.ToString()
             << ", adjusted coded width=" << adjusted_coded_width;
    DCHECK_GE(adjusted_coded_width, egl_image_size_.width());
    egl_image_size_.set_width(adjusted_coded_width);
    if (!CreateImageProcessor())
      return;
    DCHECK_EQ(kAwaitingPictureBuffers, decoder_state_);
  }
  DCHECK_EQ(egl_image_size_.width(), adjusted_coded_width);

  if (decoder_state_ == kAwaitingPictureBuffers) {
    decoder_state_ = kDecoding;
    DVLOGF(3) << "Change state to kDecoding";
  }

  size_t index = iter - output_buffer_map_.begin();
  DCHECK_EQ(std::count(free_output_buffers_.begin(), free_output_buffers_.end(),
                       index),
            0);

  iter->state = kFree;
  if (iter->texture_id != 0) {
    if (iter->egl_image != EGL_NO_IMAGE_KHR) {
      child_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(base::IgnoreResult(&V4L2Device::DestroyEGLImage), device_,
                     egl_display_, iter->egl_image));
    }

    child_task_runner_->PostTask(
        FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::CreateEGLImageFor,
                              weak_this_, index, picture_buffer_id,
                              base::Passed(&dmabuf_fds), iter->texture_id,
                              egl_image_size_, egl_image_format_fourcc_));
  } else {
    // No need for an EGLImage, start using this buffer now.
    DCHECK_EQ(egl_image_planes_count_, dmabuf_fds.size());
    iter->output_fds.swap(dmabuf_fds);
    free_output_buffers_.push_back(index);
    if (decoder_state_ != kChangingResolution) {
      Enqueue();
      ScheduleDecodeBufferTaskIfNeeded();
    }
  }
}

void V4L2VideoDecodeAccelerator::ReusePictureBuffer(int32_t picture_buffer_id) {
  DVLOGF(4) << "picture_buffer_id=" << picture_buffer_id;
  // Must be run on child thread, as we'll insert a sync in the EGL context.
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  std::unique_ptr<EGLSyncKHRRef> egl_sync_ref;

  if (!make_context_current_cb_.is_null()) {
    if (!make_context_current_cb_.Run()) {
      VLOGF(1) << "could not make context current";
      NOTIFY_ERROR(PLATFORM_FAILURE);
      return;
    }

    EGLSyncKHR egl_sync = EGL_NO_SYNC_KHR;
// TODO(posciak): https://crbug.com/450898.
#if defined(ARCH_CPU_ARMEL)
    egl_sync = eglCreateSyncKHR(egl_display_, EGL_SYNC_FENCE_KHR, NULL);
    if (egl_sync == EGL_NO_SYNC_KHR) {
      VLOGF(1) << "eglCreateSyncKHR() failed";
      NOTIFY_ERROR(PLATFORM_FAILURE);
      return;
    }
#endif

    egl_sync_ref.reset(new EGLSyncKHRRef(egl_display_, egl_sync));
  }

  decoder_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::ReusePictureBufferTask,
                            base::Unretained(this), picture_buffer_id,
                            base::Passed(&egl_sync_ref)));
}

void V4L2VideoDecodeAccelerator::Flush() {
  VLOGF(2);
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  decoder_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::FlushTask,
                            base::Unretained(this)));
}

void V4L2VideoDecodeAccelerator::Reset() {
  VLOGF(2);
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  decoder_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::ResetTask,
                            base::Unretained(this)));
}

void V4L2VideoDecodeAccelerator::Destroy() {
  VLOGF(2);
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  // We're destroying; cancel all callbacks.
  client_ptr_factory_.reset();
  weak_this_factory_.InvalidateWeakPtrs();

  // If the decoder thread is running, destroy using posted task.
  if (decoder_thread_.IsRunning()) {
    decoder_thread_.task_runner()->PostTask(
        FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::DestroyTask,
                              base::Unretained(this)));
    // DestroyTask() will cause the decoder_thread_ to flush all tasks.
    decoder_thread_.Stop();
  } else {
    // Otherwise, call the destroy task directly.
    DestroyTask();
  }

  delete this;
  VLOGF(2) << "Destroyed.";
}

bool V4L2VideoDecodeAccelerator::TryToSetupDecodeOnSeparateThread(
    const base::WeakPtr<Client>& decode_client,
    const scoped_refptr<base::SingleThreadTaskRunner>& decode_task_runner) {
  VLOGF(2);
  decode_client_ = decode_client;
  decode_task_runner_ = decode_task_runner;
  return true;
}

// static
VideoDecodeAccelerator::SupportedProfiles
V4L2VideoDecodeAccelerator::GetSupportedProfiles() {
  scoped_refptr<V4L2Device> device = V4L2Device::Create();
  if (!device)
    return SupportedProfiles();

  return device->GetSupportedDecodeProfiles(arraysize(supported_input_fourccs_),
                                            supported_input_fourccs_);
}

void V4L2VideoDecodeAccelerator::DecodeTask(
    const BitstreamBuffer& bitstream_buffer) {
  DVLOGF(4) << "input_id=" << bitstream_buffer.id();
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_NE(decoder_state_, kUninitialized);
  TRACE_EVENT1("media,gpu", "V4L2VDA::DecodeTask", "input_id",
               bitstream_buffer.id());

  std::unique_ptr<BitstreamBufferRef> bitstream_record(new BitstreamBufferRef(
      decode_client_, decode_task_runner_,
      std::unique_ptr<SharedMemoryRegion>(
          new SharedMemoryRegion(bitstream_buffer, true)),
      bitstream_buffer.id()));

  // Skip empty buffer.
  if (bitstream_buffer.size() == 0)
    return;

  if (!bitstream_record->shm->Map()) {
    VLOGF(1) << "could not map bitstream_buffer";
    NOTIFY_ERROR(UNREADABLE_INPUT);
    return;
  }
  DVLOGF(4) << "mapped at=" << bitstream_record->shm->memory();

  if (decoder_state_ == kResetting || decoder_flushing_) {
    // In the case that we're resetting or flushing, we need to delay decoding
    // the BitstreamBuffers that come after the Reset() or Flush() call.  When
    // we're here, we know that this DecodeTask() was scheduled by a Decode()
    // call that came after (in the client thread) the Reset() or Flush() call;
    // thus set up the delay if necessary.
    if (decoder_delay_bitstream_buffer_id_ == -1)
      decoder_delay_bitstream_buffer_id_ = bitstream_record->input_id;
  } else if (decoder_state_ == kError) {
    VLOGF(2) << "early out: kError state";
    return;
  }

  decoder_input_queue_.push(std::move(bitstream_record));
  decoder_decode_buffer_tasks_scheduled_++;
  DecodeBufferTask();
}

void V4L2VideoDecodeAccelerator::DecodeBufferTask() {
  DVLOGF(4);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_NE(decoder_state_, kUninitialized);
  TRACE_EVENT0("media,gpu", "V4L2VDA::DecodeBufferTask");

  decoder_decode_buffer_tasks_scheduled_--;

  if (decoder_state_ != kInitialized && decoder_state_ != kDecoding) {
    DVLOGF(3) << "early out: state=" << decoder_state_;
    return;
  }

  if (decoder_current_bitstream_buffer_ == NULL) {
    if (decoder_input_queue_.empty()) {
      // We're waiting for a new buffer -- exit without scheduling a new task.
      return;
    }
    if (decoder_delay_bitstream_buffer_id_ ==
        decoder_input_queue_.front()->input_id) {
      // We're asked to delay decoding on this and subsequent buffers.
      return;
    }

    // Setup to use the next buffer.
    decoder_current_bitstream_buffer_ = std::move(decoder_input_queue_.front());
    decoder_input_queue_.pop();
    const auto& shm = decoder_current_bitstream_buffer_->shm;
    if (shm) {
      DVLOGF(4) << "reading input_id="
                << decoder_current_bitstream_buffer_->input_id
                << ", addr=" << shm->memory() << ", size=" << shm->size();
    } else {
      DCHECK_EQ(decoder_current_bitstream_buffer_->input_id, kFlushBufferId);
      DVLOGF(4) << "reading input_id=kFlushBufferId";
    }
  }
  bool schedule_task = false;
  size_t decoded_size = 0;
  const auto& shm = decoder_current_bitstream_buffer_->shm;
  if (!shm) {
    // This is a dummy buffer, queued to flush the pipe.  Flush.
    DCHECK_EQ(decoder_current_bitstream_buffer_->input_id, kFlushBufferId);
    // Enqueue a buffer guaranteed to be empty.  To do that, we flush the
    // current input, enqueue no data to the next frame, then flush that down.
    schedule_task = true;
    if (decoder_current_input_buffer_ != -1 &&
        input_buffer_map_[decoder_current_input_buffer_].input_id !=
            kFlushBufferId)
      schedule_task = FlushInputFrame();

    if (schedule_task && AppendToInputFrame(NULL, 0) && FlushInputFrame()) {
      VLOGF(2) << "enqueued flush buffer";
      decoder_partial_frame_pending_ = false;
      schedule_task = true;
    } else {
      // If we failed to enqueue the empty buffer (due to pipeline
      // backpressure), don't advance the bitstream buffer queue, and don't
      // schedule the next task.  This bitstream buffer queue entry will get
      // reprocessed when the pipeline frees up.
      schedule_task = false;
    }
  } else if (shm->size() == 0) {
    // This is a buffer queued from the client that has zero size.  Skip.
    schedule_task = true;
  } else {
    // This is a buffer queued from the client, with actual contents.  Decode.
    const uint8_t* const data =
        reinterpret_cast<const uint8_t*>(shm->memory()) +
        decoder_current_bitstream_buffer_->bytes_used;
    const size_t data_size =
        shm->size() - decoder_current_bitstream_buffer_->bytes_used;
    if (!AdvanceFrameFragment(data, data_size, &decoded_size)) {
      NOTIFY_ERROR(UNREADABLE_INPUT);
      return;
    }
    // AdvanceFrameFragment should not return a size larger than the buffer
    // size, even on invalid data.
    CHECK_LE(decoded_size, data_size);

    switch (decoder_state_) {
      case kInitialized:
        schedule_task = DecodeBufferInitial(data, decoded_size, &decoded_size);
        break;
      case kDecoding:
        schedule_task = DecodeBufferContinue(data, decoded_size);
        break;
      default:
        NOTIFY_ERROR(ILLEGAL_STATE);
        return;
    }
  }
  if (decoder_state_ == kError) {
    // Failed during decode.
    return;
  }

  if (schedule_task) {
    decoder_current_bitstream_buffer_->bytes_used += decoded_size;
    if ((shm ? shm->size() : 0) ==
        decoder_current_bitstream_buffer_->bytes_used) {
      // Our current bitstream buffer is done; return it.
      int32_t input_id = decoder_current_bitstream_buffer_->input_id;
      DVLOGF(4) << "finished input_id=" << input_id;
      // BitstreamBufferRef destructor calls NotifyEndOfBitstreamBuffer().
      decoder_current_bitstream_buffer_.reset();
    }
    ScheduleDecodeBufferTaskIfNeeded();
  }
}

bool V4L2VideoDecodeAccelerator::AdvanceFrameFragment(const uint8_t* data,
                                                      size_t size,
                                                      size_t* endpos) {
  if (video_profile_ >= H264PROFILE_MIN && video_profile_ <= H264PROFILE_MAX) {
    // For H264, we need to feed HW one frame at a time.  This is going to take
    // some parsing of our input stream.
    decoder_h264_parser_->SetStream(data, size);
    H264NALU nalu;
    H264Parser::Result result;
    *endpos = 0;

    // Keep on peeking the next NALs while they don't indicate a frame
    // boundary.
    for (;;) {
      bool end_of_frame = false;
      result = decoder_h264_parser_->AdvanceToNextNALU(&nalu);
      if (result == H264Parser::kInvalidStream ||
          result == H264Parser::kUnsupportedStream)
        return false;
      if (result == H264Parser::kEOStream) {
        // We've reached the end of the buffer before finding a frame boundary.
        decoder_partial_frame_pending_ = true;
        *endpos = size;
        return true;
      }
      switch (nalu.nal_unit_type) {
        case H264NALU::kNonIDRSlice:
        case H264NALU::kIDRSlice:
          if (nalu.size < 1)
            return false;
          // For these two, if the "first_mb_in_slice" field is zero, start a
          // new frame and return.  This field is Exp-Golomb coded starting on
          // the eighth data bit of the NAL; a zero value is encoded with a
          // leading '1' bit in the byte, which we can detect as the byte being
          // (unsigned) greater than or equal to 0x80.
          if (nalu.data[1] >= 0x80) {
            end_of_frame = true;
            break;
          }
          break;
        case H264NALU::kSEIMessage:
        case H264NALU::kSPS:
        case H264NALU::kPPS:
        case H264NALU::kAUD:
        case H264NALU::kEOSeq:
        case H264NALU::kEOStream:
        case H264NALU::kReserved14:
        case H264NALU::kReserved15:
        case H264NALU::kReserved16:
        case H264NALU::kReserved17:
        case H264NALU::kReserved18:
          // These unconditionally signal a frame boundary.
          end_of_frame = true;
          break;
        default:
          // For all others, keep going.
          break;
      }
      if (end_of_frame) {
        if (!decoder_partial_frame_pending_ && *endpos == 0) {
          // The frame was previously restarted, and we haven't filled the
          // current frame with any contents yet.  Start the new frame here and
          // continue parsing NALs.
        } else {
          // The frame wasn't previously restarted and/or we have contents for
          // the current frame; signal the start of a new frame here: we don't
          // have a partial frame anymore.
          decoder_partial_frame_pending_ = false;
          return true;
        }
      }
      *endpos = (nalu.data + nalu.size) - data;
    }
    NOTREACHED();
    return false;
  } else {
    DCHECK_GE(video_profile_, VP8PROFILE_MIN);
    DCHECK_LE(video_profile_, VP9PROFILE_MAX);
    // For VP8/9, we can just dump the entire buffer.  No fragmentation needed,
    // and we never return a partial frame.
    *endpos = size;
    decoder_partial_frame_pending_ = false;
    return true;
  }
}

void V4L2VideoDecodeAccelerator::ScheduleDecodeBufferTaskIfNeeded() {
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());

  // If we're behind on tasks, schedule another one.
  int buffers_to_decode = decoder_input_queue_.size();
  if (decoder_current_bitstream_buffer_ != NULL)
    buffers_to_decode++;
  if (decoder_decode_buffer_tasks_scheduled_ < buffers_to_decode) {
    decoder_decode_buffer_tasks_scheduled_++;
    decoder_thread_.task_runner()->PostTask(
        FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::DecodeBufferTask,
                              base::Unretained(this)));
  }
}

bool V4L2VideoDecodeAccelerator::DecodeBufferInitial(const void* data,
                                                     size_t size,
                                                     size_t* endpos) {
  DVLOGF(3) << "data=" << data << ", size=" << size;
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_EQ(decoder_state_, kInitialized);
  // Initial decode.  We haven't been able to get output stream format info yet.
  // Get it, and start decoding.

  // Copy in and send to HW.
  if (!AppendToInputFrame(data, size))
    return false;

  // If we only have a partial frame, don't flush and process yet.
  if (decoder_partial_frame_pending_)
    return true;

  if (!FlushInputFrame())
    return false;

  // Recycle buffers.
  Dequeue();

  *endpos = size;

  // If an initial resolution change event is not done yet, a driver probably
  // needs more stream to decode format.
  // Return true and schedule next buffer without changing status to kDecoding.
  // If the initial resolution change is done and coded size is known, we may
  // still have to wait for AssignPictureBuffers() and output buffers to be
  // allocated.
  if (coded_size_.IsEmpty() || output_buffer_map_.empty()) {
    return true;
  }

  decoder_state_ = kDecoding;
  ScheduleDecodeBufferTaskIfNeeded();
  return true;
}

bool V4L2VideoDecodeAccelerator::DecodeBufferContinue(const void* data,
                                                      size_t size) {
  DVLOGF(4) << "data=" << data << ", size=" << size;
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_EQ(decoder_state_, kDecoding);

  // Both of these calls will set kError state if they fail.
  // Only flush the frame if it's complete.
  return (AppendToInputFrame(data, size) &&
          (decoder_partial_frame_pending_ || FlushInputFrame()));
}

bool V4L2VideoDecodeAccelerator::AppendToInputFrame(const void* data,
                                                    size_t size) {
  DVLOGF(4);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_NE(decoder_state_, kUninitialized);
  DCHECK_NE(decoder_state_, kResetting);
  DCHECK_NE(decoder_state_, kError);
  // This routine can handle data == NULL and size == 0, which occurs when
  // we queue an empty buffer for the purposes of flushing the pipe.

  // Flush if we're too big
  if (decoder_current_input_buffer_ != -1) {
    InputRecord& input_record =
        input_buffer_map_[decoder_current_input_buffer_];
    if (input_record.bytes_used + size > input_record.length) {
      if (!FlushInputFrame())
        return false;
      decoder_current_input_buffer_ = -1;
    }
  }

  // Try to get an available input buffer
  if (decoder_current_input_buffer_ == -1) {
    if (free_input_buffers_.empty()) {
      // See if we can get more free buffers from HW
      Dequeue();
      if (free_input_buffers_.empty()) {
        // Nope!
        DVLOGF(4) << "stalled for input buffers";
        return false;
      }
    }
    decoder_current_input_buffer_ = free_input_buffers_.back();
    free_input_buffers_.pop_back();
    InputRecord& input_record =
        input_buffer_map_[decoder_current_input_buffer_];
    DCHECK_EQ(input_record.bytes_used, 0);
    DCHECK_EQ(input_record.input_id, -1);
    DCHECK(decoder_current_bitstream_buffer_ != NULL);
    input_record.input_id = decoder_current_bitstream_buffer_->input_id;
  }

  DCHECK(data != NULL || size == 0);
  if (size == 0) {
    // If we asked for an empty buffer, return now.  We return only after
    // getting the next input buffer, since we might actually want an empty
    // input buffer for flushing purposes.
    return true;
  }

  // Copy in to the buffer.
  InputRecord& input_record = input_buffer_map_[decoder_current_input_buffer_];
  if (size > input_record.length - input_record.bytes_used) {
    VLOGF(1) << "over-size frame, erroring";
    NOTIFY_ERROR(UNREADABLE_INPUT);
    return false;
  }
  memcpy(reinterpret_cast<uint8_t*>(input_record.address) +
             input_record.bytes_used,
         data, size);
  input_record.bytes_used += size;

  return true;
}

bool V4L2VideoDecodeAccelerator::FlushInputFrame() {
  DVLOGF(4);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_NE(decoder_state_, kUninitialized);
  DCHECK_NE(decoder_state_, kResetting);
  DCHECK_NE(decoder_state_, kError);

  if (decoder_current_input_buffer_ == -1)
    return true;

  InputRecord& input_record = input_buffer_map_[decoder_current_input_buffer_];
  DCHECK_NE(input_record.input_id, -1);
  DCHECK(input_record.input_id != kFlushBufferId ||
         input_record.bytes_used == 0);
  // * if input_id >= 0, this input buffer was prompted by a bitstream buffer we
  //   got from the client.  We can skip it if it is empty.
  // * if input_id < 0 (should be kFlushBufferId in this case), this input
  //   buffer was prompted by a flush buffer, and should be queued even when
  //   empty.
  if (input_record.input_id >= 0 && input_record.bytes_used == 0) {
    input_record.input_id = -1;
    free_input_buffers_.push_back(decoder_current_input_buffer_);
    decoder_current_input_buffer_ = -1;
    return true;
  }

  // Queue it.
  input_ready_queue_.push(decoder_current_input_buffer_);
  decoder_current_input_buffer_ = -1;
  DVLOGF(4) << "submitting input_id=" << input_record.input_id;
  // Enqueue once since there's new available input for it.
  Enqueue();

  return (decoder_state_ != kError);
}

void V4L2VideoDecodeAccelerator::ServiceDeviceTask(bool event_pending) {
  DVLOGF(4);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_NE(decoder_state_, kUninitialized);
  TRACE_EVENT0("media,gpu", "V4L2VDA::ServiceDeviceTask");

  if (decoder_state_ == kResetting) {
    DVLOGF(3) << "early out: kResetting state";
    return;
  } else if (decoder_state_ == kError) {
    DVLOGF(3) << "early out: kError state";
    return;
  } else if (decoder_state_ == kChangingResolution) {
    DVLOGF(3) << "early out: kChangingResolution state";
    return;
  }

  bool resolution_change_pending = false;
  if (event_pending)
    resolution_change_pending = DequeueResolutionChangeEvent();

  if (!resolution_change_pending && coded_size_.IsEmpty()) {
    // Some platforms do not send an initial resolution change event.
    // To work around this, we need to keep checking if the initial resolution
    // is known already by explicitly querying the format after each decode,
    // regardless of whether we received an event.
    // This needs to be done on initial resolution change,
    // i.e. when coded_size_.IsEmpty().

    // Try GetFormatInfo to check if an initial resolution change can be done.
    struct v4l2_format format;
    gfx::Size visible_size;
    bool again;
    if (GetFormatInfo(&format, &visible_size, &again) && !again) {
      resolution_change_pending = true;
      DequeueResolutionChangeEvent();
    }
  }

  Dequeue();
  Enqueue();

  // Clear the interrupt fd.
  if (!device_->ClearDevicePollInterrupt()) {
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return;
  }

  bool poll_device = false;
  // Add fd, if we should poll on it.
  // Can be polled as soon as either input or output buffers are queued.
  if (input_buffer_queued_count_ + output_buffer_queued_count_ > 0)
    poll_device = true;

  // ServiceDeviceTask() should only ever be scheduled from DevicePollTask(),
  // so either:
  // * device_poll_thread_ is running normally
  // * device_poll_thread_ scheduled us, but then a ResetTask() or DestroyTask()
  //   shut it down, in which case we're either in kResetting or kError states
  //   respectively, and we should have early-outed already.
  DCHECK(device_poll_thread_.message_loop());
  // Queue the DevicePollTask() now.
  device_poll_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::DevicePollTask,
                            base::Unretained(this), poll_device));

  DVLOGF(3) << "ServiceDeviceTask(): buffer counts: DEC["
            << decoder_input_queue_.size() << "->"
            << input_ready_queue_.size() << "] => DEVICE["
            << free_input_buffers_.size() << "+"
            << input_buffer_queued_count_ << "/"
            << input_buffer_map_.size() << "->"
            << free_output_buffers_.size() << "+"
            << output_buffer_queued_count_ << "/"
            << output_buffer_map_.size() << "] => PROCESSOR["
            << image_processor_bitstream_buffer_ids_.size() << "] => CLIENT["
            << decoder_frames_at_client_ << "]";

  ScheduleDecodeBufferTaskIfNeeded();
  if (resolution_change_pending)
    StartResolutionChange();
}

void V4L2VideoDecodeAccelerator::Enqueue() {
  DVLOGF(4);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_NE(decoder_state_, kUninitialized);
  TRACE_EVENT0("media,gpu", "V4L2VDA::Enqueue");

  // Drain the pipe of completed decode buffers.
  const int old_inputs_queued = input_buffer_queued_count_;
  while (!input_ready_queue_.empty()) {
    const int buffer = input_ready_queue_.front();
    InputRecord& input_record = input_buffer_map_[buffer];
    if (input_record.input_id == kFlushBufferId && decoder_cmd_supported_) {
      // Send the flush command after all input buffers are dequeued. This makes
      // sure all previous resolution changes have been handled because the
      // driver must hold the input buffer that triggers resolution change. The
      // driver cannot decode data in it without new output buffers. If we send
      // the flush now and a queued input buffer triggers resolution change
      // later, the driver will send an output buffer that has
      // V4L2_BUF_FLAG_LAST. But some queued input buffer have not been decoded
      // yet. Also, V4L2VDA calls STREAMOFF and STREAMON after resolution
      // change. They implicitly send a V4L2_DEC_CMD_STOP and V4L2_DEC_CMD_START
      // to the decoder.
      if (input_buffer_queued_count_ == 0) {
        input_ready_queue_.pop();
        free_input_buffers_.push_back(buffer);
        input_record.input_id = -1;
        if (coded_size_.IsEmpty() || !input_streamon_) {
          // (1) If coded_size_.IsEmpty(), no output buffer could have been
          // allocated and there is nothing to flush. We can NotifyFlushDone()
          // immediately, without requesting flush to the driver via
          // SendDecoderCmdStop().
          // (2) If input stream is off, we will never get the output buffer
          // with V4L2_BUF_FLAG_LAST. We should NotifyFlushDone().
          NotifyFlushDoneIfNeeded();
        } else if (!SendDecoderCmdStop()) {
          return;
        }
      } else {
        break;
      }
    } else if (!EnqueueInputRecord())
      return;
  }
  if (old_inputs_queued == 0 && input_buffer_queued_count_ != 0) {
    // We just started up a previously empty queue.
    // Queue state changed; signal interrupt.
    if (!device_->SetDevicePollInterrupt()) {
      VPLOGF(1) << "SetDevicePollInterrupt failed";
      NOTIFY_ERROR(PLATFORM_FAILURE);
      return;
    }
    // Start VIDIOC_STREAMON if we haven't yet.
    if (!input_streamon_) {
      __u32 type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
      IOCTL_OR_ERROR_RETURN(VIDIOC_STREAMON, &type);
      input_streamon_ = true;
    }
  }

  // Enqueue all the outputs we can.
  const int old_outputs_queued = output_buffer_queued_count_;
  while (!free_output_buffers_.empty()) {
    if (!EnqueueOutputRecord())
      return;
  }
  if (old_outputs_queued == 0 && output_buffer_queued_count_ != 0) {
    // We just started up a previously empty queue.
    // Queue state changed; signal interrupt.
    if (!device_->SetDevicePollInterrupt()) {
      VPLOGF(1) << "SetDevicePollInterrupt(): failed";
      NOTIFY_ERROR(PLATFORM_FAILURE);
      return;
    }
    // Start VIDIOC_STREAMON if we haven't yet.
    if (!output_streamon_) {
      __u32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
      IOCTL_OR_ERROR_RETURN(VIDIOC_STREAMON, &type);
      output_streamon_ = true;
    }
  }
}

bool V4L2VideoDecodeAccelerator::DequeueResolutionChangeEvent() {
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_NE(decoder_state_, kUninitialized);
  DVLOGF(3);

  struct v4l2_event ev;
  memset(&ev, 0, sizeof(ev));

  while (device_->Ioctl(VIDIOC_DQEVENT, &ev) == 0) {
    if (ev.type == V4L2_EVENT_SOURCE_CHANGE) {
      if (ev.u.src_change.changes & V4L2_EVENT_SRC_CH_RESOLUTION) {
        VLOGF(2) << "got resolution change event.";
        return true;
      }
    } else {
      VLOGF(1) << "got an event (" << ev.type << ") we haven't subscribed to.";
    }
  }
  return false;
}

void V4L2VideoDecodeAccelerator::Dequeue() {
  DVLOGF(4);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_NE(decoder_state_, kUninitialized);
  TRACE_EVENT0("media,gpu", "V4L2VDA::Dequeue");

  while (input_buffer_queued_count_ > 0) {
    if (!DequeueInputBuffer())
      break;
  }
  while (output_buffer_queued_count_ > 0) {
    if (!DequeueOutputBuffer())
      break;
  }
  NotifyFlushDoneIfNeeded();
}

bool V4L2VideoDecodeAccelerator::DequeueInputBuffer() {
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_GT(input_buffer_queued_count_, 0);
  DCHECK(input_streamon_);

  // Dequeue a completed input (VIDEO_OUTPUT) buffer, and recycle to the free
  // list.
  struct v4l2_buffer dqbuf;
  struct v4l2_plane planes[1];
  memset(&dqbuf, 0, sizeof(dqbuf));
  memset(planes, 0, sizeof(planes));
  dqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  dqbuf.memory = V4L2_MEMORY_MMAP;
  dqbuf.m.planes = planes;
  dqbuf.length = 1;
  if (device_->Ioctl(VIDIOC_DQBUF, &dqbuf) != 0) {
    if (errno == EAGAIN) {
      // EAGAIN if we're just out of buffers to dequeue.
      return false;
    }
    VPLOGF(1) << "ioctl() failed: VIDIOC_DQBUF";
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return false;
  }
  InputRecord& input_record = input_buffer_map_[dqbuf.index];
  DCHECK(input_record.at_device);
  free_input_buffers_.push_back(dqbuf.index);
  input_record.at_device = false;
  input_record.bytes_used = 0;
  input_record.input_id = -1;
  input_buffer_queued_count_--;

  return true;
}

bool V4L2VideoDecodeAccelerator::DequeueOutputBuffer() {
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_GT(output_buffer_queued_count_, 0);
  DCHECK(output_streamon_);

  // Dequeue a completed output (VIDEO_CAPTURE) buffer, and queue to the
  // completed queue.
  struct v4l2_buffer dqbuf;
  std::unique_ptr<struct v4l2_plane[]> planes(
      new v4l2_plane[output_planes_count_]);
  memset(&dqbuf, 0, sizeof(dqbuf));
  memset(planes.get(), 0, sizeof(struct v4l2_plane) * output_planes_count_);
  dqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  dqbuf.memory = V4L2_MEMORY_MMAP;
  dqbuf.m.planes = planes.get();
  dqbuf.length = output_planes_count_;
  if (device_->Ioctl(VIDIOC_DQBUF, &dqbuf) != 0) {
    if (errno == EAGAIN) {
      // EAGAIN if we're just out of buffers to dequeue.
      return false;
    } else if (errno == EPIPE) {
      DVLOGF(3) << "Got EPIPE. Last output buffer was already dequeued.";
      return false;
    }
    VPLOGF(1) << "ioctl() failed: VIDIOC_DQBUF";
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return false;
  }
  OutputRecord& output_record = output_buffer_map_[dqbuf.index];
  DCHECK_EQ(output_record.state, kAtDevice);
  DCHECK_NE(output_record.picture_id, -1);
  output_buffer_queued_count_--;
  if (dqbuf.m.planes[0].bytesused == 0) {
    // This is an empty output buffer returned as part of a flush.
    output_record.state = kFree;
    free_output_buffers_.push_back(dqbuf.index);
  } else {
    int32_t bitstream_buffer_id = dqbuf.timestamp.tv_sec;
    DCHECK_GE(bitstream_buffer_id, 0);
    DVLOGF(4) << "Dequeue output buffer: dqbuf index=" << dqbuf.index
              << " bitstream input_id=" << bitstream_buffer_id;
    if (image_processor_device_) {
      if (!ProcessFrame(bitstream_buffer_id, dqbuf.index)) {
        VLOGF(1) << "Processing frame failed";
        NOTIFY_ERROR(PLATFORM_FAILURE);
        return false;
      }
    } else {
      output_record.state = kAtClient;
      decoder_frames_at_client_++;
      // TODO(hubbe): Insert correct color space. http://crbug.com/647725
      const Picture picture(output_record.picture_id, bitstream_buffer_id,
                            gfx::Rect(visible_size_), gfx::ColorSpace(), false);
      pending_picture_ready_.push(
          PictureRecord(output_record.cleared, picture));
      SendPictureReady();
      output_record.cleared = true;
    }
  }
  if (dqbuf.flags & V4L2_BUF_FLAG_LAST) {
    DVLOGF(3) << "Got last output buffer. Waiting last buffer="
              << flush_awaiting_last_output_buffer_;
    if (flush_awaiting_last_output_buffer_) {
      flush_awaiting_last_output_buffer_ = false;
      struct v4l2_decoder_cmd cmd;
      memset(&cmd, 0, sizeof(cmd));
      cmd.cmd = V4L2_DEC_CMD_START;
      IOCTL_OR_ERROR_RETURN_FALSE(VIDIOC_DECODER_CMD, &cmd);
    }
  }
  return true;
}

bool V4L2VideoDecodeAccelerator::EnqueueInputRecord() {
  DVLOGF(4);
  DCHECK(!input_ready_queue_.empty());

  // Enqueue an input (VIDEO_OUTPUT) buffer.
  const int buffer = input_ready_queue_.front();
  InputRecord& input_record = input_buffer_map_[buffer];
  DCHECK(!input_record.at_device);
  struct v4l2_buffer qbuf;
  struct v4l2_plane qbuf_plane;
  memset(&qbuf, 0, sizeof(qbuf));
  memset(&qbuf_plane, 0, sizeof(qbuf_plane));
  qbuf.index = buffer;
  qbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  qbuf.timestamp.tv_sec = input_record.input_id;
  qbuf.memory = V4L2_MEMORY_MMAP;
  qbuf.m.planes = &qbuf_plane;
  qbuf.m.planes[0].bytesused = input_record.bytes_used;
  qbuf.length = 1;
  IOCTL_OR_ERROR_RETURN_FALSE(VIDIOC_QBUF, &qbuf);
  input_ready_queue_.pop();
  input_record.at_device = true;
  input_buffer_queued_count_++;
  DVLOGF(4) << "enqueued input_id=" << input_record.input_id
            << " size=" << input_record.bytes_used;
  return true;
}

bool V4L2VideoDecodeAccelerator::EnqueueOutputRecord() {
  DCHECK(!free_output_buffers_.empty());

  // Enqueue an output (VIDEO_CAPTURE) buffer.
  const int buffer = free_output_buffers_.front();
  DVLOGF(4) << "buffer " << buffer;
  OutputRecord& output_record = output_buffer_map_[buffer];
  DCHECK_EQ(output_record.state, kFree);
  DCHECK_NE(output_record.picture_id, -1);
  if (output_record.egl_sync != EGL_NO_SYNC_KHR) {
    TRACE_EVENT0("media,gpu",
                 "V4L2VDA::EnqueueOutputRecord: eglClientWaitSyncKHR");
    // If we have to wait for completion, wait.  Note that
    // free_output_buffers_ is a FIFO queue, so we always wait on the
    // buffer that has been in the queue the longest.
    if (eglClientWaitSyncKHR(egl_display_, output_record.egl_sync, 0,
                             EGL_FOREVER_KHR) == EGL_FALSE) {
      // This will cause tearing, but is safe otherwise.
      DVLOGF(1) << "eglClientWaitSyncKHR failed!";
    }
    if (eglDestroySyncKHR(egl_display_, output_record.egl_sync) != EGL_TRUE) {
      VLOGF(1) << "eglDestroySyncKHR failed!";
      NOTIFY_ERROR(PLATFORM_FAILURE);
      return false;
    }
    output_record.egl_sync = EGL_NO_SYNC_KHR;
  }
  struct v4l2_buffer qbuf;
  std::unique_ptr<struct v4l2_plane[]> qbuf_planes(
      new v4l2_plane[output_planes_count_]);
  memset(&qbuf, 0, sizeof(qbuf));
  memset(qbuf_planes.get(), 0,
         sizeof(struct v4l2_plane) * output_planes_count_);
  qbuf.index = buffer;
  qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  if (!image_processor_device_ && output_mode_ == Config::OutputMode::IMPORT) {
    DCHECK_EQ(output_planes_count_, output_record.output_fds.size());
    for (size_t i = 0; i < output_planes_count_; ++i) {
      qbuf_planes[i].m.fd = output_record.output_fds[i].get();
    }
    qbuf.memory = V4L2_MEMORY_DMABUF;
  } else {
    qbuf.memory = V4L2_MEMORY_MMAP;
  }
  qbuf.m.planes = qbuf_planes.get();
  qbuf.length = output_planes_count_;
  DVLOGF(4) << "qbuf.index=" << qbuf.index;
  IOCTL_OR_ERROR_RETURN_FALSE(VIDIOC_QBUF, &qbuf);
  free_output_buffers_.pop_front();
  output_record.state = kAtDevice;
  output_buffer_queued_count_++;
  return true;
}

void V4L2VideoDecodeAccelerator::ReusePictureBufferTask(
    int32_t picture_buffer_id,
    std::unique_ptr<EGLSyncKHRRef> egl_sync_ref) {
  DVLOGF(4) << "picture_buffer_id=" << picture_buffer_id;
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  TRACE_EVENT0("media,gpu", "V4L2VDA::ReusePictureBufferTask");

  // We run ReusePictureBufferTask even if we're in kResetting.
  if (decoder_state_ == kError) {
    DVLOGF(4) << "early out: kError state";
    return;
  }

  if (decoder_state_ == kChangingResolution) {
    DVLOGF(4) << "early out: kChangingResolution";
    return;
  }

  size_t index;
  for (index = 0; index < output_buffer_map_.size(); ++index)
    if (output_buffer_map_[index].picture_id == picture_buffer_id)
      break;

  if (index >= output_buffer_map_.size()) {
    // It's possible that we've already posted a DismissPictureBuffer for this
    // picture, but it has not yet executed when this ReusePictureBuffer was
    // posted to us by the client. In that case just ignore this (we've already
    // dismissed it and accounted for that) and let the sync object get
    // destroyed.
    DVLOGF(3) << "got picture id= " << picture_buffer_id
              << " not in use (anymore?).";
    return;
  }

  OutputRecord& output_record = output_buffer_map_[index];
  if (output_record.state != kAtClient) {
    VLOGF(1) << "picture_buffer_id not reusable";
    NOTIFY_ERROR(INVALID_ARGUMENT);
    return;
  }

  DCHECK_EQ(output_record.egl_sync, EGL_NO_SYNC_KHR);
  output_record.state = kFree;
  free_output_buffers_.push_back(index);
  decoder_frames_at_client_--;
  if (egl_sync_ref) {
    output_record.egl_sync = egl_sync_ref->egl_sync;
    // Take ownership of the EGLSync.
    egl_sync_ref->egl_sync = EGL_NO_SYNC_KHR;
  }
  // We got a buffer back, so enqueue it back.
  Enqueue();
}

void V4L2VideoDecodeAccelerator::FlushTask() {
  VLOGF(2);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  TRACE_EVENT0("media,gpu", "V4L2VDA::FlushTask");

  if (decoder_state_ == kError) {
    VLOGF(2) << "early out: kError state";
    return;
  }

  // We don't support stacked flushing.
  DCHECK(!decoder_flushing_);

  // Queue up an empty buffer -- this triggers the flush.
  decoder_input_queue_.push(std::make_unique<BitstreamBufferRef>(
      decode_client_, decode_task_runner_, nullptr, kFlushBufferId));
  decoder_flushing_ = true;
  SendPictureReady();  // Send all pending PictureReady.

  ScheduleDecodeBufferTaskIfNeeded();
}

void V4L2VideoDecodeAccelerator::NotifyFlushDoneIfNeeded() {
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  if (!decoder_flushing_)
    return;

  // Pipeline is empty when:
  // * Decoder input queue is empty of non-delayed buffers.
  // * There is no currently filling input buffer.
  // * Input holding queue is empty.
  // * All input (VIDEO_OUTPUT) buffers are returned.
  // * All image processor buffers are returned.
  if (!decoder_input_queue_.empty()) {
    if (decoder_input_queue_.front()->input_id !=
        decoder_delay_bitstream_buffer_id_) {
      DVLOGF(3) << "Some input bitstream buffers are not queued.";
      return;
    }
  }
  if (decoder_current_input_buffer_ != -1) {
    DVLOGF(3) << "Current input buffer != -1";
    return;
  }
  if ((input_ready_queue_.size() + input_buffer_queued_count_) != 0) {
    DVLOGF(3) << "Some input buffers are not dequeued.";
    return;
  }
  if (image_processor_bitstream_buffer_ids_.size() != 0) {
    DVLOGF(3) << "Waiting for image processor to complete.";
    return;
  }
  if (flush_awaiting_last_output_buffer_) {
    DVLOGF(3) << "Waiting for last output buffer.";
    return;
  }

  // TODO(posciak): https://crbug.com/270039. Exynos requires a
  // streamoff-streamon sequence after flush to continue, even if we are not
  // resetting. This would make sense, because we don't really want to resume
  // from a non-resume point (e.g. not from an IDR) if we are flushed.
  // MSE player however triggers a Flush() on chunk end, but never Reset(). One
  // could argue either way, or even say that Flush() is not needed/harmful when
  // transitioning to next chunk.
  // For now, do the streamoff-streamon cycle to satisfy Exynos and not freeze
  // when doing MSE. This should be harmless otherwise.
  if (!(StopDevicePoll() && StopOutputStream() && StopInputStream()))
    return;

  if (!StartDevicePoll())
    return;

  decoder_delay_bitstream_buffer_id_ = -1;
  decoder_flushing_ = false;
  VLOGF(2) << "returning flush";
  child_task_runner_->PostTask(FROM_HERE,
                               base::Bind(&Client::NotifyFlushDone, client_));

  // While we were flushing, we early-outed DecodeBufferTask()s.
  ScheduleDecodeBufferTaskIfNeeded();
}

bool V4L2VideoDecodeAccelerator::IsDecoderCmdSupported() {
  // CMD_STOP should always succeed. If the decoder is started, the command can
  // flush it. If the decoder is stopped, the command does nothing. We use this
  // to know if a driver supports V4L2_DEC_CMD_STOP to flush.
  struct v4l2_decoder_cmd cmd;
  memset(&cmd, 0, sizeof(cmd));
  cmd.cmd = V4L2_DEC_CMD_STOP;
  if (device_->Ioctl(VIDIOC_TRY_DECODER_CMD, &cmd) != 0) {
    VLOGF(2) "V4L2_DEC_CMD_STOP is not supported.";
    return false;
  }

  return true;
}

bool V4L2VideoDecodeAccelerator::SendDecoderCmdStop() {
  VLOGF(2);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK(!flush_awaiting_last_output_buffer_);

  struct v4l2_decoder_cmd cmd;
  memset(&cmd, 0, sizeof(cmd));
  cmd.cmd = V4L2_DEC_CMD_STOP;
  IOCTL_OR_ERROR_RETURN_FALSE(VIDIOC_DECODER_CMD, &cmd);
  flush_awaiting_last_output_buffer_ = true;

  return true;
}

void V4L2VideoDecodeAccelerator::ResetTask() {
  VLOGF(2);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  TRACE_EVENT0("media,gpu", "V4L2VDA::ResetTask");

  if (decoder_state_ == kError) {
    VLOGF(2) << "early out: kError state";
    return;
  }
  decoder_current_bitstream_buffer_.reset();
  while (!decoder_input_queue_.empty())
    decoder_input_queue_.pop();

  decoder_current_input_buffer_ = -1;

  // If we are in the middle of switching resolutions or awaiting picture
  // buffers, postpone reset until it's done. We don't have to worry about
  // timing of this wrt to decoding, because output pipe is already
  // stopped if we are changing resolution. We will come back here after
  // we are done.
  DCHECK(!reset_pending_);
  if (decoder_state_ == kChangingResolution ||
      decoder_state_ == kAwaitingPictureBuffers) {
    reset_pending_ = true;
    return;
  }
  FinishReset();
}

void V4L2VideoDecodeAccelerator::FinishReset() {
  VLOGF(2);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());

  reset_pending_ = false;
  // After the output stream is stopped, the codec should not post any
  // resolution change events. So we dequeue the resolution change event
  // afterwards. The event could be posted before or while stopping the output
  // stream. The codec will expect the buffer of new size after the seek, so
  // we need to handle the resolution change event first.
  if (!(StopDevicePoll() && StopOutputStream()))
    return;

  if (DequeueResolutionChangeEvent()) {
    reset_pending_ = true;
    StartResolutionChange();
    return;
  }

  if (!StopInputStream())
    return;

  // Drop all buffers in image processor.
  if (image_processor_ && !ResetImageProcessor()) {
    VLOGF(1) << "Fail to reset image processor";
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return;
  }

  // If we were flushing, we'll never return any more BitstreamBuffers or
  // PictureBuffers; they have all been dropped and returned by now.
  NotifyFlushDoneIfNeeded();

  // Mark that we're resetting, then enqueue a ResetDoneTask().  All intervening
  // jobs will early-out in the kResetting state.
  decoder_state_ = kResetting;
  SendPictureReady();  // Send all pending PictureReady.
  decoder_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::ResetDoneTask,
                            base::Unretained(this)));
}

void V4L2VideoDecodeAccelerator::ResetDoneTask() {
  VLOGF(2);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  TRACE_EVENT0("media,gpu", "V4L2VDA::ResetDoneTask");

  if (decoder_state_ == kError) {
    VLOGF(2) << "early out: kError state";
    return;
  }

  // Start poll thread if NotifyFlushDoneIfNeeded has not already.
  if (!device_poll_thread_.IsRunning()) {
    if (!StartDevicePoll())
      return;
  }

  // Reset format-specific bits.
  if (video_profile_ >= H264PROFILE_MIN && video_profile_ <= H264PROFILE_MAX) {
    decoder_h264_parser_.reset(new H264Parser());
  }

  // Jobs drained, we're finished resetting.
  DCHECK_EQ(decoder_state_, kResetting);
  decoder_state_ = kInitialized;

  decoder_partial_frame_pending_ = false;
  decoder_delay_bitstream_buffer_id_ = -1;
  child_task_runner_->PostTask(FROM_HERE,
                               base::Bind(&Client::NotifyResetDone, client_));

  // While we were resetting, we early-outed DecodeBufferTask()s.
  ScheduleDecodeBufferTaskIfNeeded();
}

void V4L2VideoDecodeAccelerator::DestroyTask() {
  VLOGF(2);
  TRACE_EVENT0("media,gpu", "V4L2VDA::DestroyTask");

  // DestroyTask() should run regardless of decoder_state_.

  StopDevicePoll();
  StopOutputStream();
  StopInputStream();

  decoder_current_bitstream_buffer_.reset();
  decoder_current_input_buffer_ = -1;
  decoder_decode_buffer_tasks_scheduled_ = 0;
  decoder_frames_at_client_ = 0;
  while (!decoder_input_queue_.empty())
    decoder_input_queue_.pop();
  decoder_flushing_ = false;

  if (image_processor_)
    image_processor_.release()->Destroy();

  // Set our state to kError.  Just in case.
  decoder_state_ = kError;

  DestroyInputBuffers();
  DestroyOutputBuffers();
}

bool V4L2VideoDecodeAccelerator::StartDevicePoll() {
  DVLOGF(3);
  DCHECK(!device_poll_thread_.IsRunning());
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());

  // Start up the device poll thread and schedule its first DevicePollTask().
  if (!device_poll_thread_.Start()) {
    VLOGF(1) << "Device thread failed to start";
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return false;
  }
  device_poll_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::DevicePollTask,
                            base::Unretained(this), 0));

  return true;
}

bool V4L2VideoDecodeAccelerator::StopDevicePoll() {
  DVLOGF(3);

  if (!device_poll_thread_.IsRunning())
    return true;

  if (decoder_thread_.IsRunning())
    DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());

  // Signal the DevicePollTask() to stop, and stop the device poll thread.
  if (!device_->SetDevicePollInterrupt()) {
    VPLOGF(1) << "SetDevicePollInterrupt(): failed";
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return false;
  }
  device_poll_thread_.Stop();
  // Clear the interrupt now, to be sure.
  if (!device_->ClearDevicePollInterrupt()) {
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return false;
  }
  DVLOGF(3) << "device poll stopped";
  return true;
}

bool V4L2VideoDecodeAccelerator::StopOutputStream() {
  VLOGF(2);
  if (!output_streamon_)
    return true;

  __u32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  IOCTL_OR_ERROR_RETURN_FALSE(VIDIOC_STREAMOFF, &type);
  output_streamon_ = false;

  // Output stream is stopped. No need to wait for the buffer anymore.
  flush_awaiting_last_output_buffer_ = false;

  for (size_t i = 0; i < output_buffer_map_.size(); ++i) {
    // After streamoff, the device drops ownership of all buffers, even if we
    // don't dequeue them explicitly. Some of them may still be owned by the
    // client however. Reuse only those that aren't.
    OutputRecord& output_record = output_buffer_map_[i];
    if (output_record.state == kAtDevice) {
      output_record.state = kFree;
      free_output_buffers_.push_back(i);
      DCHECK_EQ(output_record.egl_sync, EGL_NO_SYNC_KHR);
    }
  }
  output_buffer_queued_count_ = 0;
  return true;
}

bool V4L2VideoDecodeAccelerator::StopInputStream() {
  VLOGF(2);
  if (!input_streamon_)
    return true;

  __u32 type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  IOCTL_OR_ERROR_RETURN_FALSE(VIDIOC_STREAMOFF, &type);
  input_streamon_ = false;

  // Reset accounting info for input.
  while (!input_ready_queue_.empty())
    input_ready_queue_.pop();
  free_input_buffers_.clear();
  for (size_t i = 0; i < input_buffer_map_.size(); ++i) {
    free_input_buffers_.push_back(i);
    input_buffer_map_[i].at_device = false;
    input_buffer_map_[i].bytes_used = 0;
    input_buffer_map_[i].input_id = -1;
  }
  input_buffer_queued_count_ = 0;

  return true;
}

void V4L2VideoDecodeAccelerator::StartResolutionChange() {
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_NE(decoder_state_, kUninitialized);
  DCHECK_NE(decoder_state_, kResetting);

  VLOGF(2) << "Initiate resolution change";

  if (!(StopDevicePoll() && StopOutputStream()))
    return;

  decoder_state_ = kChangingResolution;
  SendPictureReady();  // Send all pending PictureReady.

  if (!image_processor_bitstream_buffer_ids_.empty()) {
    VLOGF(2) << "Wait image processor to finish before destroying buffers.";
    return;
  }

  if (image_processor_)
    image_processor_.release()->Destroy();

  if (!DestroyOutputBuffers()) {
    VLOGF(1) << "Failed destroying output buffers.";
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return;
  }

  FinishResolutionChange();
}

void V4L2VideoDecodeAccelerator::FinishResolutionChange() {
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_EQ(decoder_state_, kChangingResolution);
  VLOGF(2);

  if (decoder_state_ == kError) {
    VLOGF(2) << "early out: kError state";
    return;
  }

  struct v4l2_format format;
  bool again;
  gfx::Size visible_size;
  bool ret = GetFormatInfo(&format, &visible_size, &again);
  if (!ret || again) {
    VLOGF(1) << "Couldn't get format information after resolution change";
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return;
  }

  if (!CreateBuffersForFormat(format, visible_size)) {
    VLOGF(1) << "Couldn't reallocate buffers after resolution change";
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return;
  }

  if (!StartDevicePoll())
    return;
}

void V4L2VideoDecodeAccelerator::DevicePollTask(bool poll_device) {
  DVLOGF(4);
  DCHECK(device_poll_thread_.task_runner()->BelongsToCurrentThread());
  TRACE_EVENT0("media,gpu", "V4L2VDA::DevicePollTask");

  bool event_pending = false;

  if (!device_->Poll(poll_device, &event_pending)) {
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return;
  }

  // All processing should happen on ServiceDeviceTask(), since we shouldn't
  // touch decoder state from this thread.
  decoder_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::ServiceDeviceTask,
                            base::Unretained(this), event_pending));
}

void V4L2VideoDecodeAccelerator::NotifyError(Error error) {
  VLOGF(1);

  if (!child_task_runner_->BelongsToCurrentThread()) {
    child_task_runner_->PostTask(
        FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::NotifyError,
                              weak_this_, error));
    return;
  }

  if (client_) {
    client_->NotifyError(error);
    client_ptr_factory_.reset();
  }
}

void V4L2VideoDecodeAccelerator::SetErrorState(Error error) {
  // We can touch decoder_state_ only if this is the decoder thread or the
  // decoder thread isn't running.
  if (decoder_thread_.task_runner() &&
      !decoder_thread_.task_runner()->BelongsToCurrentThread()) {
    decoder_thread_.task_runner()->PostTask(
        FROM_HERE, base::Bind(&V4L2VideoDecodeAccelerator::SetErrorState,
                              base::Unretained(this), error));
    return;
  }

  // Post NotifyError only if we are already initialized, as the API does
  // not allow doing so before that.
  if (decoder_state_ != kError && decoder_state_ != kUninitialized)
    NotifyError(error);

  decoder_state_ = kError;
}

bool V4L2VideoDecodeAccelerator::GetFormatInfo(struct v4l2_format* format,
                                               gfx::Size* visible_size,
                                               bool* again) {
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());

  *again = false;
  memset(format, 0, sizeof(*format));
  format->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  if (device_->Ioctl(VIDIOC_G_FMT, format) != 0) {
    if (errno == EINVAL) {
      // EINVAL means we haven't seen sufficient stream to decode the format.
      *again = true;
      return true;
    } else {
      VPLOGF(1) << "ioctl() failed: VIDIOC_G_FMT";
      NOTIFY_ERROR(PLATFORM_FAILURE);
      return false;
    }
  }

  // Make sure we are still getting the format we set on initialization.
  if (format->fmt.pix_mp.pixelformat != output_format_fourcc_) {
    VLOGF(1) << "Unexpected format from G_FMT on output";
    return false;
  }

  gfx::Size coded_size(format->fmt.pix_mp.width, format->fmt.pix_mp.height);
  if (visible_size != nullptr)
    *visible_size = GetVisibleSize(coded_size);

  return true;
}

bool V4L2VideoDecodeAccelerator::CreateBuffersForFormat(
    const struct v4l2_format& format,
    const gfx::Size& visible_size) {
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  output_planes_count_ = format.fmt.pix_mp.num_planes;
  coded_size_.SetSize(format.fmt.pix_mp.width, format.fmt.pix_mp.height);
  visible_size_ = visible_size;
  if (image_processor_device_) {
    egl_image_size_ = visible_size_;
    egl_image_planes_count_ = 0;
    if (!V4L2ImageProcessor::TryOutputFormat(
            output_format_fourcc_, egl_image_format_fourcc_, &egl_image_size_,
            &egl_image_planes_count_)) {
      VLOGF(1) << "Fail to get output size and plane count of processor";
      return false;
    }
  } else {
    egl_image_size_ = coded_size_;
    egl_image_planes_count_ = output_planes_count_;
  }
  VLOGF(2) << "new resolution: " << coded_size_.ToString()
           << ", visible size: " << visible_size_.ToString()
           << ", decoder output planes count: " << output_planes_count_
           << ", EGLImage size: " << egl_image_size_.ToString()
           << ", EGLImage plane count: " << egl_image_planes_count_;

  return CreateOutputBuffers();
}

gfx::Size V4L2VideoDecodeAccelerator::GetVisibleSize(
    const gfx::Size& coded_size) {
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());

  struct v4l2_rect* visible_rect;
  struct v4l2_selection selection_arg;
  memset(&selection_arg, 0, sizeof(selection_arg));
  selection_arg.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  selection_arg.target = V4L2_SEL_TGT_COMPOSE;

  if (device_->Ioctl(VIDIOC_G_SELECTION, &selection_arg) == 0) {
    VLOGF(2) << "VIDIOC_G_SELECTION is supported";
    visible_rect = &selection_arg.r;
  } else {
    VLOGF(2) << "Fallback to VIDIOC_G_CROP";
    struct v4l2_crop crop_arg;
    memset(&crop_arg, 0, sizeof(crop_arg));
    crop_arg.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (device_->Ioctl(VIDIOC_G_CROP, &crop_arg) != 0) {
      VPLOGF(1) << "ioctl() VIDIOC_G_CROP failed";
      return coded_size;
    }
    visible_rect = &crop_arg.c;
  }

  gfx::Rect rect(visible_rect->left, visible_rect->top, visible_rect->width,
                 visible_rect->height);
  VLOGF(2) << "visible rectangle is " << rect.ToString();
  if (!gfx::Rect(coded_size).Contains(rect)) {
    DVLOGF(3) << "visible rectangle " << rect.ToString()
              << " is not inside coded size " << coded_size.ToString();
    return coded_size;
  }
  if (rect.IsEmpty()) {
    VLOGF(1) << "visible size is empty";
    return coded_size;
  }

  // Chrome assume picture frame is coded at (0, 0).
  if (!rect.origin().IsOrigin()) {
    VLOGF(1) << "Unexpected visible rectangle " << rect.ToString()
             << ", top-left is not origin";
    return coded_size;
  }

  return rect.size();
}

bool V4L2VideoDecodeAccelerator::CreateInputBuffers() {
  VLOGF(2);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  // We always run this as we prepare to initialize.
  DCHECK_EQ(decoder_state_, kInitialized);
  DCHECK(!input_streamon_);
  DCHECK(input_buffer_map_.empty());

  struct v4l2_requestbuffers reqbufs;
  memset(&reqbufs, 0, sizeof(reqbufs));
  reqbufs.count = kInputBufferCount;
  reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  reqbufs.memory = V4L2_MEMORY_MMAP;
  IOCTL_OR_ERROR_RETURN_FALSE(VIDIOC_REQBUFS, &reqbufs);
  input_buffer_map_.resize(reqbufs.count);
  for (size_t i = 0; i < input_buffer_map_.size(); ++i) {
    free_input_buffers_.push_back(i);

    // Query for the MEMORY_MMAP pointer.
    struct v4l2_plane planes[1];
    struct v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    memset(planes, 0, sizeof(planes));
    buffer.index = i;
    buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.m.planes = planes;
    buffer.length = 1;
    IOCTL_OR_ERROR_RETURN_FALSE(VIDIOC_QUERYBUF, &buffer);
    void* address = device_->Mmap(NULL,
                                  buffer.m.planes[0].length,
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED,
                                  buffer.m.planes[0].m.mem_offset);
    if (address == MAP_FAILED) {
      VPLOGF(1) << "mmap() failed";
      return false;
    }
    input_buffer_map_[i].address = address;
    input_buffer_map_[i].length = buffer.m.planes[0].length;
  }

  return true;
}

bool V4L2VideoDecodeAccelerator::SetupFormats() {
  // We always run this as we prepare to initialize.
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(decoder_state_, kUninitialized);
  DCHECK(!input_streamon_);
  DCHECK(!output_streamon_);

  size_t input_size;
  gfx::Size max_resolution, min_resolution;
  device_->GetSupportedResolution(input_format_fourcc_, &min_resolution,
                                  &max_resolution);
  if (max_resolution.width() > 1920 && max_resolution.height() > 1088)
    input_size = kInputBufferMaxSizeFor4k;
  else
    input_size = kInputBufferMaxSizeFor1080p;

  struct v4l2_fmtdesc fmtdesc;
  memset(&fmtdesc, 0, sizeof(fmtdesc));
  fmtdesc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  bool is_format_supported = false;
  while (device_->Ioctl(VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
    if (fmtdesc.pixelformat == input_format_fourcc_) {
      is_format_supported = true;
      break;
    }
    ++fmtdesc.index;
  }

  if (!is_format_supported) {
    VLOGF(1) << "Input fourcc " << input_format_fourcc_
             << " not supported by device.";
    return false;
  }

  struct v4l2_format format;
  memset(&format, 0, sizeof(format));
  format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  format.fmt.pix_mp.pixelformat = input_format_fourcc_;
  format.fmt.pix_mp.plane_fmt[0].sizeimage = input_size;
  format.fmt.pix_mp.num_planes = 1;
  IOCTL_OR_ERROR_RETURN_FALSE(VIDIOC_S_FMT, &format);

  // We have to set up the format for output, because the driver may not allow
  // changing it once we start streaming; whether it can support our chosen
  // output format or not may depend on the input format.
  memset(&fmtdesc, 0, sizeof(fmtdesc));
  fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  while (device_->Ioctl(VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
    if (device_->CanCreateEGLImageFrom(fmtdesc.pixelformat)) {
      output_format_fourcc_ = fmtdesc.pixelformat;
      break;
    }
    ++fmtdesc.index;
  }

  DCHECK(!image_processor_device_);
  if (output_format_fourcc_ == 0) {
    VLOGF(2) << "Could not find a usable output format. Try image processor";
    if (!V4L2ImageProcessor::IsSupported()) {
      VLOGF(1) << "Image processor not available";
      return false;
    }
    output_format_fourcc_ = FindImageProcessorInputFormat();
    if (output_format_fourcc_ == 0) {
      VLOGF(1) << "Can't find a usable input format from image processor";
      return false;
    }
    egl_image_format_fourcc_ = FindImageProcessorOutputFormat();
    if (egl_image_format_fourcc_ == 0) {
      VLOGF(1) << "Can't find a usable output format from image processor";
      return false;
    }
    image_processor_device_ = V4L2Device::Create();
    if (!image_processor_device_) {
      VLOGF(1) << "Could not create a V4L2Device for image processor";
      return false;
    }
    egl_image_device_ = image_processor_device_;
  } else {
    egl_image_format_fourcc_ = output_format_fourcc_;
    egl_image_device_ = device_;
  }
  VLOGF(2) << "Output format=" << output_format_fourcc_;

  // Just set the fourcc for output; resolution, etc., will come from the
  // driver once it extracts it from the stream.
  memset(&format, 0, sizeof(format));
  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  format.fmt.pix_mp.pixelformat = output_format_fourcc_;
  IOCTL_OR_ERROR_RETURN_FALSE(VIDIOC_S_FMT, &format);

  return true;
}

uint32_t V4L2VideoDecodeAccelerator::FindImageProcessorInputFormat() {
  std::vector<uint32_t> processor_input_formats =
      V4L2ImageProcessor::GetSupportedInputFormats();

  struct v4l2_fmtdesc fmtdesc;
  memset(&fmtdesc, 0, sizeof(fmtdesc));
  fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  while (device_->Ioctl(VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
    if (std::find(processor_input_formats.begin(),
                  processor_input_formats.end(),
                  fmtdesc.pixelformat) != processor_input_formats.end()) {
      VLOGF(2) << "Image processor input format=" << fmtdesc.description;
      return fmtdesc.pixelformat;
    }
    ++fmtdesc.index;
  }
  return 0;
}

uint32_t V4L2VideoDecodeAccelerator::FindImageProcessorOutputFormat() {
  // Prefer YVU420 and NV12 because ArcGpuVideoDecodeAccelerator only supports
  // single physical plane. Prefer YVU420 over NV12 because chrome rendering
  // supports YV12 only.
  static const uint32_t kPreferredFormats[] = {V4L2_PIX_FMT_YVU420,
                                               V4L2_PIX_FMT_NV12};
  auto preferred_formats_first = [](uint32_t a, uint32_t b) -> bool {
    auto* iter_a = std::find(std::begin(kPreferredFormats),
                             std::end(kPreferredFormats), a);
    auto* iter_b = std::find(std::begin(kPreferredFormats),
                             std::end(kPreferredFormats), b);
    return iter_a < iter_b;
  };

  std::vector<uint32_t> processor_output_formats =
      V4L2ImageProcessor::GetSupportedOutputFormats();

  // Move the preferred formats to the front.
  std::sort(processor_output_formats.begin(), processor_output_formats.end(),
            preferred_formats_first);

  for (uint32_t processor_output_format : processor_output_formats) {
    if (device_->CanCreateEGLImageFrom(processor_output_format)) {
      VLOGF(2) << "Image processor output format=" << processor_output_format;
      return processor_output_format;
    }
  }

  return 0;
}

bool V4L2VideoDecodeAccelerator::ResetImageProcessor() {
  VLOGF(2);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());

  if (!image_processor_->Reset())
    return false;
  for (size_t i = 0; i < output_buffer_map_.size(); ++i) {
    OutputRecord& output_record = output_buffer_map_[i];
    if (output_record.state == kAtProcessor) {
      output_record.state = kFree;
      free_output_buffers_.push_back(i);
    }
  }
  while (!image_processor_bitstream_buffer_ids_.empty())
    image_processor_bitstream_buffer_ids_.pop();

  return true;
}

bool V4L2VideoDecodeAccelerator::CreateImageProcessor() {
  VLOGF(2);
  DCHECK(!image_processor_);
  image_processor_.reset(new V4L2ImageProcessor(image_processor_device_));
  v4l2_memory output_memory_type =
      (output_mode_ == Config::OutputMode::ALLOCATE ? V4L2_MEMORY_MMAP
                                                    : V4L2_MEMORY_DMABUF);
  // Unretained is safe because |this| owns image processor and there will be
  // no callbacks after processor destroys.
  if (!image_processor_->Initialize(
          V4L2Device::V4L2PixFmtToVideoPixelFormat(output_format_fourcc_),
          V4L2Device::V4L2PixFmtToVideoPixelFormat(egl_image_format_fourcc_),
          V4L2_MEMORY_DMABUF, output_memory_type, visible_size_, coded_size_,
          visible_size_, egl_image_size_, output_buffer_map_.size(),
          base::Bind(&V4L2VideoDecodeAccelerator::ImageProcessorError,
                     base::Unretained(this)))) {
    VLOGF(1) << "Initialize image processor failed";
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return false;
  }
  VLOGF(2) << "image_processor_->output_allocated_size()="
           << image_processor_->output_allocated_size().ToString();
  DCHECK(image_processor_->output_allocated_size() == egl_image_size_);
  if (image_processor_->input_allocated_size() != coded_size_) {
    VLOGF(1) << "Image processor should be able to take the output coded "
             << "size of decoder " << coded_size_.ToString()
             << " without adjusting to "
             << image_processor_->input_allocated_size().ToString();
    NOTIFY_ERROR(PLATFORM_FAILURE);
    return false;
  }
  return true;
}

bool V4L2VideoDecodeAccelerator::ProcessFrame(int32_t bitstream_buffer_id,
                                              int output_buffer_index) {
  DVLOGF(4);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());

  OutputRecord& output_record = output_buffer_map_[output_buffer_index];
  DCHECK_EQ(output_record.state, kAtDevice);
  output_record.state = kAtProcessor;
  image_processor_bitstream_buffer_ids_.push(bitstream_buffer_id);
  std::vector<int> processor_input_fds;
  for (auto& fd : output_record.processor_input_fds) {
    processor_input_fds.push_back(fd.get());
  }
  scoped_refptr<VideoFrame> input_frame = VideoFrame::WrapExternalDmabufs(
      V4L2Device::V4L2PixFmtToVideoPixelFormat(output_format_fourcc_),
      coded_size_, gfx::Rect(visible_size_), visible_size_, processor_input_fds,
      base::TimeDelta());

  std::vector<base::ScopedFD> output_fds;
  if (output_mode_ == Config::OutputMode::IMPORT) {
    for (auto& fd : output_record.output_fds) {
      output_fds.push_back(base::ScopedFD(HANDLE_EINTR(dup(fd.get()))));
      if (!output_fds.back().is_valid()) {
        VPLOGF(1) << "Failed duplicating a dmabuf fd";
        return false;
      }
    }
  }
  // Unretained is safe because |this| owns image processor and there will
  // be no callbacks after processor destroys.
  image_processor_->Process(
      input_frame, output_buffer_index, std::move(output_fds),
      base::Bind(&V4L2VideoDecodeAccelerator::FrameProcessed,
                 base::Unretained(this), bitstream_buffer_id));
  return true;
}

bool V4L2VideoDecodeAccelerator::CreateOutputBuffers() {
  VLOGF(2);
  DCHECK(decoder_state_ == kInitialized ||
         decoder_state_ == kChangingResolution);
  DCHECK(!output_streamon_);
  DCHECK(output_buffer_map_.empty());

  // Number of output buffers we need.
  struct v4l2_control ctrl;
  memset(&ctrl, 0, sizeof(ctrl));
  ctrl.id = V4L2_CID_MIN_BUFFERS_FOR_CAPTURE;
  IOCTL_OR_ERROR_RETURN_FALSE(VIDIOC_G_CTRL, &ctrl);
  output_dpb_size_ = ctrl.value;

  // Output format setup in Initialize().

  uint32_t buffer_count = output_dpb_size_ + kDpbOutputBufferExtraCount;
  if (image_processor_device_)
    buffer_count += kDpbOutputBufferExtraCountForImageProcessor;

  DVLOGF(3) << "buffer_count=" << buffer_count
            << ", coded_size=" << egl_image_size_.ToString();

  // With ALLOCATE mode the client can sample it as RGB and doesn't need to
  // know the precise format.
  VideoPixelFormat pixel_format =
      (output_mode_ == Config::OutputMode::IMPORT)
          ? V4L2Device::V4L2PixFmtToVideoPixelFormat(egl_image_format_fourcc_)
          : PIXEL_FORMAT_UNKNOWN;

  child_task_runner_->PostTask(
      FROM_HERE, base::Bind(&Client::ProvidePictureBuffers, client_,
                            buffer_count, pixel_format, 1, egl_image_size_,
                            device_->GetTextureTarget()));

  // Go into kAwaitingPictureBuffers to prevent us from doing any more decoding
  // or event handling while we are waiting for AssignPictureBuffers(). Not
  // having Pictures available would not have prevented us from making decoding
  // progress entirely e.g. in the case of H.264 where we could further decode
  // non-slice NALUs and could even get another resolution change before we were
  // done with this one. After we get the buffers, we'll go back into kIdle and
  // kick off further event processing, and eventually go back into kDecoding
  // once no more events are pending (if any).
  decoder_state_ = kAwaitingPictureBuffers;

  return true;
}

void V4L2VideoDecodeAccelerator::DestroyInputBuffers() {
  VLOGF(2);
  DCHECK(!decoder_thread_.IsRunning() ||
         decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK(!input_streamon_);

  if (input_buffer_map_.empty())
    return;

  for (size_t i = 0; i < input_buffer_map_.size(); ++i) {
    if (input_buffer_map_[i].address != NULL) {
      device_->Munmap(input_buffer_map_[i].address,
                      input_buffer_map_[i].length);
    }
  }

  struct v4l2_requestbuffers reqbufs;
  memset(&reqbufs, 0, sizeof(reqbufs));
  reqbufs.count = 0;
  reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
  reqbufs.memory = V4L2_MEMORY_MMAP;
  IOCTL_OR_LOG_ERROR(VIDIOC_REQBUFS, &reqbufs);

  input_buffer_map_.clear();
  free_input_buffers_.clear();
}

bool V4L2VideoDecodeAccelerator::DestroyOutputBuffers() {
  VLOGF(2);
  DCHECK(!decoder_thread_.IsRunning() ||
         decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK(!output_streamon_);
  bool success = true;

  if (output_buffer_map_.empty())
    return true;

  for (size_t i = 0; i < output_buffer_map_.size(); ++i) {
    OutputRecord& output_record = output_buffer_map_[i];

    if (output_record.egl_image != EGL_NO_IMAGE_KHR) {
      child_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(base::IgnoreResult(&V4L2Device::DestroyEGLImage), device_,
                     egl_display_, output_record.egl_image));
    }

    if (output_record.egl_sync != EGL_NO_SYNC_KHR) {
      if (eglDestroySyncKHR(egl_display_, output_record.egl_sync) != EGL_TRUE) {
        VLOGF(1) << "eglDestroySyncKHR failed.";
        success = false;
      }
    }

    DVLOGF(3) << "dismissing PictureBuffer id=" << output_record.picture_id;
    child_task_runner_->PostTask(
        FROM_HERE, base::Bind(&Client::DismissPictureBuffer, client_,
                              output_record.picture_id));
  }

  struct v4l2_requestbuffers reqbufs;
  memset(&reqbufs, 0, sizeof(reqbufs));
  reqbufs.count = 0;
  reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  if (!image_processor_device_ && output_mode_ == Config::OutputMode::IMPORT) {
    reqbufs.memory = V4L2_MEMORY_DMABUF;
  } else {
    reqbufs.memory = V4L2_MEMORY_MMAP;
  }
  if (device_->Ioctl(VIDIOC_REQBUFS, &reqbufs) != 0) {
    VPLOGF(1) << "ioctl() failed: VIDIOC_REQBUFS";
    NOTIFY_ERROR(PLATFORM_FAILURE);
    success = false;
  }

  output_buffer_map_.clear();
  while (!free_output_buffers_.empty())
    free_output_buffers_.pop_front();
  output_buffer_queued_count_ = 0;
  // The client may still hold some buffers. The texture holds a reference to
  // the buffer. It is OK to free the buffer and destroy EGLImage here.
  decoder_frames_at_client_ = 0;

  return success;
}

void V4L2VideoDecodeAccelerator::SendPictureReady() {
  DVLOGF(4);
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  bool send_now = (decoder_state_ == kChangingResolution ||
                   decoder_state_ == kResetting || decoder_flushing_);
  while (pending_picture_ready_.size() > 0) {
    bool cleared = pending_picture_ready_.front().cleared;
    const Picture& picture = pending_picture_ready_.front().picture;
    if (cleared && picture_clearing_count_ == 0) {
      // This picture is cleared. It can be posted to a thread different than
      // the main GPU thread to reduce latency. This should be the case after
      // all pictures are cleared at the beginning.
      decode_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&Client::PictureReady, decode_client_, picture));
      pending_picture_ready_.pop();
    } else if (!cleared || send_now) {
      DVLOGF(4) << "cleared=" << pending_picture_ready_.front().cleared
                << ", decoder_state_=" << decoder_state_
                << ", decoder_flushing_=" << decoder_flushing_
                << ", picture_clearing_count_=" << picture_clearing_count_;
      // If the picture is not cleared, post it to the child thread because it
      // has to be cleared in the child thread. A picture only needs to be
      // cleared once. If the decoder is changing resolution, resetting or
      // flushing, send all pictures to ensure PictureReady arrive before
      // ProvidePictureBuffers, NotifyResetDone, or NotifyFlushDone.
      child_task_runner_->PostTaskAndReply(
          FROM_HERE, base::Bind(&Client::PictureReady, client_, picture),
          // Unretained is safe. If Client::PictureReady gets to run, |this| is
          // alive. Destroy() will wait the decode thread to finish.
          base::Bind(&V4L2VideoDecodeAccelerator::PictureCleared,
                     base::Unretained(this)));
      picture_clearing_count_++;
      pending_picture_ready_.pop();
    } else {
      // This picture is cleared. But some pictures are about to be cleared on
      // the child thread. To preserve the order, do not send this until those
      // pictures are cleared.
      break;
    }
  }
}

void V4L2VideoDecodeAccelerator::PictureCleared() {
  DVLOGF(4) << "clearing count=" << picture_clearing_count_;
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK_GT(picture_clearing_count_, 0);
  picture_clearing_count_--;
  SendPictureReady();
}

void V4L2VideoDecodeAccelerator::FrameProcessed(int32_t bitstream_buffer_id,
                                                int output_buffer_index) {
  DVLOGF(4) << "output_buffer_index=" << output_buffer_index
            << ", bitstream_buffer_id=" << bitstream_buffer_id;
  DCHECK(decoder_thread_.task_runner()->BelongsToCurrentThread());
  DCHECK(!image_processor_bitstream_buffer_ids_.empty());
  DCHECK(image_processor_bitstream_buffer_ids_.front() == bitstream_buffer_id);
  DCHECK_GE(output_buffer_index, 0);
  DCHECK_LT(output_buffer_index, static_cast<int>(output_buffer_map_.size()));

  OutputRecord& output_record = output_buffer_map_[output_buffer_index];
  DVLOGF(4) << "picture_id=" << output_record.picture_id;
  DCHECK_EQ(output_record.state, kAtProcessor);
  DCHECK_NE(output_record.picture_id, -1);

  // Send the processed frame to render.
  output_record.state = kAtClient;
  decoder_frames_at_client_++;
  image_processor_bitstream_buffer_ids_.pop();
  // TODO(hubbe): Insert correct color space. http://crbug.com/647725
  const Picture picture(output_record.picture_id, bitstream_buffer_id,
                        gfx::Rect(visible_size_), gfx::ColorSpace(), false);
  pending_picture_ready_.push(PictureRecord(output_record.cleared, picture));
  SendPictureReady();
  output_record.cleared = true;
  // Flush or resolution change may be waiting image processor to finish.
  if (image_processor_bitstream_buffer_ids_.empty()) {
    NotifyFlushDoneIfNeeded();
    if (decoder_state_ == kChangingResolution)
      StartResolutionChange();
  }
}

void V4L2VideoDecodeAccelerator::ImageProcessorError() {
  VLOGF(1) << "Image processor error";
  NOTIFY_ERROR(PLATFORM_FAILURE);
}

}  // namespace media
