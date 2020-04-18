// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/image_transfer_cache_entry.h"

#include "base/logging.h"
#include "base/numerics/checked_math.h"
#include "cc/paint/paint_op_reader.h"
#include "cc/paint/paint_op_writer.h"
#include "third_party/skia/include/gpu/GrContext.h"

namespace cc {

ClientImageTransferCacheEntry::ClientImageTransferCacheEntry(
    const SkPixmap* pixmap,
    const SkColorSpace* target_color_space)
    : id_(s_next_id_.GetNext()),
      pixmap_(pixmap),
      target_color_space_(target_color_space) {
  size_t target_color_space_size =
      target_color_space ? target_color_space->writeToMemory(nullptr) : 0u;
  size_t pixmap_color_space_size =
      pixmap_->colorSpace() ? pixmap_->colorSpace()->writeToMemory(nullptr)
                            : 0u;

  // Compute and cache the size of the data.
  // We write the following:
  // - Image color type (uint32_t)
  // - Image width (uint32_t)
  // - Image height (uint32_t)
  // - Pixels size (uint32_t)
  // - Pixels (variable)
  base::CheckedNumeric<size_t> safe_size;
  safe_size += sizeof(uint32_t);  // color type
  safe_size += sizeof(uint32_t);  // width
  safe_size += sizeof(uint32_t);  // height
  safe_size += sizeof(size_t);    // pixels size
  safe_size += pixmap_->computeByteSize();
  safe_size += PaintOpWriter::HeaderBytes();
  safe_size += target_color_space_size + sizeof(size_t);
  safe_size += pixmap_color_space_size + sizeof(size_t);
  size_ = safe_size.ValueOrDie();
}

ClientImageTransferCacheEntry::~ClientImageTransferCacheEntry() = default;

// static
base::AtomicSequenceNumber ClientImageTransferCacheEntry::s_next_id_;

size_t ClientImageTransferCacheEntry::SerializedSize() const {
  return size_;
}

uint32_t ClientImageTransferCacheEntry::Id() const {
  return id_;
}

bool ClientImageTransferCacheEntry::Serialize(base::span<uint8_t> data) const {
  DCHECK_GE(data.size(), SerializedSize());

  // We don't need to populate the SerializeOptions here since the writer is
  // only used for serializing primitives.
  PaintOp::SerializeOptions options(nullptr, nullptr, nullptr, nullptr, nullptr,
                                    false, SkMatrix::I());
  PaintOpWriter writer(data.data(), data.size(), options);
  writer.Write(pixmap_->colorType());
  writer.Write(pixmap_->width());
  writer.Write(pixmap_->height());
  size_t pixmap_size = pixmap_->computeByteSize();
  writer.WriteSize(pixmap_size);
  // TODO(enne): we should consider caching these in some form.
  writer.Write(pixmap_->colorSpace());
  writer.Write(target_color_space_);
  writer.WriteData(pixmap_size, pixmap_->addr());
  if (writer.size() != data.size())
    return false;

  return true;
}

ServiceImageTransferCacheEntry::ServiceImageTransferCacheEntry() = default;
ServiceImageTransferCacheEntry::~ServiceImageTransferCacheEntry() = default;

ServiceImageTransferCacheEntry::ServiceImageTransferCacheEntry(
    ServiceImageTransferCacheEntry&& other) = default;
ServiceImageTransferCacheEntry& ServiceImageTransferCacheEntry::operator=(
    ServiceImageTransferCacheEntry&& other) = default;

size_t ServiceImageTransferCacheEntry::CachedSize() const {
  return size_;
}

bool ServiceImageTransferCacheEntry::Deserialize(
    GrContext* context,
    base::span<const uint8_t> data) {
  // We don't need to populate the DeSerializeOptions here since the reader is
  // only used for de-serializing primitives.
  PaintOp::DeserializeOptions options(nullptr, nullptr);
  PaintOpReader reader(data.data(), data.size(), options);
  SkColorType color_type;
  reader.Read(&color_type);
  uint32_t width;
  reader.Read(&width);
  uint32_t height;
  reader.Read(&height);
  size_t pixel_size;
  reader.ReadSize(&pixel_size);
  size_ = data.size();
  sk_sp<SkColorSpace> pixmap_color_space;
  reader.Read(&pixmap_color_space);
  sk_sp<SkColorSpace> target_color_space;
  reader.Read(&target_color_space);

  if (!reader.valid())
    return false;

  SkImageInfo image_info = SkImageInfo::Make(
      width, height, color_type, kPremul_SkAlphaType, pixmap_color_space);
  if (image_info.computeMinByteSize() > pixel_size)
    return false;
  const volatile void* pixel_data = reader.ExtractReadableMemory(pixel_size);
  if (!reader.valid())
    return false;

  // Const-cast away the "volatile" on |pixel_data|. We specifically understand
  // that a malicious caller may change our pixels under us, and are OK with
  // this as the worst case scenario is visual corruption.
  SkPixmap pixmap(image_info, const_cast<const void*>(pixel_data),
                  image_info.minRowBytes());

  // Depending on whether the pixmap will fit in a GPU texture, either create
  // a software or GPU SkImage.
  uint32_t max_size = context->maxTextureSize();
  bool fits_on_gpu = width <= max_size && height <= max_size;
  if (fits_on_gpu) {
    sk_sp<SkImage> image = SkImage::MakeFromRaster(pixmap, nullptr, nullptr);
    if (!image)
      return false;
    image_ = image->makeTextureImage(context, nullptr);
    if (!image_)
      return false;
    if (target_color_space) {
      image_ = image_->makeColorSpace(target_color_space,
                                      SkTransferFunctionBehavior::kIgnore);
    }
  } else {
    sk_sp<SkImage> original =
        SkImage::MakeFromRaster(pixmap, [](const void*, void*) {}, nullptr);
    if (!original)
      return false;
    if (target_color_space) {
      image_ = original->makeColorSpace(target_color_space,
                                        SkTransferFunctionBehavior::kIgnore);
      // If color space conversion is a noop, use original data.
      if (image_ == original)
        image_ = SkImage::MakeRasterCopy(pixmap);
    } else {
      // No color conversion to do, use original data.
      image_ = SkImage::MakeRasterCopy(pixmap);
    }
  }

  // TODO(enne): consider adding in the DeleteSkImageAndPreventCaching
  // optimization from GpuImageDecodeCache where we forcefully remove the
  // intermediate from Skia's cache.
  return image_;
}

}  // namespace cc
