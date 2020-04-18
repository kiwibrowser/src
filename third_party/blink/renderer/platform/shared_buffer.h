/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SHARED_BUFFER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SHARED_BUFFER_H_

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

class SkData;
template <typename T>
class sk_sp;

namespace blink {

class WebProcessMemoryDump;

class PLATFORM_EXPORT SharedBuffer : public RefCounted<SharedBuffer> {
 public:
  enum : unsigned { kSegmentSize = 0x1000 };

  static scoped_refptr<SharedBuffer> Create() {
    return base::AdoptRef(new SharedBuffer);
  }

  HAS_STRICTLY_TYPED_ARG
  static scoped_refptr<SharedBuffer> Create(STRICTLY_TYPED_ARG(size)) {
    STRICT_ARG_TYPE(size_t);
    return base::AdoptRef(new SharedBuffer(size));
  }

  HAS_STRICTLY_TYPED_ARG
  static scoped_refptr<SharedBuffer> Create(const char* data,
                                            STRICTLY_TYPED_ARG(size)) {
    STRICT_ARG_TYPE(size_t);
    return base::AdoptRef(new SharedBuffer(data, size));
  }

  HAS_STRICTLY_TYPED_ARG
  static scoped_refptr<SharedBuffer> Create(const unsigned char* data,
                                            STRICTLY_TYPED_ARG(size)) {
    STRICT_ARG_TYPE(size_t);
    return base::AdoptRef(new SharedBuffer(data, size));
  }

  static scoped_refptr<SharedBuffer> AdoptVector(Vector<char>&);

  ~SharedBuffer();

  // DEPRECATED: use a segment iterator, FlatData or Copy() instead.
  //
  // Calling this function will force internal segmented buffers to be merged
  // into a flat buffer. Use getSomeData() whenever possible for better
  // performance.
  const char* Data() const;

  size_t size() const;

  bool IsEmpty() const { return !size(); }

  void Append(const SharedBuffer&);

  HAS_STRICTLY_TYPED_ARG
  void Append(const char* data, STRICTLY_TYPED_ARG(size)) {
    ALLOW_NUMERIC_ARG_TYPES_PROMOTABLE_TO(size_t);
    AppendInternal(data, size);
  }
  void Append(const Vector<char>&);

  void Clear();

  // Copies the segmented data into a contiguous buffer.  Use GetSomeData() or
  // ForEachSegment() whenever possible, as they are cheaper.
  Vector<char> Copy() const;

  // Return the number of consecutive bytes after "position". "data"
  // points to the first byte.
  // Return 0 when no more data left.
  // When extracting all data with getSomeData(), the caller should
  // repeat calling it until it returns 0.
  // Usage:
  //      const char* segment;
  //      size_t pos = 0;
  //      while (size_t length = sharedBuffer->getSomeData(segment, pos)) {
  //          // Use the data. for example: decoder->decode(segment, length);
  //          pos += length;
  //      }
  HAS_STRICTLY_TYPED_ARG
  size_t GetSomeData(
      const char*& data,
      STRICTLY_TYPED_ARG(position) = static_cast<size_t>(0)) const {
    STRICT_ARG_TYPE(size_t);
    return GetSomeDataInternal(data, position);
  }

  // Copies |byteLength| bytes from the beginning of the content data into
  // |dest| as a flat buffer. Returns true on success, otherwise the content of
  // |dest| is not guaranteed.
  HAS_STRICTLY_TYPED_ARG
  WARN_UNUSED_RESULT
  bool GetBytes(void* dest, STRICTLY_TYPED_ARG(byte_length)) const {
    STRICT_ARG_TYPE(size_t);
    return GetBytesInternal(dest, byte_length);
  }

  // Creates an SkData and copies this SharedBuffer's contents to that
  // SkData without merging segmented buffers into a flat buffer.
  sk_sp<SkData> GetAsSkData() const;

  void OnMemoryDump(const String& dump_prefix, WebProcessMemoryDump*) const;

  // Helper for applying a lambda to all data segments, sequentially:
  //
  //   bool func(const char* segment, size_t segment_size,
  //             size_t segment_offset);
  //
  // The iterator stops early when the lambda returns |false|.
  //
  template <typename Func>
  void ForEachSegment(Func&& func) const {
    const char* segment;
    size_t pos = 0;

    while (size_t length = GetSomeData(segment, pos)) {
      if (!func(segment, length, pos))
        break;
      pos += length;
    }
  }

  // Helper for providing a contiguous view of the data.  If the SharedBuffer is
  // segmented, this will copy/merge all segments into a temporary buffer.
  // In general, clients should use the efficient/segmented accessors.
  class PLATFORM_EXPORT DeprecatedFlatData {
    STACK_ALLOCATED();

   public:
    explicit DeprecatedFlatData(scoped_refptr<const SharedBuffer>);

    const char* Data() const { return data_; }
    size_t size() const { return buffer_->size(); }

   private:
    scoped_refptr<const SharedBuffer> buffer_;
    Vector<char> flat_buffer_;
    const char* data_;
  };

 private:
  SharedBuffer();
  explicit SharedBuffer(size_t);
  SharedBuffer(const char*, size_t);
  SharedBuffer(const unsigned char*, size_t);

  // See SharedBuffer::data().
  void MergeSegmentsIntoBuffer() const;

  void AppendInternal(const char* data, size_t);
  bool GetBytesInternal(void* dest, size_t) const;
  size_t GetSomeDataInternal(const char*& data, size_t position) const;

  size_t size_;
  mutable Vector<char> buffer_;
  mutable Vector<char*> segments_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SHARED_BUFFER_H_
