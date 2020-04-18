// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/color_space_transfer_cache_entry.h"

#include "ui/gfx/ipc/color/gfx_param_traits.h"

namespace cc {

ClientColorSpaceTransferCacheEntry::ClientColorSpaceTransferCacheEntry(
    const RasterColorSpace& raster_color_space)
    : id_(raster_color_space.color_space_id) {
  DCHECK(raster_color_space.color_space.IsValid());
  IPC::ParamTraits<gfx::ColorSpace>::Write(&pickle_,
                                           raster_color_space.color_space);
}

ClientColorSpaceTransferCacheEntry::~ClientColorSpaceTransferCacheEntry() =
    default;

uint32_t ClientColorSpaceTransferCacheEntry::Id() const {
  return id_;
}

size_t ClientColorSpaceTransferCacheEntry::SerializedSize() const {
  return pickle_.size();
}

bool ClientColorSpaceTransferCacheEntry::Serialize(
    base::span<uint8_t> data) const {
  DCHECK_GE(data.size(), pickle_.size());
  memcpy(data.data(), pickle_.data(), pickle_.size());
  return true;
}

ServiceColorSpaceTransferCacheEntry::ServiceColorSpaceTransferCacheEntry() =
    default;

ServiceColorSpaceTransferCacheEntry::~ServiceColorSpaceTransferCacheEntry() =
    default;

size_t ServiceColorSpaceTransferCacheEntry::CachedSize() const {
  return sizeof(gfx::ColorSpace);
}

bool ServiceColorSpaceTransferCacheEntry::Deserialize(
    GrContext* context,
    base::span<const uint8_t> data) {
  base::Pickle pickle(reinterpret_cast<const char*>(data.data()), data.size());
  base::PickleIterator iterator(pickle);
  if (!IPC::ParamTraits<gfx::ColorSpace>::Read(&pickle, &iterator,
                                               &color_space_))
    return false;
  return color_space_.IsValid();
}

}  // namespace cc
