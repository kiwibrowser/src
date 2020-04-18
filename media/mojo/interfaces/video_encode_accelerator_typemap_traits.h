// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_INTERFACES_VIDEO_ENCODE_ACCELERATOR_TYPEMAP_TRAITS_H_
#define MEDIA_MOJO_INTERFACES_VIDEO_ENCODE_ACCELERATOR_TYPEMAP_TRAITS_H_

#include "media/mojo/interfaces/video_encode_accelerator.mojom.h"
#include "media/video/video_encode_accelerator.h"

namespace mojo {

template <>
struct EnumTraits<media::mojom::VideoEncodeAccelerator::Error,
                  media::VideoEncodeAccelerator::Error> {
  static media::mojom::VideoEncodeAccelerator::Error ToMojom(
      media::VideoEncodeAccelerator::Error error);

  static bool FromMojom(media::mojom::VideoEncodeAccelerator::Error input,
                        media::VideoEncodeAccelerator::Error* out);
};

}  // namespace mojo

#endif  // MEDIA_MOJO_INTERFACES_VIDEO_ENCODE_ACCELERATOR_TYPEMAP_TRAITS_H_
