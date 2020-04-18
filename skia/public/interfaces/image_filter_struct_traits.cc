// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/public/interfaces/image_filter_struct_traits.h"

namespace mojo {

ImageFilterBuffer::ImageFilterBuffer() = default;

ImageFilterBuffer::ImageFilterBuffer(const ImageFilterBuffer& other) = default;

ImageFilterBuffer::~ImageFilterBuffer() = default;

// static
size_t ArrayTraits<ImageFilterBuffer>::GetSize(const ImageFilterBuffer& b) {
  return b.data->size();
}

// static
uint8_t* ArrayTraits<ImageFilterBuffer>::GetData(ImageFilterBuffer& b) {
  return static_cast<uint8_t*>(b.data->writable_data());
}

// static
const uint8_t* ArrayTraits<ImageFilterBuffer>::GetData(
    const ImageFilterBuffer& b) {
  return b.data->bytes();
}

// static
uint8_t& ArrayTraits<ImageFilterBuffer>::GetAt(ImageFilterBuffer& b, size_t i) {
  return *(static_cast<uint8_t*>(b.data->writable_data()) + i);
}

// static
const uint8_t& ArrayTraits<ImageFilterBuffer>::GetAt(const ImageFilterBuffer& b,
                                                     size_t i) {
  return *(b.data->bytes() + i);
}

// static
bool ArrayTraits<ImageFilterBuffer>::Resize(ImageFilterBuffer& b, size_t size) {
  if (b.data)
    return size == b.data->size();
  b.data = SkData::MakeUninitialized(size);
  return true;
}

}  // namespace mojo
