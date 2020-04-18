// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/test_options_provider.h"

namespace cc {
class TestOptionsProvider::DiscardableManager
    : public SkStrikeServer::DiscardableHandleManager,
      public SkStrikeClient::DiscardableHandleManager {
 public:
  DiscardableManager() = default;
  ~DiscardableManager() override = default;

  // SkStrikeServer::DiscardableHandleManager implementation.
  SkDiscardableHandleId createHandle() override { return next_handle_id_++; }
  bool lockHandle(SkDiscardableHandleId handle_id) override {
    CHECK_LT(handle_id, next_handle_id_);
    return true;
  }

  // SkStrikeClient::DiscardableHandleManager implementation.
  bool deleteHandle(SkDiscardableHandleId handle_id) override {
    CHECK_LT(handle_id, next_handle_id_);
    return false;
  }

 private:
  SkDiscardableHandleId next_handle_id_ = 1u;
};

TestOptionsProvider::TestOptionsProvider()
    : discardable_manager_(sk_make_sp<DiscardableManager>()),
      strike_server_(discardable_manager_.get()),
      strike_client_(discardable_manager_),
      color_space_(SkColorSpace::MakeSRGB()),
      serialize_options_(this,
                         this,
                         &canvas_,
                         &strike_server_,
                         color_space_.get(),
                         can_use_lcd_text_,
                         SkMatrix::I()),
      deserialize_options_(this, &strike_client_) {}

TestOptionsProvider::~TestOptionsProvider() = default;

void TestOptionsProvider::PushFonts() {
  std::vector<uint8_t> font_data;
  strike_server_.writeStrikeData(&font_data);
  if (font_data.size() == 0u)
    return;
  CHECK(strike_client_.readStrikeData(font_data.data(), font_data.size()));
}

ImageProvider::ScopedDecodedDrawImage TestOptionsProvider::GetDecodedDrawImage(
    const DrawImage& draw_image) {
  uint32_t image_id = draw_image.paint_image().GetSkImage()->uniqueID();
  // Lock and reuse the entry if possible.
  const EntryKey entry_key(TransferCacheEntryType::kImage, image_id);
  if (LockEntryDirect(entry_key)) {
    return ScopedDecodedDrawImage(
        DecodedDrawImage(image_id, SkSize::MakeEmpty(), draw_image.scale(),
                         draw_image.filter_quality(), true));
  }

  decoded_images_.push_back(draw_image);
  SkBitmap bitmap;
  const auto& paint_image = draw_image.paint_image();
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(paint_image.width(), paint_image.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  // Create a transfer cache entry for this image.
  auto color_space = SkColorSpace::MakeSRGB();
  ClientImageTransferCacheEntry cache_entry(&bitmap.pixmap(),
                                            color_space.get());
  std::vector<uint8_t> data;
  data.resize(cache_entry.SerializedSize());
  if (!cache_entry.Serialize(base::span<uint8_t>(data.data(), data.size()))) {
    return ScopedDecodedDrawImage();
  }

  CreateEntryDirect(entry_key, base::span<uint8_t>(data.data(), data.size()));

  return ScopedDecodedDrawImage(
      DecodedDrawImage(image_id, SkSize::MakeEmpty(), draw_image.scale(),
                       draw_image.filter_quality(), true));
}

}  // namespace cc
