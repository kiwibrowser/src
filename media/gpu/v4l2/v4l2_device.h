// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines the V4L2Device interface which is used by the
// V4L2DecodeAccelerator class to delegate/pass the device specific
// handling of any of the functionalities.

#ifndef MEDIA_GPU_V4L2_V4L2_DEVICE_H_
#define MEDIA_GPU_V4L2_V4L2_DEVICE_H_

#include <stddef.h>
#include <stdint.h>

#include <linux/videodev2.h>

#include "base/files/scoped_file.h"
#include "base/memory/ref_counted.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/gpu/media_gpu_export.h"
#include "media/video/video_decode_accelerator.h"
#include "media/video/video_encode_accelerator.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_image.h"

// TODO(posciak): remove this once V4L2 headers are updated.
#define V4L2_PIX_FMT_MT21 v4l2_fourcc('M', 'T', '2', '1')
#ifndef V4L2_BUF_FLAG_LAST
#define V4L2_BUF_FLAG_LAST 0x00100000
#endif

namespace media {

class MEDIA_GPU_EXPORT V4L2Device
    : public base::RefCountedThreadSafe<V4L2Device> {
 public:
  // Utility format conversion functions
  static VideoPixelFormat V4L2PixFmtToVideoPixelFormat(uint32_t format);
  static uint32_t VideoPixelFormatToV4L2PixFmt(VideoPixelFormat format);
  static uint32_t VideoCodecProfileToV4L2PixFmt(VideoCodecProfile profile,
                                                bool slice_based);
  std::vector<VideoCodecProfile> V4L2PixFmtToVideoCodecProfiles(
      uint32_t pix_fmt,
      bool is_encoder);
  static uint32_t V4L2PixFmtToDrmFormat(uint32_t format);
  // Convert format requirements requested by a V4L2 device to gfx::Size.
  static gfx::Size CodedSizeFromV4L2Format(struct v4l2_format format);

  enum class Type {
    kDecoder,
    kEncoder,
    kImageProcessor,
    kJpegDecoder,
  };

  // Create and initialize an appropriate V4L2Device instance for the current
  // platform, or return nullptr if not available.
  static scoped_refptr<V4L2Device> Create();

  // Open a V4L2 device of |type| for use with |v4l2_pixfmt|.
  // Return true on success.
  // The device will be closed in the destructor.
  virtual bool Open(Type type, uint32_t v4l2_pixfmt) = 0;

  // Parameters and return value are the same as for the standard ioctl() system
  // call.
  virtual int Ioctl(int request, void* arg) = 0;

  // This method sleeps until either:
  // - SetDevicePollInterrupt() is called (on another thread),
  // - |poll_device| is true, and there is new data to be read from the device,
  //   or an event from the device has arrived; in the latter case
  //   |*event_pending| will be set to true.
  // Returns false on error, true otherwise.
  // This method should be called from a separate thread.
  virtual bool Poll(bool poll_device, bool* event_pending) = 0;

  // These methods are used to interrupt the thread sleeping on Poll() and force
  // it to return regardless of device state, which is usually when the client
  // is no longer interested in what happens with the device (on cleanup,
  // client state change, etc.). When SetDevicePollInterrupt() is called, Poll()
  // will return immediately, and any subsequent calls to it will also do so
  // until ClearDevicePollInterrupt() is called.
  virtual bool SetDevicePollInterrupt() = 0;
  virtual bool ClearDevicePollInterrupt() = 0;

  // Wrappers for standard mmap/munmap system calls.
  virtual void* Mmap(void* addr,
                     unsigned int len,
                     int prot,
                     int flags,
                     unsigned int offset) = 0;
  virtual void Munmap(void* addr, unsigned int len) = 0;

  // Return a vector of dmabuf file descriptors, exported for V4L2 buffer with
  // |index|, assuming the buffer contains |num_planes| V4L2 planes and is of
  // |type|. Return an empty vector on failure.
  // The caller is responsible for closing the file descriptors after use.
  virtual std::vector<base::ScopedFD> GetDmabufsForV4L2Buffer(
      int index,
      size_t num_planes,
      enum v4l2_buf_type type) = 0;

  // Return true if the given V4L2 pixfmt can be used in CreateEGLImage()
  // for the current platform.
  virtual bool CanCreateEGLImageFrom(uint32_t v4l2_pixfmt) = 0;

  // Create an EGLImage from provided |dmabuf_fds| and bind |texture_id| to it.
  // Some implementations may also require the V4L2 |buffer_index| of the buffer
  // for which |dmabuf_fds| have been exported.
  // The caller may choose to close the file descriptors after this method
  // returns, and may expect the buffers to remain valid for the lifetime of
  // the created EGLImage.
  // Return EGL_NO_IMAGE_KHR on failure.
  virtual EGLImageKHR CreateEGLImage(
      EGLDisplay egl_display,
      EGLContext egl_context,
      GLuint texture_id,
      const gfx::Size& size,
      unsigned int buffer_index,
      uint32_t v4l2_pixfmt,
      const std::vector<base::ScopedFD>& dmabuf_fds) = 0;

  // Create a GLImage from provided |dmabuf_fds|.
  // The caller may choose to close the file descriptors after this method
  // returns, and may expect the buffers to remain valid for the lifetime of
  // the created GLImage.
  // Return the newly created GLImage.
  virtual scoped_refptr<gl::GLImage> CreateGLImage(
      const gfx::Size& size,
      uint32_t fourcc,
      const std::vector<base::ScopedFD>& dmabuf_fds) = 0;

  // Destroys the EGLImageKHR.
  virtual EGLBoolean DestroyEGLImage(EGLDisplay egl_display,
                                     EGLImageKHR egl_image) = 0;

  // Returns the supported texture target for the V4L2Device.
  virtual GLenum GetTextureTarget() = 0;

  // Returns the preferred V4L2 input format for |type| or 0 if none.
  virtual uint32_t PreferredInputFormat(Type type) = 0;

  // NOTE: The below methods to query capabilities have a side effect of
  // closing the previously-open device, if any, and should not be called after
  // Open().
  // TODO(posciak): fix this.

  // Get minimum and maximum resolution for fourcc |pixelformat| and store to
  // |min_resolution| and |max_resolution|.
  void GetSupportedResolution(uint32_t pixelformat,
                              gfx::Size* min_resolution,
                              gfx::Size* max_resolution);

  // Return V4L2 pixelformats supported by the available image processor
  // devices for |buf_type|.
  virtual std::vector<uint32_t> GetSupportedImageProcessorPixelformats(
      v4l2_buf_type buf_type) = 0;

  // Return supported profiles for decoder, including only profiles for given
  // fourcc |pixelformats|.
  virtual VideoDecodeAccelerator::SupportedProfiles GetSupportedDecodeProfiles(
      const size_t num_formats,
      const uint32_t pixelformats[]) = 0;

  // Return supported profiles for encoder.
  virtual VideoEncodeAccelerator::SupportedProfiles
  GetSupportedEncodeProfiles() = 0;

  // Return true if image processing is supported, false otherwise.
  virtual bool IsImageProcessingSupported() = 0;

  // Return true if JPEG decoding is supported, false otherwise.
  virtual bool IsJpegDecodingSupported() = 0;

 protected:
  friend class base::RefCountedThreadSafe<V4L2Device>;
  V4L2Device();
  virtual ~V4L2Device();

  VideoDecodeAccelerator::SupportedProfiles EnumerateSupportedDecodeProfiles(
      const size_t num_formats,
      const uint32_t pixelformats[]);

  VideoEncodeAccelerator::SupportedProfiles EnumerateSupportedEncodeProfiles();

  std::vector<uint32_t> EnumerateSupportedPixelformats(v4l2_buf_type buf_type);

 private:
  // Perform platform-specific initialization of the device instance.
  // Return true on success, false on error or if the particular implementation
  // is not available.
  virtual bool Initialize() = 0;
};

}  //  namespace media

#endif  // MEDIA_GPU_V4L2_V4L2_DEVICE_H_
