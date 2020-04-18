// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/bitstream_buffer.h"

#include "media/base/decrypt_config.h"

namespace media {

BitstreamBuffer::BitstreamBuffer()
    : BitstreamBuffer(-1, base::SharedMemoryHandle(), 0) {}

BitstreamBuffer::BitstreamBuffer(int32_t id,
                                 base::SharedMemoryHandle handle,
                                 size_t size,
                                 off_t offset,
                                 base::TimeDelta presentation_timestamp)
    : id_(id),
      handle_(handle),
      size_(size),
      offset_(offset),
      presentation_timestamp_(presentation_timestamp) {}

BitstreamBuffer::BitstreamBuffer(const BitstreamBuffer& other) = default;

BitstreamBuffer::~BitstreamBuffer() = default;

scoped_refptr<DecoderBuffer> BitstreamBuffer::ToDecoderBuffer() const {
  scoped_refptr<DecoderBuffer> buffer =
      DecoderBuffer::FromSharedMemoryHandle(handle_, offset_, size_);
  if (!buffer)
    return nullptr;
  buffer->set_timestamp(presentation_timestamp_);
  if (!key_id_.empty()) {
    buffer->set_decrypt_config(
        DecryptConfig::CreateCencConfig(key_id_, iv_, subsamples_));
  }
  return buffer;
}

void BitstreamBuffer::SetDecryptionSettings(
    const std::string& key_id,
    const std::string& iv,
    const std::vector<SubsampleEntry>& subsamples) {
  key_id_ = key_id;
  iv_ = iv;
  subsamples_ = subsamples;
}

}  // namespace media
