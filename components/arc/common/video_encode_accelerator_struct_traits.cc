// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/common/video_encode_accelerator_struct_traits.h"

namespace mojo {

// Make sure values in arc::mojom::VideoEncodeAccelerator::Error and
// media::VideoEncodeAccelerator::Error match.
#define CHECK_ERROR_ENUM(value)                                             \
  static_assert(                                                            \
      static_cast<int>(arc::mojom::VideoEncodeAccelerator::Error::value) == \
          media::VideoEncodeAccelerator::Error::value,                      \
      "enum ##value mismatch")

CHECK_ERROR_ENUM(kIllegalStateError);
CHECK_ERROR_ENUM(kInvalidArgumentError);
CHECK_ERROR_ENUM(kPlatformFailureError);
CHECK_ERROR_ENUM(kErrorMax);

#undef CHECK_ERROR_ENUM

// static
arc::mojom::VideoEncodeAccelerator::Error
EnumTraits<arc::mojom::VideoEncodeAccelerator::Error,
           media::VideoEncodeAccelerator::Error>::
    ToMojom(media::VideoEncodeAccelerator::Error input) {
  return static_cast<arc::mojom::VideoEncodeAccelerator::Error>(input);
}

// static
bool EnumTraits<arc::mojom::VideoEncodeAccelerator::Error,
                media::VideoEncodeAccelerator::Error>::
    FromMojom(arc::mojom::VideoEncodeAccelerator::Error input,
              media::VideoEncodeAccelerator::Error* output) {
  NOTIMPLEMENTED();
  return false;
}

// Make sure values in arc::mojom::VideoPixelFormat match to the values in
// media::VideoPixelFormat. The former is a subset of the later.
#define CHECK_PIXEL_FORMAT_ENUM(value)                                       \
  static_assert(                                                             \
      static_cast<int>(arc::mojom::VideoPixelFormat::value) == media::value, \
      "enum ##value mismatch")

CHECK_PIXEL_FORMAT_ENUM(PIXEL_FORMAT_I420);

#undef CHECK_PXIEL_FORMAT_ENUM

// static
arc::mojom::VideoPixelFormat
EnumTraits<arc::mojom::VideoPixelFormat, media::VideoPixelFormat>::ToMojom(
    media::VideoPixelFormat input) {
  NOTIMPLEMENTED();
  return arc::mojom::VideoPixelFormat::PIXEL_FORMAT_I420;
}

// static
bool EnumTraits<arc::mojom::VideoPixelFormat, media::VideoPixelFormat>::
    FromMojom(arc::mojom::VideoPixelFormat input,
              media::VideoPixelFormat* output) {
  switch (input) {
    case arc::mojom::VideoPixelFormat::PIXEL_FORMAT_I420:
      *output = static_cast<media::VideoPixelFormat>(input);
      return true;
    default:
      DLOG(ERROR) << "Unknown VideoPixelFormat: " << input;
      return false;
  }
}

}  // namespace mojo
