/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_CACHED_METADATA_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_CACHED_METADATA_H_

#include <stdint.h>
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

// |m_serializedData| consists of a 32 bit marker, 32 bits type ID, and actual
// data.
constexpr size_t kCacheDataTypeStart = sizeof(uint32_t);
constexpr size_t kCachedMetaDataStart = kCacheDataTypeStart + sizeof(uint32_t);

// Metadata retrieved from the embedding application's cache.
//
// Serialized data is NOT portable across architectures. However, reading the
// data type ID will reject data generated with a different byte-order.
class PLATFORM_EXPORT CachedMetadata : public RefCounted<CachedMetadata> {
 public:
  static scoped_refptr<CachedMetadata> Create(uint32_t data_type_id,
                                              const char* data,
                                              size_t size) {
    return base::AdoptRef(new CachedMetadata(data_type_id, data, size));
  }

  static scoped_refptr<CachedMetadata> CreateFromSerializedData(
      const char* data,
      size_t);

  ~CachedMetadata() = default;

  const Vector<char>& SerializedData() const { return serialized_data_; }

  uint32_t DataTypeID() const {
    DCHECK_GE(serialized_data_.size(), kCachedMetaDataStart);
    return *reinterpret_cast_ptr<uint32_t*>(
        const_cast<char*>(serialized_data_.data() + kCacheDataTypeStart));
  }

  const char* Data() const {
    DCHECK_GE(serialized_data_.size(), kCachedMetaDataStart);
    return serialized_data_.data() + kCachedMetaDataStart;
  }

  size_t size() const {
    DCHECK_GE(serialized_data_.size(), kCachedMetaDataStart);
    return serialized_data_.size() - kCachedMetaDataStart;
  }

 private:
  CachedMetadata(const char* data, size_t);
  CachedMetadata(uint32_t data_type_id, const char* data, size_t);

  // Since the serialization format supports random access, storing it in
  // serialized form avoids need for a copy during serialization.
  Vector<char> serialized_data_;
};

}  // namespace blink

#endif
