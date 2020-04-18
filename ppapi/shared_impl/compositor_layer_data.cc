// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/shared_impl/compositor_layer_data.h"

namespace ppapi {

namespace {

template <typename T>
void Copy(std::unique_ptr<T>* a, const std::unique_ptr<T>& b) {
  if (b) {
    if (!(*a))
      a->reset(new T());
    **a = *b;
  } else {
    a->reset();
  }
}

}  // namespace

const CompositorLayerData& CompositorLayerData::operator=(
    const CompositorLayerData& other) {
  DCHECK(other.is_null() || other.is_valid());

  common = other.common;
  Copy(&color, other.color);
  Copy(&texture, other.texture);
  Copy(&image, other.image);

  return *this;
}

}  // namespace ppapi
