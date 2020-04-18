// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_TYPEFACE_TRANSFER_CACHE_ENTRY_H_
#define CC_PAINT_PAINT_TYPEFACE_TRANSFER_CACHE_ENTRY_H_

#include "base/containers/span.h"
#include "cc/paint/paint_export.h"
#include "cc/paint/paint_typeface.h"
#include "cc/paint/transfer_cache_entry.h"

namespace cc {

class CC_PAINT_EXPORT ClientPaintTypefaceTransferCacheEntry
    : public ClientTransferCacheEntryBase<
          TransferCacheEntryType::kPaintTypeface> {
 public:
  explicit ClientPaintTypefaceTransferCacheEntry(const PaintTypeface& typeface);
  ~ClientPaintTypefaceTransferCacheEntry() final;
  uint32_t Id() const final;
  size_t SerializedSize() const final;
  bool Serialize(base::span<uint8_t> data) const final;

 private:
  template <typename Writer>
  bool SerializeInternal(Writer* writer) const;

  const PaintTypeface typeface_;
  size_t size_ = 0u;
};

class CC_PAINT_EXPORT ServicePaintTypefaceTransferCacheEntry
    : public ServiceTransferCacheEntryBase<
          TransferCacheEntryType::kPaintTypeface> {
 public:
  ServicePaintTypefaceTransferCacheEntry();
  ~ServicePaintTypefaceTransferCacheEntry() final;
  size_t CachedSize() const final;
  bool Deserialize(GrContext* context, base::span<const uint8_t> data) final;

  const PaintTypeface& typeface() const { return typeface_; }

 private:
  template <typename T>
  void ReadSimple(T* val);

  void ReadData(size_t bytes, void* data);

  PaintTypeface typeface_;
  size_t size_ = 0;
  bool valid_ = true;
  // TODO(enne): this transient value shouldn't be a member and should just be
  // passed around internally to functions that need it.
  base::span<const uint8_t> data_;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_TYPEFACE_TRANSFER_CACHE_ENTRY_H_
