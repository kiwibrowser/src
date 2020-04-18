// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/fetch/cached_metadata.h"

#include "third_party/blink/renderer/platform/loader/fetch/cached_metadata_handler.h"

namespace blink {

scoped_refptr<CachedMetadata> CachedMetadata::CreateFromSerializedData(
    const char* data,
    size_t size) {
  // Ensure the data is big enough, otherwise discard the data.
  if (size < kCachedMetaDataStart) {
    return nullptr;
  }
  // Ensure the marker matches, otherwise discard the data.
  if (*reinterpret_cast<const uint32_t*>(data) !=
      CachedMetadataHandler::kSingleEntry) {
    return nullptr;
  }
  return base::AdoptRef(new CachedMetadata(data, size));
}

CachedMetadata::CachedMetadata(const char* data, size_t size) {
  // Serialized metadata should have non-empty data.
  DCHECK_GT(size, kCachedMetaDataStart);
  DCHECK(data);
  // Make sure that the first int in the data is the single entry marker.
  CHECK_EQ(*reinterpret_cast<const uint32_t*>(data),
           CachedMetadataHandler::kSingleEntry);

  serialized_data_.ReserveInitialCapacity(size);
  serialized_data_.Append(data, size);
}

CachedMetadata::CachedMetadata(uint32_t data_type_id,
                               const char* data,
                               size_t size) {
  // Don't allow an ID of 0, it is used internally to indicate errors.
  DCHECK(data_type_id);
  DCHECK(data);

  serialized_data_.ReserveInitialCapacity(kCachedMetaDataStart + size);
  uint32_t marker = CachedMetadataHandler::kSingleEntry;
  serialized_data_.Append(reinterpret_cast<const char*>(&marker),
                          sizeof(uint32_t));
  serialized_data_.Append(reinterpret_cast<const char*>(&data_type_id),
                          sizeof(uint32_t));
  serialized_data_.Append(data, size);
}

}  // namespace blink
