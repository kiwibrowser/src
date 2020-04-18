// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_INTERFACES_JPEG_DECODE_ACCELERATOR_TYPEMAP_TRAITS_H_
#define MEDIA_MOJO_INTERFACES_JPEG_DECODE_ACCELERATOR_TYPEMAP_TRAITS_H_

#include "base/numerics/safe_conversions.h"
#include "media/base/bitstream_buffer.h"
#include "media/mojo/interfaces/jpeg_decode_accelerator.mojom.h"
#include "media/video/jpeg_decode_accelerator.h"

namespace mojo {

template <>
struct EnumTraits<media::mojom::DecodeError,
                  media::JpegDecodeAccelerator::Error> {
  static media::mojom::DecodeError ToMojom(
      media::JpegDecodeAccelerator::Error error);

  static bool FromMojom(media::mojom::DecodeError input,
                        media::JpegDecodeAccelerator::Error* out);
};

template <>
struct StructTraits<media::mojom::BitstreamBufferDataView,
                    media::BitstreamBuffer> {
  static int32_t id(const media::BitstreamBuffer& input) { return input.id(); }

  static mojo::ScopedSharedBufferHandle memory_handle(
      const media::BitstreamBuffer& input);

  static uint32_t size(const media::BitstreamBuffer& input) {
    return base::checked_cast<uint32_t>(input.size());
  }

  static int64_t offset(const media::BitstreamBuffer& input) {
    return base::checked_cast<int64_t>(input.offset());
  }

  static base::TimeDelta timestamp(const media::BitstreamBuffer& input) {
    return input.presentation_timestamp();
  }

  static const std::string& key_id(const media::BitstreamBuffer& input) {
    return input.key_id();
  }

  static const std::string& iv(const media::BitstreamBuffer& input) {
    return input.iv();
  }

  static const std::vector<media::SubsampleEntry>& subsamples(
      const media::BitstreamBuffer& input) {
    return input.subsamples();
  }

  static bool Read(media::mojom::BitstreamBufferDataView input,
                   media::BitstreamBuffer* output);
};

}  // namespace mojo

#endif  // MEDIA_MOJO_INTERFACES_JPEG_DECODE_ACCELERATOR_TYPEMAP_TRAITS_H_
