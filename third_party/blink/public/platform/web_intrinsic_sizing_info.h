// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_INTRINSIC_SIZING_INFO_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_INTRINSIC_SIZING_INFO_H_

#include "third_party/blink/public/platform/web_float_size.h"

namespace blink {

struct WebIntrinsicSizingInfo {
  WebIntrinsicSizingInfo() : has_width(true), has_height(true) {}

  WebFloatSize size;
  WebFloatSize aspect_ratio;
  bool has_width;
  bool has_height;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_INTRINSIC_SIZING_INFO_H_
