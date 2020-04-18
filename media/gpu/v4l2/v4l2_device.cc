// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <libdrm/drm_fourcc.h>
#include <linux/videodev2.h>
#include <string.h>

#include "base/numerics/safe_conversions.h"
#include "build/build_config.h"
#include "media/gpu/v4l2/generic_v4l2_device.h"
#if defined(ARCH_CPU_ARMEL)
#include "media/gpu/v4l2/tegra_v4l2_device.h"
#endif

#define DVLOGF(level) DVLOG(level) << __func__ << "(): "
#define VLOGF(level) VLOG(level) << __func__ << "(): "

namespace media {

V4L2Device::V4L2Device() {}

V4L2Device::~V4L2Device() {}

// static
scoped_refptr<V4L2Device> V4L2Device::Create() {
  VLOGF(2);

  scoped_refptr<V4L2Device> device;

#if defined(ARCH_CPU_ARMEL)
  device = new TegraV4L2Device();
  if (device->Initialize())
    return device;
#endif

  device = new GenericV4L2Device();
  if (device->Initialize())
    return device;

  VLOGF(1) << "Failed to create a V4L2Device";
  return nullptr;
}

// static
VideoPixelFormat V4L2Device::V4L2PixFmtToVideoPixelFormat(uint32_t pix_fmt) {
  switch (pix_fmt) {
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV12M:
      return PIXEL_FORMAT_NV12;

    case V4L2_PIX_FMT_MT21:
      return PIXEL_FORMAT_MT21;

    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YUV420M:
      return PIXEL_FORMAT_I420;

    case V4L2_PIX_FMT_YVU420:
      return PIXEL_FORMAT_YV12;

    case V4L2_PIX_FMT_YUV422M:
      return PIXEL_FORMAT_I422;

    case V4L2_PIX_FMT_RGB32:
      return PIXEL_FORMAT_ARGB;

    default:
      DVLOGF(1) << "Add more cases as needed";
      return PIXEL_FORMAT_UNKNOWN;
  }
}

// static
uint32_t V4L2Device::VideoPixelFormatToV4L2PixFmt(VideoPixelFormat format) {
  switch (format) {
    case PIXEL_FORMAT_NV12:
      return V4L2_PIX_FMT_NV12M;

    case PIXEL_FORMAT_MT21:
      return V4L2_PIX_FMT_MT21;

    case PIXEL_FORMAT_I420:
      return V4L2_PIX_FMT_YUV420M;

    case PIXEL_FORMAT_YV12:
      return V4L2_PIX_FMT_YVU420;

    default:
      LOG(FATAL) << "Add more cases as needed";
      return 0;
  }
}

// static
uint32_t V4L2Device::VideoCodecProfileToV4L2PixFmt(VideoCodecProfile profile,
                                                   bool slice_based) {
  if (profile >= H264PROFILE_MIN && profile <= H264PROFILE_MAX) {
    if (slice_based)
      return V4L2_PIX_FMT_H264_SLICE;
    else
      return V4L2_PIX_FMT_H264;
  } else if (profile >= VP8PROFILE_MIN && profile <= VP8PROFILE_MAX) {
    if (slice_based)
      return V4L2_PIX_FMT_VP8_FRAME;
    else
      return V4L2_PIX_FMT_VP8;
  } else if (profile >= VP9PROFILE_MIN && profile <= VP9PROFILE_MAX) {
    if (slice_based)
      return V4L2_PIX_FMT_VP9_FRAME;
    else
      return V4L2_PIX_FMT_VP9;
  } else {
    LOG(FATAL) << "Add more cases as needed";
    return 0;
  }
}

// static
std::vector<VideoCodecProfile> V4L2Device::V4L2PixFmtToVideoCodecProfiles(
    uint32_t pix_fmt,
    bool is_encoder) {
  VideoCodecProfile min_profile, max_profile;
  std::vector<VideoCodecProfile> profiles;

  switch (pix_fmt) {
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H264_SLICE:
      if (is_encoder) {
        // TODO(posciak): need to query the device for supported H.264 profiles,
        // for now choose Main as a sensible default.
        min_profile = H264PROFILE_MAIN;
        max_profile = H264PROFILE_MAIN;
      } else {
        min_profile = H264PROFILE_MIN;
        max_profile = H264PROFILE_MAX;
      }
      break;

    case V4L2_PIX_FMT_VP8:
    case V4L2_PIX_FMT_VP8_FRAME:
      min_profile = VP8PROFILE_MIN;
      max_profile = VP8PROFILE_MAX;
      break;

    case V4L2_PIX_FMT_VP9:
    case V4L2_PIX_FMT_VP9_FRAME:
      // TODO(posciak): https://crbug.com/819930 Query supported profiles.
      // Currently no devices support Profiles > 0 https://crbug.com/796297.
      min_profile = VP9PROFILE_PROFILE0;
      max_profile = VP9PROFILE_PROFILE0;
      break;

    default:
      VLOGF(1) << "Unhandled pixelformat " << std::hex << "0x" << pix_fmt;
      return profiles;
  }

  for (int profile = min_profile; profile <= max_profile; ++profile)
    profiles.push_back(static_cast<VideoCodecProfile>(profile));

  return profiles;
}

// static
uint32_t V4L2Device::V4L2PixFmtToDrmFormat(uint32_t format) {
  switch (format) {
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV12M:
      return DRM_FORMAT_NV12;

    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YUV420M:
      return DRM_FORMAT_YUV420;

    case V4L2_PIX_FMT_YVU420:
      return DRM_FORMAT_YVU420;

    case V4L2_PIX_FMT_RGB32:
      return DRM_FORMAT_ARGB8888;

    case V4L2_PIX_FMT_MT21:
      return DRM_FORMAT_MT21;

    default:
      DVLOGF(1) << "Unrecognized format " << std::hex << "0x" << format;
      return 0;
  }
}

// static
gfx::Size V4L2Device::CodedSizeFromV4L2Format(struct v4l2_format format) {
  gfx::Size coded_size;
  gfx::Size visible_size;
  VideoPixelFormat frame_format = PIXEL_FORMAT_UNKNOWN;
  size_t bytesperline = 0;
  // Total bytes in the frame.
  size_t sizeimage = 0;

  if (V4L2_TYPE_IS_MULTIPLANAR(format.type)) {
    DCHECK_GT(format.fmt.pix_mp.num_planes, 0);
    bytesperline =
        base::checked_cast<int>(format.fmt.pix_mp.plane_fmt[0].bytesperline);
    for (size_t i = 0; i < format.fmt.pix_mp.num_planes; ++i) {
      sizeimage +=
          base::checked_cast<int>(format.fmt.pix_mp.plane_fmt[i].sizeimage);
    }
    visible_size.SetSize(base::checked_cast<int>(format.fmt.pix_mp.width),
                         base::checked_cast<int>(format.fmt.pix_mp.height));
    frame_format =
        V4L2Device::V4L2PixFmtToVideoPixelFormat(format.fmt.pix_mp.pixelformat);
  } else {
    bytesperline = base::checked_cast<int>(format.fmt.pix.bytesperline);
    sizeimage = base::checked_cast<int>(format.fmt.pix.sizeimage);
    visible_size.SetSize(base::checked_cast<int>(format.fmt.pix.width),
                         base::checked_cast<int>(format.fmt.pix.height));
    frame_format =
        V4L2Device::V4L2PixFmtToVideoPixelFormat(format.fmt.pix.pixelformat);
  }

  // V4L2 does not provide per-plane bytesperline (bpl) when different
  // components are sharing one physical plane buffer. In this case, it only
  // provides bpl for the first component in the plane. So we can't depend on it
  // for calculating height, because bpl may vary within one physical plane
  // buffer. For example, YUV420 contains 3 components in one physical plane,
  // with Y at 8 bits per pixel, and Cb/Cr at 4 bits per pixel per component,
  // but we only get 8 pits per pixel from bytesperline in physical plane 0.
  // So we need to get total frame bpp from elsewhere to calculate coded height.

  // We need bits per pixel for one component only to calculate
  // coded_width from bytesperline.
  int plane_horiz_bits_per_pixel =
      VideoFrame::PlaneHorizontalBitsPerPixel(frame_format, 0);

  // Adding up bpp for each component will give us total bpp for all components.
  int total_bpp = 0;
  for (size_t i = 0; i < VideoFrame::NumPlanes(frame_format); ++i)
    total_bpp += VideoFrame::PlaneBitsPerPixel(frame_format, i);

  if (sizeimage == 0 || bytesperline == 0 || plane_horiz_bits_per_pixel == 0 ||
      total_bpp == 0 || (bytesperline * 8) % plane_horiz_bits_per_pixel != 0) {
    VLOGF(1) << "Invalid format provided";
    return coded_size;
  }

  // Coded width can be calculated by taking the first component's bytesperline,
  // which in V4L2 always applies to the first component in physical plane
  // buffer.
  int coded_width = bytesperline * 8 / plane_horiz_bits_per_pixel;
  // Sizeimage is coded_width * coded_height * total_bpp.
  int coded_height = sizeimage * 8 / coded_width / total_bpp;

  coded_size.SetSize(coded_width, coded_height);
  // It's possible the driver gave us a slightly larger sizeimage than what
  // would be calculated from coded size. This is technically not allowed, but
  // some drivers (Exynos) like to have some additional alignment that is not a
  // multiple of bytesperline. The best thing we can do is to compensate by
  // aligning to next full row.
  if (sizeimage > VideoFrame::AllocationSize(frame_format, coded_size))
    coded_size.SetSize(coded_width, coded_height + 1);
  DVLOGF(3) << "coded_size=" << coded_size.ToString();

  // Sanity checks. Calculated coded size has to contain given visible size
  // and fulfill buffer byte size requirements.
  DCHECK(gfx::Rect(coded_size).Contains(gfx::Rect(visible_size)));
  DCHECK_LE(sizeimage, VideoFrame::AllocationSize(frame_format, coded_size));

  return coded_size;
}

void V4L2Device::GetSupportedResolution(uint32_t pixelformat,
                                        gfx::Size* min_resolution,
                                        gfx::Size* max_resolution) {
  max_resolution->SetSize(0, 0);
  min_resolution->SetSize(0, 0);
  v4l2_frmsizeenum frame_size;
  memset(&frame_size, 0, sizeof(frame_size));
  frame_size.pixel_format = pixelformat;
  for (; Ioctl(VIDIOC_ENUM_FRAMESIZES, &frame_size) == 0; ++frame_size.index) {
    if (frame_size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
      if (frame_size.discrete.width >=
              base::checked_cast<uint32_t>(max_resolution->width()) &&
          frame_size.discrete.height >=
              base::checked_cast<uint32_t>(max_resolution->height())) {
        max_resolution->SetSize(frame_size.discrete.width,
                                frame_size.discrete.height);
      }
      if (min_resolution->IsEmpty() ||
          (frame_size.discrete.width <=
               base::checked_cast<uint32_t>(min_resolution->width()) &&
           frame_size.discrete.height <=
               base::checked_cast<uint32_t>(min_resolution->height()))) {
        min_resolution->SetSize(frame_size.discrete.width,
                                frame_size.discrete.height);
      }
    } else if (frame_size.type == V4L2_FRMSIZE_TYPE_STEPWISE ||
               frame_size.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
      max_resolution->SetSize(frame_size.stepwise.max_width,
                              frame_size.stepwise.max_height);
      min_resolution->SetSize(frame_size.stepwise.min_width,
                              frame_size.stepwise.min_height);
      break;
    }
  }
  if (max_resolution->IsEmpty()) {
    max_resolution->SetSize(1920, 1088);
    VLOGF(1) << "GetSupportedResolution failed to get maximum resolution for "
             << "fourcc " << std::hex << pixelformat << ", fall back to "
             << max_resolution->ToString();
  }
  if (min_resolution->IsEmpty()) {
    min_resolution->SetSize(16, 16);
    VLOGF(1) << "GetSupportedResolution failed to get minimum resolution for "
             << "fourcc " << std::hex << pixelformat << ", fall back to "
             << min_resolution->ToString();
  }
}

std::vector<uint32_t> V4L2Device::EnumerateSupportedPixelformats(
    v4l2_buf_type buf_type) {
  std::vector<uint32_t> pixelformats;

  v4l2_fmtdesc fmtdesc;
  memset(&fmtdesc, 0, sizeof(fmtdesc));
  fmtdesc.type = buf_type;

  for (; Ioctl(VIDIOC_ENUM_FMT, &fmtdesc) == 0; ++fmtdesc.index) {
    DVLOGF(3) << "Found " << fmtdesc.description << std::hex << " (0x"
              << fmtdesc.pixelformat << ")";
    pixelformats.push_back(fmtdesc.pixelformat);
  }

  return pixelformats;
}

VideoDecodeAccelerator::SupportedProfiles
V4L2Device::EnumerateSupportedDecodeProfiles(const size_t num_formats,
                                             const uint32_t pixelformats[]) {
  VideoDecodeAccelerator::SupportedProfiles profiles;

  const auto& supported_pixelformats =
      EnumerateSupportedPixelformats(V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);

  for (uint32_t pixelformat : supported_pixelformats) {
    if (std::find(pixelformats, pixelformats + num_formats, pixelformat) ==
        pixelformats + num_formats)
      continue;

    VideoDecodeAccelerator::SupportedProfile profile;
    GetSupportedResolution(pixelformat, &profile.min_resolution,
                           &profile.max_resolution);

    const auto video_codec_profiles =
        V4L2PixFmtToVideoCodecProfiles(pixelformat, false);

    for (const auto& video_codec_profile : video_codec_profiles) {
      profile.profile = video_codec_profile;
      profiles.push_back(profile);

      DVLOGF(3) << "Found decoder profile " << GetProfileName(profile.profile)
                << ", resolutions: " << profile.min_resolution.ToString() << " "
                << profile.max_resolution.ToString();
    }
  }

  return profiles;
}

VideoEncodeAccelerator::SupportedProfiles
V4L2Device::EnumerateSupportedEncodeProfiles() {
  VideoEncodeAccelerator::SupportedProfiles profiles;

  const auto& supported_pixelformats =
      EnumerateSupportedPixelformats(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

  for (const auto& pixelformat : supported_pixelformats) {
    VideoEncodeAccelerator::SupportedProfile profile;
    profile.max_framerate_numerator = 30;
    profile.max_framerate_denominator = 1;
    gfx::Size min_resolution;
    GetSupportedResolution(pixelformat, &min_resolution,
                           &profile.max_resolution);

    const auto video_codec_profiles =
        V4L2PixFmtToVideoCodecProfiles(pixelformat, true);

    for (const auto& video_codec_profile : video_codec_profiles) {
      profile.profile = video_codec_profile;
      profiles.push_back(profile);

      DVLOGF(3) << "Found encoder profile " << GetProfileName(profile.profile)
                << ", max resolution: " << profile.max_resolution.ToString();
    }
  }

  return profiles;
}

}  //  namespace media
