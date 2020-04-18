// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/interfaces/jpeg_decode_accelerator_typemap_traits.h"

#include "base/logging.h"
#include "media/base/ipc/media_param_traits_macros.h"
#include "mojo/public/cpp/base/time_mojom_traits.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace mojo {

// static
media::mojom::DecodeError
EnumTraits<media::mojom::DecodeError, media::JpegDecodeAccelerator::Error>::
    ToMojom(media::JpegDecodeAccelerator::Error error) {
  switch (error) {
    case media::JpegDecodeAccelerator::NO_ERRORS:
      return media::mojom::DecodeError::NO_ERRORS;
    case media::JpegDecodeAccelerator::INVALID_ARGUMENT:
      return media::mojom::DecodeError::INVALID_ARGUMENT;
    case media::JpegDecodeAccelerator::UNREADABLE_INPUT:
      return media::mojom::DecodeError::UNREADABLE_INPUT;
    case media::JpegDecodeAccelerator::PARSE_JPEG_FAILED:
      return media::mojom::DecodeError::PARSE_JPEG_FAILED;
    case media::JpegDecodeAccelerator::UNSUPPORTED_JPEG:
      return media::mojom::DecodeError::UNSUPPORTED_JPEG;
    case media::JpegDecodeAccelerator::PLATFORM_FAILURE:
      return media::mojom::DecodeError::PLATFORM_FAILURE;
  }
  NOTREACHED();
  return media::mojom::DecodeError::NO_ERRORS;
}

// static
bool EnumTraits<media::mojom::DecodeError,
                media::JpegDecodeAccelerator::Error>::
    FromMojom(media::mojom::DecodeError error,
              media::JpegDecodeAccelerator::Error* out) {
  switch (error) {
    case media::mojom::DecodeError::NO_ERRORS:
      *out = media::JpegDecodeAccelerator::Error::NO_ERRORS;
      return true;
    case media::mojom::DecodeError::INVALID_ARGUMENT:
      *out = media::JpegDecodeAccelerator::Error::INVALID_ARGUMENT;
      return true;
    case media::mojom::DecodeError::UNREADABLE_INPUT:
      *out = media::JpegDecodeAccelerator::Error::UNREADABLE_INPUT;
      return true;
    case media::mojom::DecodeError::PARSE_JPEG_FAILED:
      *out = media::JpegDecodeAccelerator::Error::PARSE_JPEG_FAILED;
      return true;
    case media::mojom::DecodeError::UNSUPPORTED_JPEG:
      *out = media::JpegDecodeAccelerator::Error::UNSUPPORTED_JPEG;
      return true;
    case media::mojom::DecodeError::PLATFORM_FAILURE:
      *out = media::JpegDecodeAccelerator::Error::PLATFORM_FAILURE;
      return true;
  }
  NOTREACHED();
  return false;
}

// static
mojo::ScopedSharedBufferHandle
StructTraits<media::mojom::BitstreamBufferDataView, media::BitstreamBuffer>::
    memory_handle(const media::BitstreamBuffer& input) {
  base::SharedMemoryHandle input_handle =
      base::SharedMemory::DuplicateHandle(input.handle());
  if (!base::SharedMemory::IsHandleValid(input_handle)) {
    DLOG(ERROR) << "Failed to duplicate handle of BitstreamBuffer";
    return mojo::ScopedSharedBufferHandle();
  }

  // TODO(https://crbug.com/793446): Update this to |kReadOnly| protection once
  // BitstreamBuffer can guarantee that its handle() field always corresponds to
  // a read-only SharedMemoryHandle.
  return mojo::WrapSharedMemoryHandle(
      input_handle, input.size(),
      mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
}

// static
bool StructTraits<
    media::mojom::BitstreamBufferDataView,
    media::BitstreamBuffer>::Read(media::mojom::BitstreamBufferDataView input,
                                  media::BitstreamBuffer* output) {
  base::TimeDelta timestamp;
  if (!input.ReadTimestamp(&timestamp))
    return false;

  std::string key_id;
  if (!input.ReadKeyId(&key_id))
    return false;

  std::string iv;
  if (!input.ReadIv(&iv))
    return false;

  std::vector<media::SubsampleEntry> subsamples;
  if (!input.ReadSubsamples(&subsamples))
    return false;

  mojo::ScopedSharedBufferHandle handle = input.TakeMemoryHandle();
  if (!handle.is_valid())
    return false;

  base::SharedMemoryHandle memory_handle;
  MojoResult unwrap_result = mojo::UnwrapSharedMemoryHandle(
      std::move(handle), &memory_handle, nullptr, nullptr);
  if (unwrap_result != MOJO_RESULT_OK)
    return false;

  media::BitstreamBuffer bitstream_buffer(
      input.id(), memory_handle, input.size(),
      base::checked_cast<off_t>(input.offset()), timestamp);
  if (key_id.size()) {
    // Note that BitstreamBuffer currently ignores how each buffer is
    // encrypted and uses the settings from the Audio/VideoDecoderConfig.
    bitstream_buffer.SetDecryptionSettings(key_id, iv, subsamples);
  }
  *output = bitstream_buffer;

  return true;
}

}  // namespace mojo
