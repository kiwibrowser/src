/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/shared_buffer.h"

#include "third_party/blink/renderer/platform/instrumentation/tracing/web_process_memory_dump.h"
#include "third_party/blink/renderer/platform/wtf/text/unicode.h"
#include "third_party/blink/renderer/platform/wtf/text/utf8.h"
#include "third_party/skia/include/core/SkData.h"

namespace blink {

static inline size_t SegmentIndex(size_t position) {
  return position / SharedBuffer::kSegmentSize;
}

static inline size_t OffsetInSegment(size_t position) {
  return position % SharedBuffer::kSegmentSize;
}

static inline char* AllocateSegment() {
  return static_cast<char*>(WTF::Partitions::FastMalloc(
      SharedBuffer::kSegmentSize, "blink::SharedBuffer"));
}

static inline void FreeSegment(char* p) {
  WTF::Partitions::FastFree(p);
}

SharedBuffer::SharedBuffer() : size_(0) {}

SharedBuffer::SharedBuffer(size_t size) : size_(size), buffer_(size) {}

SharedBuffer::SharedBuffer(const char* data, size_t size) : size_(0) {
  AppendInternal(data, size);
}

SharedBuffer::SharedBuffer(const unsigned char* data, size_t size) : size_(0) {
  AppendInternal(reinterpret_cast<const char*>(data), size);
}

SharedBuffer::~SharedBuffer() {
  Clear();
}

scoped_refptr<SharedBuffer> SharedBuffer::AdoptVector(Vector<char>& vector) {
  scoped_refptr<SharedBuffer> buffer = Create();
  buffer->buffer_.swap(vector);
  buffer->size_ = buffer->buffer_.size();
  return buffer;
}

size_t SharedBuffer::size() const {
  return size_;
}

const char* SharedBuffer::Data() const {
  MergeSegmentsIntoBuffer();
  return buffer_.data();
}

void SharedBuffer::Append(const SharedBuffer& data) {
  const char* segment;
  size_t position = 0;
  while (size_t length = data.GetSomeDataInternal(segment, position)) {
    Append(segment, length);
    position += length;
  }
}

void SharedBuffer::AppendInternal(const char* data, size_t length) {
  if (!length)
    return;

  DCHECK_GE(size_, buffer_.size());
  size_t position_in_segment = OffsetInSegment(size_ - buffer_.size());
  size_ += length;

  if (size_ <= kSegmentSize) {
    // No need to use segments for small resource data.
    buffer_.Append(data, length);
    return;
  }

  char* segment;
  if (!position_in_segment) {
    segment = AllocateSegment();
    segments_.push_back(segment);
  } else
    segment = segments_.back() + position_in_segment;

  size_t segment_free_space = kSegmentSize - position_in_segment;
  size_t bytes_to_copy = std::min(length, segment_free_space);

  for (;;) {
    memcpy(segment, data, bytes_to_copy);
    if (length == bytes_to_copy)
      break;

    length -= bytes_to_copy;
    data += bytes_to_copy;
    segment = AllocateSegment();
    segments_.push_back(segment);
    bytes_to_copy = std::min(length, static_cast<size_t>(kSegmentSize));
  }
}

void SharedBuffer::Append(const Vector<char>& data) {
  Append(data.data(), data.size());
}

void SharedBuffer::Clear() {
  for (size_t i = 0; i < segments_.size(); ++i)
    FreeSegment(segments_[i]);

  segments_.clear();
  size_ = 0;
  buffer_.clear();
}

Vector<char> SharedBuffer::Copy() const {
  Vector<char> buffer;
  buffer.ReserveInitialCapacity(size_);

  ForEachSegment([&buffer](const char* segment, size_t segment_size,
                           size_t segment_offset) -> bool {
    buffer.Append(segment, segment_size);
    return true;
  });

  DCHECK_EQ(buffer.size(), size_);
  return buffer;
}

void SharedBuffer::MergeSegmentsIntoBuffer() const {
  size_t buffer_size = buffer_.size();
  if (size_ > buffer_size) {
    size_t bytes_left = size_ - buffer_size;
    for (size_t i = 0; i < segments_.size(); ++i) {
      size_t bytes_to_copy =
          std::min(bytes_left, static_cast<size_t>(kSegmentSize));
      buffer_.Append(segments_[i], bytes_to_copy);
      bytes_left -= bytes_to_copy;
      FreeSegment(segments_[i]);
    }
    segments_.clear();
  }
}

size_t SharedBuffer::GetSomeDataInternal(const char*& some_data,
                                         size_t position) const {
  size_t total_size = size();
  if (position >= total_size) {
    some_data = nullptr;
    return 0;
  }

  SECURITY_DCHECK(position < size_);
  size_t consecutive_size = buffer_.size();
  if (position < consecutive_size) {
    some_data = buffer_.data() + position;
    return consecutive_size - position;
  }

  position -= consecutive_size;
  size_t segments = segments_.size();
  size_t max_segmented_size = segments * kSegmentSize;
  size_t segment = SegmentIndex(position);
  if (segment < segments) {
    size_t bytes_left = total_size - consecutive_size;
    size_t segmented_size = std::min(max_segmented_size, bytes_left);

    size_t position_in_segment = OffsetInSegment(position);
    some_data = segments_[segment] + position_in_segment;
    return segment == segments - 1 ? segmented_size - position
                                   : kSegmentSize - position_in_segment;
  }
  NOTREACHED();
  return 0;
}

bool SharedBuffer::GetBytesInternal(void* dest, size_t byte_length) const {
  if (!dest)
    return false;

  const char* segment = nullptr;
  size_t load_position = 0;
  size_t write_position = 0;
  while (byte_length > 0) {
    size_t load_size = GetSomeDataInternal(segment, load_position);
    if (load_size == 0)
      break;

    if (byte_length < load_size)
      load_size = byte_length;
    memcpy(static_cast<char*>(dest) + write_position, segment, load_size);
    load_position += load_size;
    write_position += load_size;
    byte_length -= load_size;
  }

  return byte_length == 0;
}

sk_sp<SkData> SharedBuffer::GetAsSkData() const {
  size_t buffer_length = size();
  sk_sp<SkData> data = SkData::MakeUninitialized(buffer_length);
  char* buffer = static_cast<char*>(data->writable_data());
  const char* segment = nullptr;
  size_t position = 0;
  while (size_t segment_size = GetSomeDataInternal(segment, position)) {
    memcpy(buffer + position, segment, segment_size);
    position += segment_size;
  }

  if (position != buffer_length) {
    NOTREACHED();
    // Don't return the incomplete SkData.
    return nullptr;
  }
  return data;
}

void SharedBuffer::OnMemoryDump(const String& dump_prefix,
                                WebProcessMemoryDump* memory_dump) const {
  if (buffer_.size()) {
    WebMemoryAllocatorDump* dump =
        memory_dump->CreateMemoryAllocatorDump(dump_prefix + "/shared_buffer");
    dump->AddScalar("size", "bytes", buffer_.size());
    memory_dump->AddSuballocation(
        dump->Guid(), String(WTF::Partitions::kAllocatedObjectPoolName));
  } else {
    // If there is data in the segments, then it should have been allocated
    // using fastMalloc.
    const String data_dump_name = dump_prefix + "/segments";
    auto* dump = memory_dump->CreateMemoryAllocatorDump(data_dump_name);
    dump->AddScalar("size", "bytes", size_);
    memory_dump->AddSuballocation(
        dump->Guid(), String(WTF::Partitions::kAllocatedObjectPoolName));
  }
}

SharedBuffer::DeprecatedFlatData::DeprecatedFlatData(
    scoped_refptr<const SharedBuffer> buffer)
    : buffer_(std::move(buffer)) {
  DCHECK(buffer_);

  if (buffer_->size() <= buffer_->buffer_.size()) {
    // The SharedBuffer is not segmented - just point to its data.
    data_ = buffer_->buffer_.data();
    return;
  }

  // Merge all segments.
  flat_buffer_.ReserveInitialCapacity(buffer_->size());
  buffer_->ForEachSegment([this](const char* segment, size_t segment_size,
                                 size_t segment_offset) -> bool {
    flat_buffer_.Append(segment, segment_size);
    return true;
  });

  data_ = flat_buffer_.data();
}

}  // namespace blink
