// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_COMMON_VIDEO_ENCODE_ACCELERATOR_STRUCT_TRAITS_H_
#define COMPONENTS_ARC_COMMON_VIDEO_ENCODE_ACCELERATOR_STRUCT_TRAITS_H_

#include "components/arc/common/video_encode_accelerator.mojom.h"
#include "media/video/video_encode_accelerator.h"

namespace mojo {

template <>
struct EnumTraits<arc::mojom::VideoEncodeAccelerator::Error,
                  media::VideoEncodeAccelerator::Error> {
  static arc::mojom::VideoEncodeAccelerator::Error ToMojom(
      media::VideoEncodeAccelerator::Error input);

  static bool FromMojom(arc::mojom::VideoEncodeAccelerator::Error input,
                        media::VideoEncodeAccelerator::Error* output);
};

template <>
struct EnumTraits<arc::mojom::VideoPixelFormat, media::VideoPixelFormat> {
  static arc::mojom::VideoPixelFormat ToMojom(media::VideoPixelFormat input);

  static bool FromMojom(arc::mojom::VideoPixelFormat input,
                        media::VideoPixelFormat* output);
};

template <>
struct StructTraits<arc::mojom::VideoEncodeProfileDataView,
                    media::VideoEncodeAccelerator::SupportedProfile> {
  static media::VideoCodecProfile profile(
      const media::VideoEncodeAccelerator::SupportedProfile& r) {
    return r.profile;
  }
  static const gfx::Size& max_resolution(
      const media::VideoEncodeAccelerator::SupportedProfile& r) {
    return r.max_resolution;
  }
  static uint32_t max_framerate_numerator(
      const media::VideoEncodeAccelerator::SupportedProfile& r) {
    return r.max_framerate_numerator;
  }
  static uint32_t max_framerate_denominator(
      const media::VideoEncodeAccelerator::SupportedProfile& r) {
    return r.max_framerate_denominator;
  }

  static bool Read(arc::mojom::VideoEncodeProfileDataView data,
                   media::VideoEncodeAccelerator::SupportedProfile* out) {
    NOTIMPLEMENTED();
    return false;
  }
};

}  // namespace mojo

#endif  // COMPONENTS_ARC_COMMON_VIDEO_ENCODE_ACCELERATOR_STRUCT_TRAITS_H_
