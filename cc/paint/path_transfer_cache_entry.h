// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PATH_TRANSFER_CACHE_ENTRY_H_
#define CC_PAINT_PATH_TRANSFER_CACHE_ENTRY_H_

#include "base/containers/span.h"
#include "cc/paint/paint_export.h"
#include "cc/paint/transfer_cache_entry.h"
#include "third_party/skia/include/core/SkPath.h"

namespace cc {

class CC_PAINT_EXPORT ClientPathTransferCacheEntry
    : public ClientTransferCacheEntryBase<TransferCacheEntryType::kPath> {
 public:
  explicit ClientPathTransferCacheEntry(const SkPath& path);
  ~ClientPathTransferCacheEntry() final;
  uint32_t Id() const final;
  size_t SerializedSize() const final;
  bool Serialize(base::span<uint8_t> data) const final;

 private:
  SkPath path_;
  size_t size_ = 0u;
};

class CC_PAINT_EXPORT ServicePathTransferCacheEntry
    : public ServiceTransferCacheEntryBase<TransferCacheEntryType::kPath> {
 public:
  ServicePathTransferCacheEntry();
  ~ServicePathTransferCacheEntry() final;
  size_t CachedSize() const final;
  bool Deserialize(GrContext* context, base::span<const uint8_t> data) final;

  const SkPath& path() const { return path_; }

 private:
  SkPath path_;
  size_t size_ = 0;
};

}  // namespace cc

#endif  // CC_PAINT_PATH_TRANSFER_CACHE_ENTRY_H_
