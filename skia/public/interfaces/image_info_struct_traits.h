// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_PUBLIC_INTERFACES_IMAGE_INFO_STRUCT_TRAITS_H_
#define SKIA_PUBLIC_INTERFACES_IMAGE_INFO_STRUCT_TRAITS_H_

#include "skia/public/interfaces/image_info.mojom.h"
#include "third_party/skia/include/core/SkImageInfo.h"

namespace mojo {

template <>
struct StructTraits<skia::mojom::ImageInfoDataView, SkImageInfo> {
  static skia::mojom::ColorType color_type(const SkImageInfo& info);
  static skia::mojom::AlphaType alpha_type(const SkImageInfo& info);
  static skia::mojom::ColorProfileType profile_type(const SkImageInfo& info);
  static uint32_t width(const SkImageInfo& info);
  static uint32_t height(const SkImageInfo& info);
  static bool Read(skia::mojom::ImageInfoDataView data, SkImageInfo* info);
};

}  // namespace mojo

#endif  // SKIA_PUBLIC_INTERFACES_IMAGE_INFO_STRUCT_TRAITS_H_
