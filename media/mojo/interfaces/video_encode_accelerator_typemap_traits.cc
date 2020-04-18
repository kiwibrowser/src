// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/interfaces/video_encode_accelerator_typemap_traits.h"

#include "base/logging.h"

namespace mojo {

// static
media::mojom::VideoEncodeAccelerator::Error
EnumTraits<media::mojom::VideoEncodeAccelerator::Error,
           media::VideoEncodeAccelerator::Error>::
    ToMojom(media::VideoEncodeAccelerator::Error error) {
  switch (error) {
    case media::VideoEncodeAccelerator::kIllegalStateError:
      return media::mojom::VideoEncodeAccelerator::Error::ILLEGAL_STATE;
    case media::VideoEncodeAccelerator::kInvalidArgumentError:
      return media::mojom::VideoEncodeAccelerator::Error::INVALID_ARGUMENT;
    case media::VideoEncodeAccelerator::kPlatformFailureError:
      return media::mojom::VideoEncodeAccelerator::Error::PLATFORM_FAILURE;
  }
  NOTREACHED();
  return media::mojom::VideoEncodeAccelerator::Error::INVALID_ARGUMENT;
}

// static
bool EnumTraits<media::mojom::VideoEncodeAccelerator::Error,
                media::VideoEncodeAccelerator::Error>::
    FromMojom(media::mojom::VideoEncodeAccelerator::Error error,
              media::VideoEncodeAccelerator::Error* out) {
  switch (error) {
    case media::mojom::VideoEncodeAccelerator::Error::ILLEGAL_STATE:
      *out = media::VideoEncodeAccelerator::kIllegalStateError;
      return true;
    case media::mojom::VideoEncodeAccelerator::Error::INVALID_ARGUMENT:
      *out = media::VideoEncodeAccelerator::kInvalidArgumentError;
      return true;
    case media::mojom::VideoEncodeAccelerator::Error::PLATFORM_FAILURE:
      *out = media::VideoEncodeAccelerator::kPlatformFailureError;
      return true;
  }
  NOTREACHED();
  return false;
}

}  // namespace mojo
