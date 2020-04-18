// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_INTERFACES_JPEG_ENCODE_ACCELERATOR_TYPEMAP_TRAITS_H_
#define MEDIA_MOJO_INTERFACES_JPEG_ENCODE_ACCELERATOR_TYPEMAP_TRAITS_H_

#include "media/mojo/interfaces/jpeg_encode_accelerator.mojom.h"
#include "media/video/jpeg_encode_accelerator.h"

namespace mojo {

template <>
struct EnumTraits<media::mojom::EncodeStatus,
                  media::JpegEncodeAccelerator::Status> {
  static media::mojom::EncodeStatus ToMojom(
      media::JpegEncodeAccelerator::Status status);

  static bool FromMojom(media::mojom::EncodeStatus input,
                        media::JpegEncodeAccelerator::Status* out);
};

}  // namespace mojo

#endif  // MEDIA_MOJO_INTERFACES_JPEG_ENCODE_ACCELERATOR_TYPEMAP_TRAITS_H_
