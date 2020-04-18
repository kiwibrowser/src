// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/cursors/webcursor.h"

#include <algorithm>

#include "base/logging.h"
#include "base/pickle.h"
#include "build/build_config.h"
#include "third_party/blink/public/platform/web_image.h"

using blink::WebCursorInfo;

static const int kMaxCursorDimension = 1024;

namespace content {

WebCursor::WebCursor() : type_(WebCursorInfo::kTypePointer), custom_scale_(1) {
  InitPlatformData();
}

WebCursor::~WebCursor() {
  Clear();
}

WebCursor::WebCursor(const WebCursor& other) {
  InitPlatformData();
  Copy(other);
}

const WebCursor& WebCursor::operator=(const WebCursor& other) {
  if (this == &other)
    return *this;

  Clear();
  Copy(other);
  return *this;
}

void WebCursor::InitFromCursorInfo(const CursorInfo& cursor_info) {
  Clear();

  type_ = cursor_info.type;
  hotspot_ = cursor_info.hotspot;
  if (IsCustom())
    SetCustomData(cursor_info.custom_image);
  custom_scale_ = cursor_info.image_scale_factor;
  CHECK(custom_scale_ > 0);
  ClampHotspot();
}

void WebCursor::GetCursorInfo(CursorInfo* cursor_info) const {
  cursor_info->type = static_cast<WebCursorInfo::Type>(type_);
  cursor_info->hotspot = hotspot_;
  ImageFromCustomData(&cursor_info->custom_image);
  cursor_info->image_scale_factor = custom_scale_;
}

bool WebCursor::Deserialize(base::PickleIterator* iter) {
  int type, hotspot_x, hotspot_y, size_x, size_y, data_len;
  float scale;
  const char* data;

  // Leave |this| unmodified unless we are going to return success.
  if (!iter->ReadInt(&type) ||
      !iter->ReadInt(&hotspot_x) ||
      !iter->ReadInt(&hotspot_y) ||
      !iter->ReadLength(&size_x) ||
      !iter->ReadLength(&size_y) ||
      !iter->ReadFloat(&scale) ||
      !iter->ReadData(&data, &data_len))
    return false;

  // Ensure the size is sane, and there is enough data.
  if (size_x > kMaxCursorDimension ||
      size_y > kMaxCursorDimension)
    return false;

  // Ensure scale isn't ridiculous, and the scaled image size is still sane.
  if (scale < 0.01 || scale > 100 ||
      size_x / scale > kMaxCursorDimension ||
      size_y / scale > kMaxCursorDimension)
    return false;

  type_ = type;

  if (type == WebCursorInfo::kTypeCustom) {
    if (size_x > 0 && size_y > 0) {
      // The * 4 is because the expected format is an array of RGBA pixel
      // values.
      if (size_x * size_y * 4 != data_len) {
        DLOG(WARNING) << "WebCursor's data length and image size mismatch: "
                      << size_x << "x" << size_y << "x4 != " << data_len;
        return false;
      }

      hotspot_.set_x(hotspot_x);
      hotspot_.set_y(hotspot_y);
      custom_size_.set_width(size_x);
      custom_size_.set_height(size_y);
      custom_scale_ = scale;
      ClampHotspot();

      custom_data_.clear();
      if (data_len > 0) {
        custom_data_.resize(data_len);
        memcpy(&custom_data_[0], data, data_len);
      }
    }
  }
  return true;
}

void WebCursor::Serialize(base::Pickle* pickle) const {
  pickle->WriteInt(type_);
  pickle->WriteInt(hotspot_.x());
  pickle->WriteInt(hotspot_.y());
  pickle->WriteInt(custom_size_.width());
  pickle->WriteInt(custom_size_.height());
  pickle->WriteFloat(custom_scale_);

  const char* data = nullptr;
  if (!custom_data_.empty())
    data = &custom_data_[0];
  pickle->WriteData(data, custom_data_.size());
}

bool WebCursor::IsCustom() const {
  return type_ == WebCursorInfo::kTypeCustom;
}

bool WebCursor::IsEqual(const WebCursor& other) const {
  if (type_ != other.type_)
    return false;

  if (!IsPlatformDataEqual(other))
    return false;

  return hotspot_ == other.hotspot_ &&
         custom_size_ == other.custom_size_ &&
         custom_scale_ == other.custom_scale_ &&
         custom_data_ == other.custom_data_;
}

void WebCursor::Clear() {
  type_ = WebCursorInfo::kTypePointer;
  hotspot_.set_x(0);
  hotspot_.set_y(0);
  custom_size_.set_width(0);
  custom_size_.set_height(0);
  custom_scale_ = 1;
  custom_data_.clear();
  CleanupPlatformData();
}

void WebCursor::Copy(const WebCursor& other) {
  type_ = other.type_;
  hotspot_ = other.hotspot_;
  custom_size_ = other.custom_size_;
  custom_scale_ = other.custom_scale_;
  custom_data_ = other.custom_data_;
  CopyPlatformData(other);
}

void WebCursor::SetCustomData(const SkBitmap& bitmap) {
  CreateCustomData(bitmap, &custom_data_, &custom_size_);
}

void WebCursor::CreateCustomData(const SkBitmap& bitmap,
                                 std::vector<char>* custom_data,
                                 gfx::Size* custom_size) {
  if (bitmap.empty())
    return;

  // Fill custom_data directly with the NativeImage pixels.
  custom_data->resize(bitmap.computeByteSize());
  if (!custom_data->empty()) {
    //This will divide color values by alpha (un-premultiply) if necessary
    SkImageInfo dstInfo = bitmap.info().makeAlphaType(kUnpremul_SkAlphaType);
    bitmap.readPixels(dstInfo, &(*custom_data)[0], dstInfo.minRowBytes(), 0, 0);
  }
  custom_size->set_width(bitmap.width());
  custom_size->set_height(bitmap.height());
}

void WebCursor::ImageFromCustomData(SkBitmap* image) const {
  if (custom_data_.empty())
    return;

  SkImageInfo image_info = SkImageInfo::MakeN32(custom_size_.width(),
                                                custom_size_.height(),
                                                kUnpremul_SkAlphaType);
  if (!image->tryAllocPixels(image_info))
    return;
  memcpy(image->getPixels(), &custom_data_[0], custom_data_.size());
}

void WebCursor::ClampHotspot() {
  if (!IsCustom())
    return;

  // Clamp the hotspot to the custom image's dimensions.
  hotspot_.set_x(std::max(0,
                          std::min(custom_size_.width() - 1, hotspot_.x())));
  hotspot_.set_y(std::max(0,
                          std::min(custom_size_.height() - 1, hotspot_.y())));
}

}  // namespace content
