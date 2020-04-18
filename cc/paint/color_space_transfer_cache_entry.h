// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_COLOR_SPACE_TRANSFER_CACHE_ENTRY_H_
#define CC_PAINT_COLOR_SPACE_TRANSFER_CACHE_ENTRY_H_

#include "base/containers/span.h"
#include "base/pickle.h"
#include "cc/paint/paint_export.h"
#include "cc/paint/transfer_cache_entry.h"
#include "ui/gfx/color_space.h"

namespace cc {

struct RasterColorSpace {
  RasterColorSpace() = default;
  RasterColorSpace(const gfx::ColorSpace& color_space, int color_space_id)
      : color_space(color_space), color_space_id(color_space_id) {}

  gfx::ColorSpace color_space;
  int color_space_id = -1;
};

class CC_PAINT_EXPORT ClientColorSpaceTransferCacheEntry final
    : public ClientTransferCacheEntryBase<TransferCacheEntryType::kColorSpace> {
 public:
  explicit ClientColorSpaceTransferCacheEntry(
      const RasterColorSpace& raster_color_space);
  ~ClientColorSpaceTransferCacheEntry() override;
  uint32_t Id() const override;
  size_t SerializedSize() const override;
  bool Serialize(base::span<uint8_t> data) const final;

 private:
  int id_;
  base::Pickle pickle_;
};

class CC_PAINT_EXPORT ServiceColorSpaceTransferCacheEntry final
    : public ServiceTransferCacheEntryBase<
          TransferCacheEntryType::kColorSpace> {
 public:
  ServiceColorSpaceTransferCacheEntry();
  ~ServiceColorSpaceTransferCacheEntry() override;
  size_t CachedSize() const override;
  bool Deserialize(GrContext* context, base::span<const uint8_t> data) override;

  const gfx::ColorSpace& color_space() const { return color_space_; }

 private:
  gfx::ColorSpace color_space_;
};

}  // namespace cc

#endif  // CC_PAINT_COLOR_SPACE_TRANSFER_CACHE_ENTRY_H_
