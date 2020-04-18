// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/interfaces/jpeg_encode_accelerator_typemap_traits.h"

#include "base/logging.h"

namespace mojo {

// static
media::mojom::EncodeStatus
EnumTraits<media::mojom::EncodeStatus, media::JpegEncodeAccelerator::Status>::
    ToMojom(media::JpegEncodeAccelerator::Status status) {
  switch (status) {
    case media::JpegEncodeAccelerator::ENCODE_OK:
      return media::mojom::EncodeStatus::ENCODE_OK;
    case media::JpegEncodeAccelerator::HW_JPEG_ENCODE_NOT_SUPPORTED:
      return media::mojom::EncodeStatus::HW_JPEG_ENCODE_NOT_SUPPORTED;
    case media::JpegEncodeAccelerator::THREAD_CREATION_FAILED:
      return media::mojom::EncodeStatus::THREAD_CREATION_FAILED;
    case media::JpegEncodeAccelerator::INVALID_ARGUMENT:
      return media::mojom::EncodeStatus::INVALID_ARGUMENT;
    case media::JpegEncodeAccelerator::INACCESSIBLE_OUTPUT_BUFFER:
      return media::mojom::EncodeStatus::INACCESSIBLE_OUTPUT_BUFFER;
    case media::JpegEncodeAccelerator::PARSE_IMAGE_FAILED:
      return media::mojom::EncodeStatus::PARSE_IMAGE_FAILED;
    case media::JpegEncodeAccelerator::PLATFORM_FAILURE:
      return media::mojom::EncodeStatus::PLATFORM_FAILURE;
  }
  NOTREACHED();
  return media::mojom::EncodeStatus::ENCODE_OK;
}

// static
bool EnumTraits<media::mojom::EncodeStatus,
                media::JpegEncodeAccelerator::Status>::
    FromMojom(media::mojom::EncodeStatus status,
              media::JpegEncodeAccelerator::Status* out) {
  switch (status) {
    case media::mojom::EncodeStatus::ENCODE_OK:
      *out = media::JpegEncodeAccelerator::Status::ENCODE_OK;
      return true;
    case media::mojom::EncodeStatus::HW_JPEG_ENCODE_NOT_SUPPORTED:
      *out = media::JpegEncodeAccelerator::Status::HW_JPEG_ENCODE_NOT_SUPPORTED;
      return true;
    case media::mojom::EncodeStatus::THREAD_CREATION_FAILED:
      *out = media::JpegEncodeAccelerator::Status::THREAD_CREATION_FAILED;
      return true;
    case media::mojom::EncodeStatus::INVALID_ARGUMENT:
      *out = media::JpegEncodeAccelerator::Status::INVALID_ARGUMENT;
      return true;
    case media::mojom::EncodeStatus::INACCESSIBLE_OUTPUT_BUFFER:
      *out = media::JpegEncodeAccelerator::Status::INACCESSIBLE_OUTPUT_BUFFER;
      return true;
    case media::mojom::EncodeStatus::PARSE_IMAGE_FAILED:
      *out = media::JpegEncodeAccelerator::Status::PARSE_IMAGE_FAILED;
      return true;
    case media::mojom::EncodeStatus::PLATFORM_FAILURE:
      *out = media::JpegEncodeAccelerator::Status::PLATFORM_FAILURE;
      return true;
  }
  NOTREACHED();
  return false;
}

}  // namespace mojo
