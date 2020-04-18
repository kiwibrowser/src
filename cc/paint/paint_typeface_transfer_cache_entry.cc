// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_typeface_transfer_cache_entry.h"

namespace cc {
namespace {

size_t kMaxFilenameSize = 1024;
size_t kMaxFamilyNameSize = 128;

class DataWriter {
 public:
  explicit DataWriter(base::span<uint8_t> data) : data_(data) {}

  template <typename T>
  void WriteSimple(const T& val) {
    DCHECK_LE(sizeof(T), data_.size());
    *reinterpret_cast<T*>(data_.data()) = val;
    data_ = data_.subspan(sizeof(T));
  }
  void WriteData(size_t bytes, const void* input) {
    DCHECK_LE(bytes, data_.size());
    memcpy(data_.data(), input, bytes);
    data_ = data_.subspan(bytes);
  }

 private:
  base::span<uint8_t> data_;
};

class SizeCounter {
 public:
  SizeCounter() = default;

  template <typename T>
  void WriteSimple(const T& val) {
    size_ += sizeof(T);
  }
  void WriteData(size_t bytes, const void* input) { size_ += bytes; }
  size_t size() const { return size_; }

 private:
  size_t size_ = 0;
};

}  // namespace

ClientPaintTypefaceTransferCacheEntry::ClientPaintTypefaceTransferCacheEntry(
    const PaintTypeface& typeface)
    : typeface_(typeface) {
  SizeCounter counter;
  SerializeInternal(&counter);
  size_ = counter.size();
}

ClientPaintTypefaceTransferCacheEntry::
    ~ClientPaintTypefaceTransferCacheEntry() = default;

uint32_t ClientPaintTypefaceTransferCacheEntry::Id() const {
  return typeface_.sk_id();
}

size_t ClientPaintTypefaceTransferCacheEntry::SerializedSize() const {
  return size_;
}

bool ClientPaintTypefaceTransferCacheEntry::Serialize(
    base::span<uint8_t> data) const {
  DataWriter writer(data);
  return SerializeInternal(&writer);
}

template <typename Writer>
bool ClientPaintTypefaceTransferCacheEntry::SerializeInternal(
    Writer* writer) const {
  writer->WriteSimple(typeface_.sk_id());
  writer->WriteSimple(static_cast<uint8_t>(typeface_.type()));
  switch (typeface_.type()) {
    case PaintTypeface::Type::kTestTypeface:
      // Nothing to serialize here.
      break;
    case PaintTypeface::Type::kSkTypeface:
      // Nothing to do here. This should never be the case when everything is
      // implemented. This should be a NOTREACHED() eventually.
      break;
    case PaintTypeface::Type::kFontConfigInterfaceIdAndTtcIndex:
      writer->WriteSimple(typeface_.font_config_interface_id());
      writer->WriteSimple(typeface_.ttc_index());
      break;
    case PaintTypeface::Type::kFilenameAndTtcIndex:
      writer->WriteSimple(typeface_.filename().size());
      writer->WriteData(typeface_.filename().size(),
                        typeface_.filename().data());
      writer->WriteSimple(typeface_.ttc_index());
      break;
    case PaintTypeface::Type::kFamilyNameAndFontStyle:
      writer->WriteSimple(typeface_.family_name().size());
      writer->WriteData(typeface_.family_name().size(),
                        typeface_.family_name().data());
      writer->WriteSimple(typeface_.font_style().weight());
      writer->WriteSimple(typeface_.font_style().width());
      writer->WriteSimple(typeface_.font_style().slant());
      break;
  }
  return true;
}

ServicePaintTypefaceTransferCacheEntry::
    ServicePaintTypefaceTransferCacheEntry() = default;
ServicePaintTypefaceTransferCacheEntry::
    ~ServicePaintTypefaceTransferCacheEntry() = default;

size_t ServicePaintTypefaceTransferCacheEntry::CachedSize() const {
  return size_;
}

bool ServicePaintTypefaceTransferCacheEntry::Deserialize(
    GrContext* context,
    base::span<const uint8_t> data) {
  data_ = data;
  size_t initial_size = data_.size();

  SkFontID id;
  uint8_t type;
  ReadSimple(&id);
  ReadSimple(&type);
  if (!valid_ || type > static_cast<uint8_t>(PaintTypeface::Type::kLastType)) {
    valid_ = false;
    return false;
  }
  switch (static_cast<PaintTypeface::Type>(type)) {
    case PaintTypeface::Type::kTestTypeface:
      typeface_ = PaintTypeface::TestTypeface();
      break;
    case PaintTypeface::Type::kSkTypeface:
      // TODO(vmpstr): This shouldn't ever happen once everything is
      // implemented. So this should be a failure (ie |valid_| = false).
      break;
    case PaintTypeface::Type::kFontConfigInterfaceIdAndTtcIndex: {
      int font_config_interface_id = 0;
      int ttc_index = 0;
      ReadSimple(&font_config_interface_id);
      ReadSimple(&ttc_index);
      if (!valid_)
        return false;
      typeface_ = PaintTypeface::FromFontConfigInterfaceIdAndTtcIndex(
          font_config_interface_id, ttc_index);
      break;
    }
    case PaintTypeface::Type::kFilenameAndTtcIndex: {
      size_t size;
      ReadSimple(&size);
      if (!valid_ || size > kMaxFilenameSize) {
        valid_ = false;
        return false;
      }

      std::unique_ptr<char[]> buffer(new char[size]);
      ReadData(size, buffer.get());
      std::string filename(buffer.get(), size);

      int ttc_index = 0;
      ReadSimple(&ttc_index);
      if (!valid_)
        return false;
      typeface_ = PaintTypeface::FromFilenameAndTtcIndex(filename, ttc_index);
      break;
    }
    case PaintTypeface::Type::kFamilyNameAndFontStyle: {
      size_t size;
      ReadSimple(&size);
      if (!valid_ || size > kMaxFamilyNameSize) {
        valid_ = false;
        return false;
      }

      std::unique_ptr<char[]> buffer(new char[size]);
      ReadData(size, buffer.get());
      std::string family_name(buffer.get(), size);

      int weight = 0;
      int width = 0;
      SkFontStyle::Slant slant = SkFontStyle::kUpright_Slant;
      ReadSimple(&weight);
      ReadSimple(&width);
      ReadSimple(&slant);
      if (!valid_)
        return false;

      typeface_ = PaintTypeface::FromFamilyNameAndFontStyle(
          family_name, SkFontStyle(weight, width, slant));
      break;
    }
  }
  typeface_.SetSkId(id);

  // Set the size to however much data we read.
  size_ = initial_size - data_.size();
  data_ = base::span<uint8_t>();
  return valid_;
}

template <typename T>
void ServicePaintTypefaceTransferCacheEntry::ReadSimple(T* val) {
  if (data_.size() < sizeof(T))
    valid_ = false;
  if (!valid_)
    return;
  *val = *reinterpret_cast<const T*>(data_.data());
  data_ = data_.subspan(sizeof(T));
}

void ServicePaintTypefaceTransferCacheEntry::ReadData(size_t bytes,
                                                      void* data) {
  if (data_.size() < bytes)
    valid_ = false;
  if (!valid_)
    return;
  memcpy(data, data_.data(), bytes);
  data_ = data_.subspan(bytes);
}

}  // namespace cc
