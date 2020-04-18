// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/fetch/source_keyed_cached_metadata_handler.h"

#include "base/bit_cast.h"
#include "third_party/blink/renderer/platform/crypto.h"
#include "third_party/blink/renderer/platform/loader/fetch/cached_metadata.h"
#include "third_party/blink/renderer/platform/wtf/string_hasher.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// Defined here for storage/ODR reasons, but initialized in the header.
const size_t SourceKeyedCachedMetadataHandler::kKeySize;

class SourceKeyedCachedMetadataHandler::SingleKeyHandler final
    : public SingleCachedMetadataHandler {
 public:
  void Trace(Visitor* visitor) override {
    visitor->Trace(parent_);
    SingleCachedMetadataHandler::Trace(visitor);
  }

  SingleKeyHandler(SourceKeyedCachedMetadataHandler* parent, Key key)
      : parent_(parent), key_(key) {}

  void SetCachedMetadata(uint32_t data_type_id,
                         const char* data,
                         size_t size,
                         CacheType cache_type) override {
    DCHECK(!parent_->cached_metadata_map_.Contains(key_));
    parent_->cached_metadata_map_.insert(
        key_, CachedMetadata::Create(data_type_id, data, size));
    if (cache_type == CachedMetadataHandler::kSendToPlatform)
      parent_->SendToPlatform();
  }

  void ClearCachedMetadata(CacheType cache_type) override {
    parent_->cached_metadata_map_.erase(key_);
    if (cache_type == CachedMetadataHandler::kSendToPlatform)
      parent_->SendToPlatform();
  }

  scoped_refptr<CachedMetadata> GetCachedMetadata(
      uint32_t data_type_id) const override {
    scoped_refptr<CachedMetadata> cached_metadata =
        parent_->cached_metadata_map_.at(key_);
    if (!cached_metadata || cached_metadata->DataTypeID() != data_type_id)
      return nullptr;
    return cached_metadata;
  }

  String Encoding() const override { return parent_->Encoding(); }

  bool IsServedFromCacheStorage() const override {
    return parent_->IsServedFromCacheStorage();
  }

 private:
  Member<SourceKeyedCachedMetadataHandler> parent_;
  Key key_;
};

class SourceKeyedCachedMetadataHandler::KeyHash {
 public:
  static unsigned GetHash(const Key& key) {
    return StringHasher::ComputeHash(key.data(), key.size());
  }

  static bool Equal(const Key& a, const Key& b) { return a == b; }

  static const bool safe_to_compare_to_empty_or_deleted = true;
};

SingleCachedMetadataHandler* SourceKeyedCachedMetadataHandler::HandlerForSource(
    const String& source) {
  DigestValue digest_value;

  if (!ComputeDigest(kHashAlgorithmSha256,
                     static_cast<const char*>(source.Bytes()),
                     source.CharactersSizeInBytes(), digest_value))
    return nullptr;

  Key key;
  DCHECK_EQ(digest_value.size(), kKeySize);
  memcpy(key.data(), digest_value.data(), kKeySize);

  return new SingleKeyHandler(this, key);
}

void SourceKeyedCachedMetadataHandler::ClearCachedMetadata(
    CachedMetadataHandler::CacheType cache_type) {
  cached_metadata_map_.clear();
  if (cache_type == CachedMetadataHandler::kSendToPlatform)
    SendToPlatform();
};

String SourceKeyedCachedMetadataHandler::Encoding() const {
  return String(encoding_.GetName());
}

// Encoding of keyed map:
//   - marker: CachedMetadataHandler::kSourceKeyedMap (uint32_t)
//   - num_entries (int)
//   - key 1 (Key type)
//   - len data 1 (size_t)
//   - type data 1
//   - data for key 1
//     ...
//   - key N (Key type)
//   - len data N (size_t)
//   - type data N
//   - data for key N

namespace {
// Reading a value from a char buffer without using reinterpret cast. This
// should inline and optimize to the same code as *reinterpret_cast<T>(data),
// but without the risk of undefined behaviour.
template <typename T>
T ReadVal(const char* data) {
  static_assert(base::is_trivially_copyable<T>::value,
                "ReadVal requires the value type to be copyable");
  T ret;
  memcpy(&ret, data, sizeof(T));
  return ret;
}
}  // namespace

void SourceKeyedCachedMetadataHandler::SetSerializedCachedMetadata(
    const char* data,
    size_t size) {
  // We only expect to receive cached metadata from the platform once. If this
  // triggers, it indicates an efficiency problem which is most likely
  // unexpected in code designed to improve performance.
  DCHECK(cached_metadata_map_.IsEmpty());

  // Ensure we have a marker.
  if (size < sizeof(uint32_t))
    return;
  uint32_t marker = ReadVal<uint32_t>(data);
  // Check for our marker to avoid conflicts with other kinds of cached
  // metadata.
  if (marker != CachedMetadataHandler::kSourceKeyedMap) {
    return;
  }
  data += sizeof(uint32_t);
  size -= sizeof(uint32_t);

  // Ensure we have a length.
  if (size < sizeof(int))
    return;
  int num_entries = ReadVal<int>(data);
  data += sizeof(int);
  size -= sizeof(int);

  for (int i = 0; i < num_entries; ++i) {
    // Ensure we have an entry key and size.
    if (size < kKeySize + sizeof(size_t)) {
      cached_metadata_map_.clear();
      return;
    }

    Key key;
    std::copy(data, data + kKeySize, std::begin(key));
    data += kKeySize;
    size_t entry_size = ReadVal<size_t>(data);
    data += sizeof(size_t);

    size -= kKeySize + sizeof(size_t);

    // Ensure we have enough data for this entry.
    if (size < entry_size) {
      cached_metadata_map_.clear();
      return;
    }

    if (scoped_refptr<CachedMetadata> deserialized_entry =
            CachedMetadata::CreateFromSerializedData(data, entry_size)) {
      // Only insert the deserialized entry if it deserialized correctly.
      cached_metadata_map_.insert(key, std::move(deserialized_entry));
    }
    data += entry_size;
    size -= entry_size;
  }

  // Ensure we have no more data.
  if (size > 0) {
    cached_metadata_map_.clear();
  }
};

void SourceKeyedCachedMetadataHandler::SendToPlatform() {
  if (!sender_)
    return;

  if (cached_metadata_map_.IsEmpty()) {
    sender_->Send(nullptr, 0);
  } else {
    Vector<char> serialized_data;
    uint32_t marker = CachedMetadataHandler::kSourceKeyedMap;
    serialized_data.Append(reinterpret_cast<char*>(&marker), sizeof(marker));
    int num_entries = cached_metadata_map_.size();
    serialized_data.Append(reinterpret_cast<char*>(&num_entries),
                           sizeof(num_entries));
    for (const auto& metadata : cached_metadata_map_) {
      serialized_data.Append(metadata.key.data(), kKeySize);
      size_t entry_size = metadata.value->SerializedData().size();
      serialized_data.Append(reinterpret_cast<const char*>(&entry_size),
                             sizeof(entry_size));
      serialized_data.AppendVector(metadata.value->SerializedData());
    }
    sender_->Send(serialized_data.data(), serialized_data.size());
  }
}

}  // namespace blink
